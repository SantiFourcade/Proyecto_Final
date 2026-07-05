/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "can.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ff.h"
#include "ff_gen_drv.h"
#include "fatfs.h"
#include "fatfs_sd.h"
#include "can_app.h" // Importante para CAN_Message_t
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
#define MAX_DATOS_A_GRABAR   200u
/* USER CODE BEGIN Variables */
extern CAN_HandleTypeDef hcan; // Para monitorear errores en la tarea
volatile uint8_t g_grabacion_activa = 1;
/* Variables Globales de control */
char sd_buffers[2][512];
int active_buffer = 0;
int sd_block_idx = 0;

/* Variables para que la tarea de la SD sepa qué grabar exactamente */
int buffer_ready_record = 0;
int bytes_ready_record = 0;

// Handle para avisarle a la tarea de la SD que trabaje
osThreadId ButtonTaskHandle;
osThreadId CANParserHandle;
osThreadId SDWriterHandle;
/* USER CODE END Variables */

//osThreadId SDRecordHandle;
QueueHandle_t canRxQueueSD; // Nombre definitivo de la cola

/* Private function prototypes -----------------------------------------------*/
void StartButtonTask(void const * argument);
void StartCANParserTask(void const * argument);
void StartSDWriterTask(void const * argument);
void MX_FREERTOS_Init(void); 

/* GetIdleTaskMemory (Static allocation) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/**
  * @brief  FreeRTOS initialization
  */
void MX_FREERTOS_Init(void) {

  /* USER CODE BEGIN RTOS_QUEUES */
  /* Creación de la cola de recepción CAN hacia la SD */
    canRxQueueSD = xQueueCreate(64, sizeof(CAN_Message_t));
    if (canRxQueueSD == NULL) {
        printf("Error creando cola canRxQueueSD\r\n");
    }
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
    osThreadDef(ButtonTask, StartButtonTask, osPriorityBelowNormal, 0, 128);
    ButtonTaskHandle = osThreadCreate(osThread(ButtonTask), NULL);

    osThreadDef(CANParser, StartCANParserTask, osPriorityAboveNormal, 0, 256);
    CANParserHandle = osThreadCreate(osThread(CANParser), NULL);

    osThreadDef(SDWriter, StartSDWriterTask, osPriorityNormal, 0, 512);
    SDWriterHandle = osThreadCreate(osThread(SDWriter), NULL);
  
}

/*
    Analizar si enviamos un mensaje de CAN START y despues matamos la tarea
*/
void StartButtonTask(void const * argument)
{
  uint8_t button_state = 0; // 0=idle, 1=detectado, 2=presionado
  uint32_t press_tick = 0;
  
  osDelay(500); // Espera inicial para estabilizar el sistema
  
  while (HAL_GPIO_ReadPin(Init_GPIO_Port, Init_Pin) == GPIO_PIN_RESET) {
        osDelay(10);
  }
  
  for(;;)
  {
    if (HAL_GPIO_ReadPin(Init_GPIO_Port, Init_Pin) == GPIO_PIN_RESET) 
    {
      if (button_state == 0) 
      {
        button_state = 1;      // Detectamos posible presión
        press_tick = osKernelSysTick();
      } 
      else if (button_state == 1) 
      {
        if ((osKernelSysTick() - press_tick) > 50) 
        {
          button_state = 2;
          
          printf("[BOTON] Enviando orden de START por CAN...\r\n");
          
          // Trama BROADCAST para iniciar adquisición en todos los nodos
          CAN_TxHeaderTypeDef txHeader;
          uint8_t txData[8] = {0}; 
          uint32_t txMailbox;

          txHeader.StdId = 0x100;           // ID global de control
          txHeader.RTR = CAN_RTR_DATA;
          txHeader.IDE = CAN_ID_STD;
          txHeader.DLC = 1;                 
          txData[0] = 0xAA;                 // Código START
          txHeader.TransmitGlobalTime = DISABLE;

          if (HAL_CAN_AddTxMessage(&hcan, &txHeader, txData, &txMailbox) == HAL_OK)
          {
            printf("[CAN TX] Comando START enviado (ID: 0x100)\r\n");
          }
          else
          {
            printf("[CAN TX] ERROR: No hay mailboxes libres o bus saturado\r\n");
          }
        }
      }
      // Obliga a que se envíe un solo mensaje CAN hasta que sueltes el botón.
    } 
    else 
    {
      button_state = 0; // Se soltó el botón, habilitamos la próxima pulsación
    }

    /* Revisamos el botón cada 50ms */
    osDelay(50); 
  }
}

