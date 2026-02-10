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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ff.h"
#include "ff_gen_drv.h"
#include "fatfs.h"
#include "fatfs_sd.h"
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

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
  FATFS fs;         // Objeto del sistema de archivos
  FIL file;          // Objeto del archivo
  FRESULT res;       // Resultado de las operaciones FatFS
  UINT bw;           // Bytes escritos
  char buffer[64];   // Buffer para mensajes
  int contador = 0;

  printf("\r\n--- Iniciando Tarea SD ---\r\n");

  /* 1. Montar la SD */
  // El '1' al final fuerza el montaje inmediato (llama a SD_disk_initialize)
  res = f_mount(&fs, USERPath, 1);

  if (res != FR_OK) {
      printf("Error al montar SD: %d\r\n", res);
      // Si falla, podrías intentar un reintento tras un delay
      osDelay(1000);
      res = f_mount(&fs, USERPath, 1);
  }

  if (res == FR_OK) {
      printf("¡SD montada con éxito!\r\n");

      /* 2. Abrir o Crear el archivo */
      // FA_OPEN_ALWAYS: Abre si existe, crea si no.
      // FA_WRITE: Acceso de escritura.
      res = f_open(&file, "LOG.TXT", FA_OPEN_ALWAYS | FA_WRITE);

      if (res == FR_OK) {
          /* 3. Ir al final del archivo para hacer un 'Append' */
          f_lseek(&file, f_size(&file));
          
          f_printf(&file, "--- Nueva Sesion de Log ---\n");

          /* Bucle infinito de escritura */
          for(;;) {
              contador++;
              sprintf(buffer, "Log numero: %d | Status: OK\r\n", contador);

              res = f_write(&file, buffer, strlen(buffer), &bw);
              
              if (res == FR_OK) {
                  // f_sync asegura que los datos se guarden físicamente
                  // sin tener que cerrar el archivo constantemente.
                  f_sync(&file);
                  
                  // Opcional: Toggle LED para feedback visual de escritura
                  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
              } else {
                  printf("Error escribiendo: %d\r\n", res);
                  break; // Salir si hay error físico (ej. sacaron la SD)
              }

              osDelay(1000); // Esperar 1 segundo
          }
          
          f_close(&file); // Cerrar si sale del bucle
      } else {
          printf("No se pudo abrir el archivo: %d\r\n", res);
      }
  }

  /* Si llegamos aquí es por un error crítico */
  for(;;) {
      osDelay(1000);
  }
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

