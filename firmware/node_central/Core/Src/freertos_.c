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
/* USER CODE BEGIN Variables */
extern CAN_HandleTypeDef hcan; // Para monitorear errores en la tarea
/* USER CODE END Variables */

osThreadId SDRecordHandle;
osThreadId ButtonTaskHandle;
QueueHandle_t canRxQueueSD; // Nombre definitivo de la cola

/* Private function prototypes -----------------------------------------------*/
void StartSDRecord(void const * argument);
void StartButtonTask(void const * argument);
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
  /* Definición de la tarea de grabación SD con prioridad normal y stack de 1024 */
  osThreadDef(SDRecord, StartSDRecord, osPriorityNormal, 0, 1024);
  SDRecordHandle = osThreadCreate(osThread(SDRecord), NULL);

  // Tarea de Monitoreo de Botón (Polling)
  osThreadDef(ButtonTask, StartButtonTask, osPriorityAboveNormal, 0, 128);
  ButtonTaskHandle = osThreadCreate(osThread(ButtonTask), NULL);

}

/* USER CODE BEGIN Header_StartButtonTask */
/**
  * @brief  Tarea encargada de monitorear el estado del botón.
  * @param  argument: Not used
  * @retval None
  */
 
/* USER CODE END Header_StartButtonTask */

void StartButtonTask(void const * argument)
{
  uint8_t button_state = 0; // 0=idle, 1=debouncing, 2=pressed
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
        // Si pasaron 50ms y sigue presionado, es una pulsación real
        if ((osKernelSysTick() - press_tick) > 50) 
        {
          button_state = 2; 
          printf("¡Boton validado! Iniciando tarea...\r\n");
          
          // ACTIVAMOS LA TAREA (Usando una notificación de tarea)
          xTaskNotifyGive(SDRecordHandle); 
        }
      }
    } 
    else 
    {
      button_state = 0; // Se soltó el botón
    }

    /* Revisamos el botón cada 20ms. 
       Esto es el "debounce" natural por software. */
    osDelay(20); 
  }
}

/* USER CODE BEGIN Header_StartSDRecord */
/**
  * @brief  Tarea encargada de recibir datos de CAN y grabarlos en la SD.
  * @param  argument: Not used
  * @retval None
  */
 
/* USER CODE END Header_StartSDRecord */
void StartSDRecord(void const * argument)
{
    FATFS fs;         
    FIL file;          
    FRESULT res;       
    UINT bw;           
    char buffer[80];   

    CAN_Message_t rxMsg;
    float canValue;
    int syncCounter = 0;

    uint32_t startTime = 0;
    uint32_t samplesCount = 0;

    ulTaskNotifyTake(pdTRUE, 0);

    printf("\r\n Iniciando Nodo de Registro CAN -> SD \r\n");

    // Montar la SD
    res = f_mount(&fs, USERPath, 1);
    if (res != FR_OK) {
        printf("Error al montar SD: %d. Reintentando...\r\n", res);
        osDelay(1000);
        res = f_mount(&fs, USERPath, 1);
    }   

    if (res == FR_OK) {
        printf("SD montada con exito\r\n");

        // Abrir el archivo en modo Append (añadir al final)
        res = f_open(&file, "CAN_LOG.CSV", FA_OPEN_ALWAYS | FA_WRITE);

        if (res == FR_OK) {
            f_lseek(&file, f_size(&file));
            f_printf(&file, "Timestamp(ms),Label,ID,Value\n");
            printf("Archivo CAN_LOG.CSV listo para grabar\r\n");

            // Bucle Principal de Consumo
            for(;;) {
                // Esperamos a que la tarea sea activada por el botón
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                // Bloquear la tarea hasta recibir un mensaje CAN
                samplesCount = 0;
                syncCounter = 0;
                startTime = osKernelSysTick();

                while ((osKernelSysTick() - startTime < 10000) && (samplesCount < 500)){
                    printf("Esperando CAN... (Tarea activa por boton)\r\n");
                    if (xQueueReceive(canRxQueueSD, &rxMsg, 1000) == pdPASS) {
                        
                        // Extraer el valor float de los datos CAN
                        memcpy(&canValue, rxMsg.data, sizeof(float));

                        // Identificar el Label según el ID
                        const char *label = "UNKNOWN";
                        if      (rxMsg.header.StdId == 0x101) label = "RMS";
                        else if (rxMsg.header.StdId == 0x102) label = "CREST";
                        else if (rxMsg.header.StdId == 0x103) label = "PEAK";
                        else if (rxMsg.header.StdId == 0x104) label = "TEMP";
                        else if (rxMsg.header.StdId == 0x105) label = "CURRENT";
                        else if (rxMsg.header.StdId == 0x106) label = "SPEED";
                        printf(">> [RX] %s (ID: 0x%03lX) -> Val: %.3f\r\n", 
                                label, rxMsg.header.StdId, canValue);
                        // Formatear línea CSV
                        int len = sprintf(buffer, "%lu,%s,0x%03lX,%.3f\n", 
                                            osKernelSysTick(), label, rxMsg.header.StdId, canValue);

                        // Escribir en la SD
                        res = f_write(&file, buffer, len, &bw);
                        
                        if (res == FR_OK) {
                            samplesCount++;
                            // Sincronizar cada 10 mensajes para no saturar el bus SPI
                            if (++syncCounter >= 10) {
                                f_sync(&file);
                                syncCounter = 0;
                                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Feedback visual
                            }
                            
                            // Log por consola (opcional para debug)
                            // printf("[SD] Grado: %s Val: %.3f\r\n", label, canValue);
                        } else {
                            printf("Error crítico de escritura en SD!\r\n");
                            break; 
                        }
                    }
                }

                // Monitoreo de errores de bus (opcional)
                uint32_t err = HAL_CAN_GetError(&hcan);
                if(err != HAL_CAN_ERROR_NONE) {
                    printf("CAN Bus Error: 0x%lx\r\n", err);
                }
            }
            f_close(&file); 
        } else {
          printf("No se pudo abrir CAN_LOG.CSV: %d\r\n", res);
        }
  }

  /* Si falla el montaje o la apertura, la tarea queda en bucle de error */
  printf("Tarea SD terminada por error.\r\n");
  for(;;) {
      osDelay(5000);
  }
}