void StartCANParserTask(void const * argument)
{
    CAN_Message_t rxMsg;
    float canValue;

    static uint32_t contador_mensajes = 0;
    
    for(;;) {
        /* Espera por mensaje CAN */
        if (xQueueReceive(canRxQueueSD, &rxMsg, portMAX_DELAY) == pdPASS) {

          if (!g_grabacion_activa) {
              continue;
            }
            
            memcpy(&canValue, rxMsg.data, sizeof(float));

            const char *label = "UNKNOWN";
            if      (rxMsg.header.StdId == 0x101) label = "RMS";
            else if (rxMsg.header.StdId == 0x102) label = "CREST";
            else if (rxMsg.header.StdId == 0x103) label = "PEAK";
            else if (rxMsg.header.StdId == 0x104) label = "TEMP";
            else if (rxMsg.header.StdId == 0x105) label = "CURRENT";
            else if (rxMsg.header.StdId == 0x106) label = "SPEED";

            char linea_local[128];
            int len = sprintf(linea_local, "%lu,%s,0x%03lX,%.3f\n", 
                              osKernelSysTick(), label, rxMsg.header.StdId, canValue);

            if ((sd_block_idx + len) < 512) {
                memcpy(&sd_buffers[active_buffer][sd_block_idx], linea_local, len);
                sd_block_idx += len;
            } 
            else {
                buffer_ready_record = active_buffer;
                bytes_ready_record = sd_block_idx;

                active_buffer = !active_buffer; 
                memcpy(&sd_buffers[active_buffer][0], linea_local, len);
                sd_block_idx = len;

                xTaskNotifyGive(SDWriterHandle); 
            }
            
            contador_mensajes++;
 
            if (contador_mensajes >= MAX_DATOS_A_GRABAR)
            {
                g_grabacion_activa = 0;
 
                if (sd_block_idx > 0)
                {
                    buffer_ready_record = active_buffer;
                    bytes_ready_record  = sd_block_idx;
                    sd_block_idx = 0;
                    xTaskNotifyGive(SDWriterHandle);
                }
 
                printf("[SD] Limite de %lu datos alcanzado. Grabacion detenida.\r\n",
                       (unsigned long)MAX_DATOS_A_GRABAR);
            }
        }
    }
}

void StartSDWriterTask(void const * argument)
{
    FRESULT res;
    UINT bw;
    static FATFS fs;         
    static FIL file;  
    printf("\r\n--- Iniciando Nodo de Registro CAN -> SD ---\r\n");

    /* Montar la SD */
    res = f_mount(&fs, USERPath, 1);
    if (res != FR_OK) {
        printf("Error al montar SD: %d. Reintentando...\r\n", res);
        osDelay(1000);
        res = f_mount(&fs, USERPath, 1);
    }

    if (res == FR_OK) {
        printf("¡SD montada con exito!\r\n");
        /* 2. Abrir el archivo en modo Append (añadir al final) */
        res = f_open(&file, "CAN_LOG.CSV", FA_OPEN_ALWAYS | FA_WRITE);
        f_lseek(&file, f_size(&file));
        f_printf(&file, "Timestamp(ms),Label,ID,Value\n"); // Encabezado CSV
        printf("Archivo CAN_LOG.CSV listo para grabar\r\n");                                                    

        for(;;) {
            // Forzar salida de sleep si entró
            if (CAN1->MSR & CAN_MSR_SLAK)
            {
                CAN1->MCR &= ~CAN_MCR_SLEEP;
                printf(">>> WAKEUP forzado\r\n");
            }
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // LED ON
        
            res = f_write(&file, sd_buffers[buffer_ready_record], bytes_ready_record, &bw); 
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Feedback visual
            if (res == FR_OK) {
                f_sync(&file); 
            }
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // LED OFF      
               
            if (!g_grabacion_activa){
              f_close(&file);
              printf("[SD] Archivo cerrado. Registro finalizado.\r\n");

              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

              vTaskSuspend(NULL);   // Fin de la tarea
            }
        }
    }
}