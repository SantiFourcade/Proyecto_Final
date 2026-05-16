/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications (Merged Version)
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
#include "can.h"
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
extern volatile uint8_t flag_pulso;
extern volatile uint32_t diff_ticks;
extern volatile uint8_t new_read;
/* USER CODE END Variables */

osThreadId acquireHandle;
osThreadId packageHandle;
osThreadId txCANHandle;
//osThreadId loggerHandle;
osThreadId pulsoDebugHandle;

QueueHandle_t rawQueue;
QueueHandle_t canQueue;

extern CAN_HandleTypeDef hcan;
//osMessageQId rawQueueHandle;
//osMessageQId canQueueHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartAcquireTask(void const * argument);
void StartPackageTask(void const * argument);
void StartTxCAN(void const * argument);
void Pulso_Debug(void const * argument);
//void StartLogger(void const * argument);

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
  rawQueue = xQueueCreate(2, sizeof(raw_data_t));
  canQueue = xQueueCreate(5, sizeof(data_frame_t));

  configASSERT(rawQueue);
  configASSERT(canQueue);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  
  /* Acquire Task */
  osThreadDef(acquire, StartAcquireTask, osPriorityNormal, 0, 256);
  acquireHandle = osThreadCreate(osThread(acquire), NULL);

  /* Package Task */
  osThreadDef(package, StartPackageTask, osPriorityNormal, 0, 256);
  packageHandle = osThreadCreate(osThread(package), NULL);

  /* TxCAN Task */
  osThreadDef(txCAN, StartTxCAN, osPriorityNormal, 0, 512);
  txCANHandle = osThreadCreate(osThread(txCAN), NULL);

  /* Logger Task (from second code) */
  //osThreadDef(logger, StartLogger, osPriorityNormal, 0, 512);
  //loggerHandle = osThreadCreate(osThread(logger), NULL);

  /* Pulso Debug Task */
  osThreadDef(pulsoDebug, Pulso_Debug, osPriorityLow, 0, 256);
  pulsoDebugHandle = osThreadCreate(osThread(pulsoDebug), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */
}

/* --- Task Implementations --- */

#define N 500
volatile uint16_t buffer[N];
volatile uint16_t i = 0;

void StartAcquireTask(void const * argument)
{
    printf("Acquire task started\r\n");

    static raw_data_t raw;
    uint16_t idx = 0;

    /* Variables para velocidad */
    static uint32_t acumulador_ticks = 0;
    static uint8_t contador_pulsos = 0;
    float rpm = 0.0f;

    for(;;)
    {
    
        // ACELERÓMETRO
        ADXL345_ReadXYZ(&raw.accel.ax[idx],
                        &raw.accel.ay[idx],
                        &raw.accel.az[idx]);
        // printf("%d,%d,%d\r\n",
        //     raw.accel.ax[idx],
        //     raw.accel.ay[idx],
        //     raw.accel.az[idx]);
        // TEMPERATURA (LM35)
        //  ADC_Select_Channel(ADC_CHANNEL_0);
        //  HAL_Delay(2);
        //  HAL_ADC_Start(&hadc1);
        //  HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
        //  raw.temp = HAL_ADC_GetValue(&hadc1);
        //  printf("TEMP RAW = %u\r\n", raw.temp);
        //  HAL_ADC_Stop(&hadc1);

        //CORRIENTE 
        // ADC_Select_Channel(ADC_CHANNEL_1);
        // HAL_ADC_Start(&hadc1);
        // HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
        // raw.current[idx] = HAL_ADC_GetValue(&hadc1);
        // printf("CURRENT RAW = %d\r\n", raw.current[idx]);        
        // HAL_ADC_Stop(&hadc1);

        //VELOCIDAD (RPM)
        if (new_read)
        {
            taskENTER_CRITICAL();
            uint32_t current_ticks = diff_ticks;
            new_read = 0;
            taskEXIT_CRITICAL();

            acumulador_ticks += current_ticks;
            contador_pulsos++;

            if (contador_pulsos >= 8)
            {
                float promedio_ticks = (float)acumulador_ticks / 8.0f;

                /* Fórmula RPM */
                rpm = 7500000.0f / promedio_ticks;

                acumulador_ticks = 0;
                contador_pulsos = 0;

            }
        }

        printf("%d,%d,%d,%.2f\r\n",
       raw.accel.ax[idx],
       raw.accel.ay[idx],
       raw.accel.az[idx],
       rpm);
        raw.speed = rpm;

        idx++;

        if (idx >= WINDOW_SIZE)
        {
            xQueueSend(rawQueue, &raw, portMAX_DELAY);
            idx = 0;
        }

        osDelay(100); 
    }
}

