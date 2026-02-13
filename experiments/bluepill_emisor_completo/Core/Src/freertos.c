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
#include "queue.h"

#include "adxl345.h"

#include "i2c.h"
#include "usart.h"
#include "adc.h"

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

QueueHandle_t rawQueue;
QueueHandle_t canQueue;

//osMessageQId rawQueueHandle;
//osMessageQId canQueueHandle;

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

  /* definition and creation of canQueue */

  rawQueue = xQueueCreate(2, sizeof(raw_data_t));
  canQueue = xQueueCreate(5, sizeof(data_frame_t));

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

// Lee los datos en crudo del acelerometro y del LM35 y copia una ventana de datos a una cola
// para luego calcular los features mas importantes en otra tarea
void StartAcquireTask(void const * argument)
{
    printf("Acquire task started\r\n");

    static raw_data_t raw;
    uint16_t idx = 0;

    for(;;)
    {
        /* Acelerómetro datos crudos */
        ADXL345_ReadXYZ(&raw.accel.ax[idx],
                        &raw.accel.ay[idx],
                        &raw.accel.az[idx]);

        idx++;

        /* ADC LM35 datos crudo */
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
        raw.temp = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);

        if (idx >= WINDOW_SIZE)
        {
            xQueueSend(rawQueue, &raw, portMAX_DELAY);
            idx = 0;
        }

        osDelay(2);
    }
}


/* USER CODE BEGIN Header_StartPackageTask */
/**
* @brief Function implementing the package thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartPackageTask */

// Recibe los datos de la cola y calcula los features mas importantes y los copia a otra cola
// para realizar en envío CAN en otra tarea
void StartPackageTask(void const * argument)
{

    printf("Package task started\r\n");

    raw_data_t raw;
    data_frame_t frame;

    for(;;)
    {
        xQueueReceive(rawQueue, &raw, portMAX_DELAY);

        /* Features vibración */
        Features_ComputeRMSPeak(&raw.accel, &frame.vib);

        /* Conversión temperatura */
        frame.temperature = ADC_To_Temp(raw.temp);

        xQueueSend(canQueue, &frame, portMAX_DELAY);
    }
}


/* USER CODE BEGIN Header_StartTxCAN */
/**
* @brief Function implementing the txCAN thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTxCAN */

// Recibe los features provenientes de la cola y los envía por CAN
// La temperatura se envía cada 20 veces menos que los datos de vibración dado que varía menos
void StartTxCAN(void const * argument)
{
    data_frame_t frame;
    uint32_t temp_cnt = 0;

    printf("TxCAN task started\r\n");

    for (;;)
    {
        xQueueReceive(canQueue, &frame, portMAX_DELAY);

        /* Vibración siempre */
        CAN_SendFloat(0x101, frame.vib.rms);
        //printf("[CAN TX] RMS   ID=0x101  Val=%.2f\r\n", frame.vib.rms);

        CAN_SendFloat(0x102, frame.vib.crest);
        //printf("[CAN TX] CREST ID=0x102  Val=%.2f\r\n", frame.vib.crest);

        CAN_SendFloat(0x103, frame.vib.peak);
        //printf("[CAN TX] PEAK  ID=0x103  Val=%.2f\r\n", frame.vib.peak);


        /* Temperatura cada 20 frames */
        temp_cnt++;

        if (temp_cnt >= 20)
        {
            CAN_SendFloat(0x104, frame.temperature);
            printf("[TEMP] %.2f C\r\n", frame.temperature);

            temp_cnt = 0;
        }
    }
}




/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

