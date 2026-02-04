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


  rawQueue = xQueueCreate(2, sizeof(accel_window_t));
  canQueue = xQueueCreate(5, sizeof(vib_features_t));

  configASSERT(rawQueue);
  configASSERT(canQueue);
  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of acquire */
  osThreadDef(acquire, StartAcquireTask, osPriorityNormal, 0, 256);
  acquireHandle = osThreadCreate(osThread(acquire), NULL);

  /* definition and creation of package */
  osThreadDef(package, StartPackageTask, osPriorityNormal, 0, 256);
  packageHandle = osThreadCreate(osThread(package), NULL);

  /* definition and creation of txCAN */
  osThreadDef(txCAN, StartTxCAN, osPriorityNormal, 0, 512);
  txCANHandle = osThreadCreate(osThread(txCAN), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartAcquireTask */
/**
  * @brief  Function implementing the acquire thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartAcquireTask */
void StartAcquireTask(void const * argument)
{
  /* USER CODE BEGIN StartAcquireTask */

    printf("Acquire task started\r\n");

  /* Infinite loop */
  static accel_window_t window;
  uint16_t idx = 0;
  raw_data_t raw;
  for(;;)
  {
        // -------- ADC --------
    //HAL_ADC_Start(&hadc1);
    //HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    //raw.temp = HAL_ADC_GetValue(&hadc1);
    //HAL_ADC_Stop(&hadc1);

    // -------- Accel (placeholder) --------
    ADXL345_ReadXYZ(&window.ax[idx],
                    &window.ay[idx],
                    &window.az[idx]);
    idx++;
    if (idx >= WINDOW_SIZE)
    {
        //printf("RAW -> X:%d | Y:%d | Z:%d\r\n", 
          //  window.ax[idx-1], 
            //window.ay[idx-1], 
            //window.az[idx-1]);
        xQueueSend(rawQueue, &window, portMAX_DELAY);
        idx = 0;
    }
    
    osDelay(2); // ej 10 Hz temperatura
  }
  /* USER CODE END StartAcquireTask */
}

/* USER CODE BEGIN Header_StartPackageTask */
/**
* @brief Function implementing the package thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartPackageTask */
void StartPackageTask(void const * argument)
{
  /* USER CODE BEGIN StartPackageTask */

  printf("Package task started\r\n");

  static accel_window_t window;
  vib_features_t features;
  char uart_buf[128];
  /* Infinite loop */
  for(;;)
  {
  xQueueReceive(rawQueue, &window, portMAX_DELAY);
  Features_ComputeRMSPeak(&window, &features);

  //printf("FEATURES rms=%.2f crest=%.2f peak=%.2f\r\n",
    //      features.rms,
      //    features.crest,
        //  features.peak);

  xQueueSend(canQueue, &features, portMAX_DELAY);

  }
  /* USER CODE END StartPackageTask */
}

/* USER CODE BEGIN Header_StartTxCAN */
/**
* @brief Function implementing the txCAN thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTxCAN */
void StartTxCAN(void const * argument)
{
  vib_features_t features;

  printf("TxCAN task started\r\n");

  for (;;)
  {
    /* Espera features reales desde PackageTask */
    xQueueReceive(canQueue, &features, portMAX_DELAY);

    //printf("[CAN TX] RMS   ID=0x101  Val=%.2f\r\n", features.rms);
    CAN_SendFloat(0x101, features.rms);

    //printf("[CAN TX] CREST ID=0x102  Val=%.2f\r\n", features.crest);
    CAN_SendFloat(0x102, features.crest);

    //printf("[CAN TX] PEAK  ID=0x103  Val=%.2f\r\n", features.peak);
    CAN_SendFloat(0x103, features.peak);
  }
}



/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

