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
#include "math.h"

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
extern volatile uint16_t adc_buffer_corriente[100]; // Buffer para 100 muestras (100ms)
extern volatile uint8_t buffer_listo; // Bandera para la tarea
/* USER CODE END Variables */

osThreadId acquireHandle;
osThreadId packageHandle;
osThreadId txCANHandle;
//osThreadId pulsoDebugHandle;
//osThreadId loggerHandle;
osThreadId samplerHandle;

QueueHandle_t rawQueue;
QueueHandle_t canQueue;

extern CAN_HandleTypeDef hcan;
//osMessageQId rawQueueHandle;
//osMessageQId canQueueHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
float Procesar_Corriente_RMS(void);
/* USER CODE END FunctionPrototypes */

void StartAcquireTask(void const * argument);
void StartPackageTask(void const * argument);
void StartTxCAN(void const * argument);
//void Pulso_Debug(void const * argument);
//void StartLogger(void const * argument);
void StartSampler(void const * argument);
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
  osThreadDef(acquire, StartAcquireTask, osPriorityNormal, 0, 512);
  acquireHandle = osThreadCreate(osThread(acquire), NULL);

  /* Package Task */
  osThreadDef(package, StartPackageTask, osPriorityNormal, 0, 256);
  packageHandle = osThreadCreate(osThread(package), NULL);

  /* TxCAN Task */
  osThreadDef(txCAN, StartTxCAN, osPriorityNormal, 0, 512);
  txCANHandle = osThreadCreate(osThread(txCAN), NULL);

  // osThreadDef(logger, StartLogger, osPriorityNormal, 0, 512);
  //loggerHandle = osThreadCreate(osThread(logger), NULL);

  osThreadDef(sampler, StartSampler, osPriorityHigh, 0, 128);
  samplerHandle = osThreadCreate(osThread(sampler), NULL);

//   /* Pulso Debug Task */
//   osThreadDef(pulsoDebug, Pulso_Debug, osPriorityLow, 0, 256);
//   pulsoDebugHandle = osThreadCreate(osThread(pulsoDebug), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */
}

/* --- Task Implementations --- */

#define N 500
volatile uint16_t buffer[N];
volatile uint16_t i = 0;


void StartSampler(void const * argument)
{
  /* USER CODE BEGIN StartSampler */
  /* Infinite loop */
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(1); // 1 ms exacto
  xLastWakeTime = xTaskGetTickCount();

  for(;;) {
      vTaskDelayUntil(&xLastWakeTime, xFrequency);
      HAL_ADC_Start_IT(&hadc1); // Disparo manual cada 1ms
  }
  /* USER CODE END StartSampler */
}
/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE BEGIN 4 */

/**
 * @brief Calcula la corriente RMS centrando la señal con el offset calibrado.
 * @retval Valor de corriente eficaz en Voltios (basado en VREF de 3.262V).
 */
float Procesar_Corriente_RMS(void) {
    float suma_cuadrados = 0.0f;        
    
    for(int i = 0; i < 100; i++) {
        // Restamos offset usando tus 3.262V reales (1.631V = 2048 ticks)
        int32_t centrado = (int32_t)adc_buffer_corriente[i] - 2048;
        suma_cuadrados += (float)(centrado * centrado);
    }
    
    float rms_raw = sqrtf(suma_cuadrados / 100.0f);
    return (rms_raw * (3.262f / 4096.0f)); 
}

float temp_filtrada = 0.0f;
float corriente_actual = 0.0f;

void StartAcquireTask(void const * argument)
{
    printf("Iniciando Adquisidor...\r\n");
    uint32_t acumulador_temp = 0;
    uint32_t cuenta_muestras_temp = 0;

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

        raw.speed = rpm;

        if (buffer_listo) {
            corriente_actual = Procesar_Corriente_RMS();
            buffer_listo = 0;

            HAL_ADC_Start(&hadc2);
            if(HAL_ADC_PollForConversion(&hadc2, 2) == HAL_OK) {
                acumulador_temp += HAL_ADC_GetValue(&hadc2);
                cuenta_muestras_temp++;
            }
            HAL_ADC_Stop(&hadc2);

            if (cuenta_muestras_temp >= 32) {
                float promedio_ticks = (float)acumulador_temp / 32.0f;
                float voltaje_mv = promedio_ticks * (3262.0f / 4096.0f);
                temp_filtrada = voltaje_mv / 10.0f;

                acumulador_temp = 0;
                cuenta_muestras_temp = 0;
            }
        } 

    //ENVIO A PYTHON
    // printf("%d,%d,%d,%.2f,%.2f,%.2f\r\n",
    //        raw.accel.ax[idx],
    //        raw.accel.ay[idx],
    //        raw.accel.az[idx],
    //        temp_filtrada,
    //        corriente_actual,
    //        rpm);

        idx++;

        if (idx >= WINDOW_SIZE)
        {
            xQueueSend(rawQueue, &raw, portMAX_DELAY);
            idx = 0;
        }

        osDelay(100); 
    }
}


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
        // printf("RMS  Val=%.2f\r\n", frame.vib.rms);
        // printf("PEAK  Val=%.2f\r\n", frame.vib.peak);
        // printf("CREST  Val=%.2f\r\n", frame.vib.crest);

        /* Conversión temperatura */
        frame.temperature = temp_filtrada;
        //printf("TEMP  Val=%.2f\r\n", frame.temperature);

        /* Conversión corriente */
        frame.current = corriente_actual;
        //printf("CURRENT  Val=%.2f\r\n", frame.current);

        frame.speed=raw.speed;
        //printf("SPEED  Val=%.2f\r\n", frame.speed);

        xQueueSend(canQueue, &frame, portMAX_DELAY);
    }
}

void StartTxCAN(void const * argument)
{
    data_frame_t frame;
    printf("TxCAN task started\r\n");

     for (;;)
     {
         xQueueReceive(canQueue, &frame, portMAX_DELAY);

         CAN_SendFloat(0x101, frame.vib.rms);   vTaskDelay(pdMS_TO_TICKS(10));
         //printf("[CAN TX] RMS   ID=0x101  Val=%.2f\r\n", frame.vib.rms);
        
         CAN_SendFloat(0x102, frame.vib.crest); vTaskDelay(pdMS_TO_TICKS(10));
        //printf("[CAN TX] CREST ID=0x102  Val=%.2f\r\n", frame.vib.crest);

         CAN_SendFloat(0x103, frame.vib.peak); vTaskDelay(pdMS_TO_TICKS(10));
        //printf("[CAN TX] PEAK  ID=0x103  Val=%.2f\r\n", frame.vib.peak);
        
        CAN_SendFloat(0x104, frame.temperature); vTaskDelay(pdMS_TO_TICKS(10));
        //printf("[CAN TX] TEMP  ID=0x104  Val=%.2f\r\n", frame.temperature);

         CAN_SendFloat(0x105, frame.current); vTaskDelay(pdMS_TO_TICKS(10));
        //printf("[CAN TX] CURRENT  ID=0x105  Val=%.2f\r\n", frame.current);

         CAN_SendFloat(0x106, frame.speed); vTaskDelay(pdMS_TO_TICKS(10));
        //printf("[CAN TX] SPEED  ID=0x106  Val=%.2f\r\n", frame.speed);
    }
}
/* USER CODE BEGIN Application */

/* USER CODE END Application */