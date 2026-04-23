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
extern volatile uint32_t diff_ticks;
extern volatile uint8_t new_read;
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
  /* Variables locales para el cálculo */
  uint32_t acumulador_ticks = 0;
  uint8_t contador_pulsos = 0;
  float rpm_suave = 0.0f;

  for(;;)
  {
    if (new_read) 
    {
        taskENTER_CRITICAL();
        uint32_t current_ticks = diff_ticks;
        new_read = 0;
        taskEXIT_CRITICAL();

        // 8 pulsos para promediar la vuelta completa
        acumulador_ticks += current_ticks;
        contador_pulsos++;

        if (contador_pulsos >= 8) 
        {
            // 2. Calculamos el promedio de la vuelta
            float promedio_ticks = (float)acumulador_ticks / 8;
            
            // 3. RPM = 60,000,000 / (promedio * 8) 
            // O simplificado: 7,500,000 / promedio
            rpm_suave = 7500000.0f / promedio_ticks;

            // 4. Limpiamos para la siguiente vuelta
            acumulador_ticks = 0;
            contador_pulsos = 0;

            // 5. Imprimir el dato limpio para el modelo de ML
            printf("%.2f\n", rpm_suave);
        }
    }

    // Un delay pequeño para no saturar la CPU
    vTaskDelay(pdMS_TO_TICKS(5)); 
  }

}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

