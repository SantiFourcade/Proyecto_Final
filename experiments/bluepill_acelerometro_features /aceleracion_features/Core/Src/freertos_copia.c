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
#include "adxl345.h"
#include "i2c.h"
#include "usart.h"
#include <stdint.h>
#include "features.h"
#include "can_app.h"
#include "usart.h"
#include <stdio.h>
#include "queue.h"

/* RTOS objects */
osThreadId accelTaskHandle;
osThreadId featureTaskHandle;
osThreadId canTaskHandle;

osMessageQId accelQueueHandle;
osMessageQId featureQueueHandle;

QueueHandle_t accelQueue;
QueueHandle_t featureQueue;

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

/* USER CODE END Variables */
osThreadId AccelTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartAccelTask(void const * argument);
void StartFeatureTask(void const * argument);
void StartCANTask(void const * argument);
/* USER CODE END FunctionPrototypes */


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
  //osMessageQDef(accelQueue, 2, accel_window_t);
  //accelQueueHandle = osMessageCreate(osMessageQ(accelQueue), NULL);

  //osMessageQDef(featureQueue, 2, vib_features_t);
  //featureQueueHandle = osMessageCreate(osMessageQ(featureQueue), NULL);

  accelQueue = xQueueCreate(2, sizeof(accel_window_t));
  featureQueue = xQueueCreate(2, sizeof(vib_features_t));

  configASSERT(accelQueue);
  configASSERT(featureQueue);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of AccelTask */

  /* USER CODE BEGIN RTOS_THREADS */
 osThreadDef(accelTask, StartAccelTask, osPriorityNormal, 0, 256);
 accelTaskHandle = osThreadCreate(osThread(accelTask), NULL);

 osThreadDef(featureTask, StartFeatureTask, osPriorityAboveNormal, 0, 1024);
 featureTaskHandle = osThreadCreate(osThread(featureTask), NULL);

 osThreadDef(canTask, StartCANTask, osPriorityLow, 0, 256);
 canTaskHandle = osThreadCreate(osThread(canTask), NULL);
  /* USER CODE END RTOS_THREADS */

}



/* USER CODE BEGIN Header_StartAccelTask */
/**
* @brief Function implementing the AccelTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartAccelTask */
void StartAccelTask(void const * argument)
{
    static accel_window_t window;
    uint16_t idx = 0;
    //printf("Accel task started\r\n");
    for (;;)
    {
        ADXL345_ReadXYZ(&window.ax[idx],
                        &window.ay[idx],
                        &window.az[idx]);
        idx++;
        if (idx >= WINDOW_SIZE)
        {
            printf("RAW -> X:%d | Y:%d | Z:%d\r\n", 
                      window.ax[idx-1], 
                      window.ay[idx-1], 
                      window.az[idx-1]);
            //printf("Accel window ready\r\n");
            xQueueSend(accelQueue, &window, portMAX_DELAY);
            idx = 0;
        }
        osDelay(2);
    }
}




void StartFeatureTask(void const * argument)
{
    accel_window_t window;
    vib_features_t features;
    char uart_buf[128];

    printf("Feature task started\r\n");

    for (;;)
    {
        xQueueReceive(accelQueue, &window, portMAX_DELAY);

        //printf("Feature: window received\r\n");

        Features_ComputeRMSPeak(&window, &features);

        //snprintf(uart_buf, sizeof(uart_buf),
          //       "RMS=%.3f | PEAK=%.3f | CREST=%.3f\r\n",
             //  features.rms,
              //   features.peak,
                // features.crest);

        //HAL_UART_Transmit(&huart1,
          //                (uint8_t *)uart_buf,
            //              strlen(uart_buf),
              //            HAL_MAX_DELAY);

        xQueueSend(featureQueue, &features, portMAX_DELAY);
    }
}



void StartCANTask(void const * argument)
{
    vib_features_t features;
    printf("CAN task started\r\n");
    for (;;)
    {
        xQueueReceive(featureQueue, &features, portMAX_DELAY);
        printf("CAN TX: RMS=%.3f CREST=%.3f PEAK=%.3f\r\n",
               features.rms,
               features.crest,
               features.peak);
        CAN_SendFloat(0x101, features.rms);
        CAN_SendFloat(0x102, features.crest);
        CAN_SendFloat(0x103, features.peak);
    }
}




/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