// void StartLogger(void const * argument) {
//      /* Variables locales para el cálculo */ 
//      uint32_t acumulador_ticks = 0; 
//      uint8_t contador_pulsos = 0; 
//      float rpm_suave = 0.0f; 
//      for(;;) { 

//         if (new_read) { 

//             taskENTER_CRITICAL(); 
//             uint32_t current_ticks = diff_ticks; 
//             new_read = 0; taskEXIT_CRITICAL(); // 8 pulsos para promediar la vuelta completa 
//             acumulador_ticks += current_ticks; 
//             contador_pulsos++; 

//             if (contador_pulsos >= 8) { 

//                 // 2. Calculamos el promedio de la vuelta 
//                 float promedio_ticks = (float)acumulador_ticks / 8; 

//                 // 3. RPM = 60,000,000 / (promedio * 8) 
//                 // O simplificado: 7,500,000 / promedio 
//                 rpm_suave = 7500000.0f / promedio_ticks; 

//                 // 4. Limpiamos para la siguiente vuelta 
//                 acumulador_ticks = 0; contador_pulsos = 0; 

//                 // 5. Imprimir el dato limpio para el modelo de ML 
//                 printf("%.2f\n", rpm_suave); 
//             } 
//         } 

//                 // Un delay pequeño para no saturar la CPU 
//                 vTaskDelay(pdMS_TO_TICKS(5)); 
//     }
// }


void StartPackageTask(void const * argument)
{
    printf("Package task started\r\n");
    raw_data_t raw;
    data_frame_t frame;

    for(;;)
    {
        xQueueReceive(rawQueue, &raw, portMAX_DELAY);

        /* Features vibración */
        // Features_ComputeRMSPeak(&raw.accel, &frame.vib);
        // printf("RMS  Val=%.2f\r\n", frame.vib.rms);
        // printf("PEAK  Val=%.2f\r\n", frame.vib.peak);
        // printf("CREST  Val=%.2f\r\n", frame.vib.crest);

        /* Conversión temperatura */
        //frame.temperature = ADC_To_Temp(raw.temp);
        //printf("TEMP  Val=%.2f\r\n", frame.temperature);

        /* Conversión corriente */
        //frame.current = ADC_To_Current(raw.current);
        //printf("CURRENT  Val=%.2f\r\n", frame.current);


        //frame.speed=raw.speed;
        //printf("SPEED  Val=%.2f\r\n", frame.speed);


        //xQueueSend(canQueue, &frame, portMAX_DELAY);
    }
}

void StartTxCAN(void const * argument)
{
    data_frame_t frame;
    uint32_t temp_cnt = 0;
    printf("TxCAN task started\r\n");

    for (;;)
    {
        // xQueueReceive(canQueue, &frame, portMAX_DELAY);

        // /* Vibración siempre */
        // CAN_SendFloat(0x101, frame.vib.rms);
        // //printf("[CAN TX] RMS   ID=0x101  Val=%.2f\r\n", frame.vib.rms);
        
        // uint32_t tx_mailboxes = HAL_CAN_GetTxMailboxesFreeLevel(&hcan);

        // CAN_SendFloat(0x102, frame.vib.crest);
        // //printf("[CAN TX] CREST ID=0x102  Val=%.2f\r\n", frame.vib.crest);

        // CAN_SendFloat(0x103, frame.vib.peak);
        // //printf("[CAN TX] PEAK  ID=0x103  Val=%.2f\r\n", frame.vib.peak);

        // CAN_SendFloat(0x105, frame.current);
        // //printf("[CAN TX] CURRENT  ID=0x105  Val=%.2f\r\n", frame.current);

        // CAN_SendFloat(0x106, frame.speed);
        // //printf("[CAN TX] SPEED  ID=0x106  Val=%.2f\r\n", frame.speed);

        // /* Temperatura cada 20 frames */
        // temp_cnt++;
        // if (temp_cnt >= 20)
        // {
        //     CAN_SendFloat(0x104, frame.temperature);
        //     //printf("[CAN TX] TEMP  ID=0x104  Val=%.2f C\r\n", frame.temperature);
        //     temp_cnt = 0;
        // }
    }
}


void Pulso_Debug(void const * argument)
{
    printf("-Tarea Pulso-\r\n");
    for(;;)
    {
        if (flag_pulso)
        {
            flag_pulso = 0;
            printf("-PULSO-\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* USER CODE BEGIN Application */

/* USER CODE END Application */