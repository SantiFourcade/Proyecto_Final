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
#include "adc.h"
#include "stdio.h"
#include "math.h"

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
extern volatile uint16_t adc_buffer_corriente[100]; // Buffer para 100 muestras (100ms)
extern volatile uint8_t buffer_listo; // Bandera para la tarea
/* USER CODE END Variables */
osThreadId loggerHandle;
osThreadId samplerHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
float Procesar_Corriente_RMS(void);
/* USER CODE END FunctionPrototypes */

void StartLogger(void const * argument);
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

  osThreadDef(sampler, StartSampler, osPriorityHigh, 0, 128);
  samplerHandle = osThreadCreate(osThread(sampler), NULL);

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
  printf("Iniciando Logger...\r\n");
  /* Infinite loop */
  uint32_t acumulador_temp = 0;
  uint32_t cuenta_muestras_temp = 0;
  float temp_filtrada = 0.0f;
  float corriente_actual = 0.0f;

  /* Infinite loop */
  for(;;)
  {
    if (buffer_listo) {
        // 1. PROCESAMOS LA CORRIENTE (Esto tarda microsegundos, no bloquea)
        corriente_actual = Procesar_Corriente_RMS();
        buffer_listo = 0; // Liberamos el buffer al toque

        // 2. TOMAMOS UNA SOLA MUESTRA DE TEMPERATURA
        // Como pasó 100ms desde la última vez, el capacitor interno está limpísimo (0 crosstalk)
        HAL_ADC_Start(&hadc2);
        if(HAL_ADC_PollForConversion(&hadc2, 2) == HAL_OK) {
            acumulador_temp += HAL_ADC_GetValue(&hadc2);
            cuenta_muestras_temp++;
        }
        HAL_ADC_Stop(&hadc2);

        // 3. ¿YA TENEMOS LAS 32 MUESTRAS? (Pasan cada ~3.2 segundos)
        if (cuenta_muestras_temp >= 32) {
            float promedio_ticks = (float)acumulador_temp / 32.0f;
            float voltaje_mv = promedio_ticks * (3262.0f / 4096.0f);
            temp_filtrada = voltaje_mv / 10.0f;

            // Reseteamos para el próximo ciclo largo
            acumulador_temp = 0;
            cuenta_muestras_temp = 0;
        }

        // 4. ENVIAMOS A PYTHON
        // Manda la corriente nueva siempre, y la temperatura se actualiza cada 3.2s
        printf("%.2f,%.2f\r\n", temp_filtrada, corriente_actual);
    } 
    
    vTaskDelay(pdMS_TO_TICKS(1));  
  }
  /* USER CODE END StartLogger */
}

/* USER CODE BEGIN Header_StartSampler */
/** @brief  Function implementing the sampler thread.
  * @param  argument: Not used
  * @retval None
*/
/* USER CODE END Header_StartSampler */
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

/* USER CODE END 4 */
/* USER CODE END Application */

