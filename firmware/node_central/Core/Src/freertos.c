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
QueueHandle_t canRxQueueSD; // Nombre definitivo de la cola

/* Private function prototypes -----------------------------------------------*/
void StartSDRecord(void const * argument);
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

      if (res == FR_OK) {
          f_lseek(&file, f_size(&file));
          f_printf(&file, "Timestamp(ms),Label,ID,Value\n"); // Encabezado CSV
          printf("Archivo CAN_LOG.CSV listo para grabar\r\n");

          /* --- Bucle Principal de Consumo --- */
          for(;;) {
            // Forzar salida de sleep si entró
                if (CAN1->MSR & CAN_MSR_SLAK)
                {
                    CAN1->MCR &= ~CAN_MCR_SLEEP;
                    printf(">>> WAKEUP forzado\r\n");
                }
              /* 3. Bloquear la tarea hasta recibir un mensaje CAN */
              //printf("Esperando CAN...\r\n");
              if (xQueueReceive(canRxQueueSD, &rxMsg, portMAX_DELAY) == pdPASS) {
                  
                  // Extraer el valor float de los datos CAN
                  memcpy(&canValue, rxMsg.data, sizeof(float));

                  // Identificar el Label según el ID
                  //printf("mensaje CAN recibido\r\n");
                  const char *label = "UNKNOWN";
                  if      (rxMsg.header.StdId == 0x101) label = "RMS";
                  else if (rxMsg.header.StdId == 0x102) label = "CREST";
                  else if (rxMsg.header.StdId == 0x103) label = "PEAK";
                  else if (rxMsg.header.StdId == 0x104) label = "TEMP";
                  else if (rxMsg.header.StdId == 0x105) label = "CURRENT";
                  else if (rxMsg.header.StdId == 0x106) label = "SPEED";
                  //printf(">> [RX] %s (ID: 0x%03lX) -> Val: %.3f\r\n", 
                  //        label, rxMsg.header.StdId, canValue);
                  // Formatear línea CSV
                  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Feedback visual
                  int len = sprintf(buffer, "%lu,%s,0x%03lX,%.3f\n", 
                                    osKernelSysTick(), label, rxMsg.header.StdId, canValue);
                

                  // 4. Escribir en la SD
                  res = f_write(&file, buffer, len, &bw);
                  
                  if (res == FR_OK) {
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