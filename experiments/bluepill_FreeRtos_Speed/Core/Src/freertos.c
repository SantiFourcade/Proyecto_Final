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
extern volatile uint32_t contador_pulsos;
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
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriodo = pdMS_TO_TICKS(100); // Ventana de 100ms
  uint32_t pulsos_actuales = 0;
  uint32_t pulsos_anteriores = 0;
  for(;;) {    
      vTaskDelayUntil(&xLastWakeTime, xPeriodo);
      taskENTER_CRITICAL();
      pulsos_actuales = contador_pulsos;
      taskEXIT_CRITICAL();
      uint32_t delta_pulsos = pulsos_actuales - pulsos_anteriores;
      pulsos_anteriores = pulsos_actuales;
      // Cálculo: 
      // RPM = (delta_pulsos / ranuras) * (60,000 / tiempo_ms)
      // Para 8 ranuras y 100ms:
      // RPM = (delta_pulsos / 8) * (600) -> delta_pulsos * 75
      float rpm_actual = (float)delta_pulsos * 75.0f;
      printf("RPM: %.2f | Delta: %lu | Total: %lu\n", rpm_actual, (unsigned long)delta_pulsos, (unsigned long)pulsos_actuales);      
     //printf("Pulsos: %lu\n", (unsigned long)contador_pulsos);
     //vTaskDelay(xPeriodo);
  }
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

