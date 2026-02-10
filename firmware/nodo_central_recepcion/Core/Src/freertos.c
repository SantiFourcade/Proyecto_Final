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
//#include "FreeRTOS.h"
//#include "task.h"
//#include "main.h"
//#include "cmsis_os.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "queue.h"

#include "adxl345.h"

#include "i2c.h"
#include "usart.h"

#include "features.h"
#include "can_app.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>   

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
//typedef struct {
  //  int16_t ax;
  //  int16_t ay;
  //  int16_t az;
//} accel_msg_t;

//typedef struct {
//    accel_window_t accel;
//    uint16_t temp;
//} raw_data_t;

//typedef struct {
//    uint16_t rms;
//    uint16_t variance;
//    uint16_t temp;
//} feature_frame_t;

//typedef enum {
//    CAN_MSG_ACCEL,
//    CAN_MSG_FEATURE
//} can_msg_type_t;

//typedef struct {
//    can_msg_type_t type;
//    union {
//        accel_msg_t        accel;
//        feature_frame_t    feature;
//    };

QueueHandle_t canRxQueue;

//} can_msg_t;
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
osThreadId acquireHandle;
osThreadId packageHandle;
osThreadId txCANHandle;

osMessageQId rawQueueHandle;
osMessageQId canQueueHandle;

QueueHandle_t rawQueue;
QueueHandle_t canQueue;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartAcquireTask(void const * argument);
void StartPackageTask(void const * argument);
void StartTxCAN(void const * argument);

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

  /* Create the queue(s) */
  /* definition and creation of rawQueue */
  //osMessageQDef(rawQueue, 16, sizeof(raw_data_t));
  //rawQueueHandle = osMessageCreate(osMessageQ(rawQueue), NULL);

  /* definition and creation of canQueue */
  //osMessageQDef(canQueue, 16, sizeof(can_msg_t));
  //canQueueHandle = osMessageCreate(osMessageQ(canQueue), NULL);


  canRxQueue = xQueueCreate(8, sizeof(CAN_Message_t));

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */

  /* definition and creation of rxCAN */
xTaskCreate(
    CAN_Rx_Task,
    "CAN_RX",
    512,
    NULL,
    osPriorityNormal,
    NULL
);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}


/* USER CODE BEGIN Header_StartTxCAN */
/**
* @brief Function implementing the txCAN thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTxCAN */
void CAN_Rx_Task(void *argument)
{
    CAN_Message_t msg;
    float value;

    for (;;)
    {
        if (xQueueReceive(canRxQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            memcpy(&value, msg.data, sizeof(float));

            const char *label = "DATA";
            if (msg.header.StdId == 0x101) label = "RMS";
            else if (msg.header.StdId == 0x102) label = "CREST";
            else if (msg.header.StdId == 0x103) label = "PEAK";

            printf("[RX] %s (0x%03lX): %.3f\r\n",
                   label, msg.header.StdId, value);
        }
    }
}




/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

