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
  /* Infinite loop */
  FATFS fs;
  FIL file;
  UINT bw;
// 1. Asegurar CS en alto y dar tiempo de estabilización
  SD_CS_HIGH();
  osDelay(100); 

  // 2. Enviar 10-20 bytes dummy (0xFF) con CS ALTO para entrar en modo SPI
  uint8_t dummy = 0xFF;
  for(int i=0; i<20; i++) {
      HAL_SPI_Transmit(&hspi1, &dummy, 1, 10);
  }

  printf("Intentando montar SD...\r\n");

  // 3. Mount con '1' para forzar inicialización inmediata
  FRESULT res = f_mount(&fs, USERPath, 1);
  
  if (res != FR_OK) {
      printf("Error f_mount: %d\r\n", res);
      // Si el error es 13 (FR_NO_FILESYSTEM), intenta f_mkfs o revisa el formato
      while(1) { 
          HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); 
          osDelay(1000); 
      }
  }

  printf("SD Montada con éxito!\r\n");
  /* Open file */
  if (f_open(&file, "test.txt", FA_OPEN_ALWAYS | FA_WRITE) != FR_OK)
  {
      while(1){
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(200);
        printf("Error al abrir\r\n");
      };   // error open
  }

  /* Append */
  f_lseek(&file, f_size(&file));
  for (;;)
  {
      f_write(&file, "Hello SD\r\n", 10, &bw);
      f_sync(&file);
      osDelay(1000);
  }
  /* USER CODE END StartLogger */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

