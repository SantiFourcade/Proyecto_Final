/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ff.h"
#include "fatfs.h"
#include "fatfs_sd.h"
#include "gpio.h"
#include "spi.h"
#include "usart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SD_CS_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define SD_CS_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId loggerHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartLogger(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of logger */
  osThreadDef(logger, StartLogger, osPriorityNormal, 0, 512);
  loggerHandle = osThreadCreate(osThread(logger), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartLogger */
/**
  * @brief  Function implementing the logger thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartLogger */
void StartLogger(void const * argument)
{
  /* USER CODE BEGIN StartLogger */
  FATFS fs;
  FIL file;
  FRESULT res;
  UINT bw;
  char *logMsg = "Hello SD con driver nuevo\r\n";

  printf("\r\n--- Sistema de Log SD con Drivers de eziya ---\r\n");

  /* 1. BUCLE DE MONTAJE INFINITO */
  while (1) 
  {
      printf("Intentando montar SD (f_mount)... ");

      // Intentamos montar. f_mount llamará internamente a SD_disk_initialize
      // El parámetro '1' fuerza la inicialización física inmediata.
      res = f_mount(&fs, USERPath, 1);

      if (res == FR_OK) 
      {
          printf("¡EXITO! Sistema de archivos reconocido.\r\n");
          break; // Salimos del bucle
      } 
      else 
      {
          // Si da Error 1 o 3, el parpadeo del LED indica que seguimos intentando
          printf("FALLO (Código FatFS: %d). Reintentando en 2s...\r\n", res);
          HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); 
          osDelay(2000); 
      }
  }

  /* 2. APERTURA DE ARCHIVO */
  // Usamos FA_OPEN_APPEND si tu versión de FatFS lo soporta, 
  // si no, FA_OPEN_ALWAYS | FA_WRITE es lo más seguro.
  printf("Abriendo 'test.txt'...\r\n");
  res = f_open(&file, "test.txt", FA_OPEN_ALWAYS | FA_WRITE);
  
  if (res != FR_OK)
  {
      printf("Error crítico al abrir archivo: %d\r\n", res);
      while(1) // Bloqueo por error de apertura (parpadeo rápido)
      {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(100);
      }
  }

  /* 3. BUCLE DE ESCRITURA */
  // Movemos el puntero al final del archivo por si ya tenía datos
  f_lseek(&file, f_size(&file));
  printf("Escribiendo datos cada 1 segundo...\r\n");

  for (;;)
  {
      // Escribimos el mensaje
      res = f_write(&file, logMsg, strlen(logMsg), &bw);
      
      if (res == FR_OK && bw > 0)
      {
          // Sincronizamos para asegurar que los datos se guarden físicamente
          f_sync(&file);
          printf("Dato guardado: %d bytes\r\n", bw);
      }
      else
      {
          printf("Error de escritura física: %d\r\n", res);
      }

      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Flash de actividad
      osDelay(1000);
  }
  /* USER CODE END StartLogger */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

