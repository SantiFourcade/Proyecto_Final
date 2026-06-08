/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (Integrated Version)
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "can.h"
#include "i2c.h"
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <stdio.h>
#include "adxl345.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
volatile uint8_t flag_pulso = 0;
volatile uint32_t pulsos = 0;
volatile uint16_t adc_buffer_corriente[100]; // Buffer para 100 muestras (100ms)
volatile uint16_t indice_adc = 0;
volatile uint8_t buffer_listo = 0; // Bandera para la tarea

/* Variables para el cálculo de RPM */
volatile uint32_t diff_ticks = 0;
volatile uint8_t new_read = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  /* Inicializa la Flash, las interrupciones y los perisfericos de bajo nivel */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init(); // Activa el perisferico ADC
  MX_ADC2_Init();
  
  MX_I2C1_Init();
  ADXL345_Init(&hi2c1);
  MX_CAN_Init();

  if (HAL_CAN_Start(&hcan) != HAL_OK)
  {
   Error_Handler();
  }

  /* Activar interrupciones CAN */
HAL_CAN_ActivateNotification(&hcan,
                             CAN_IT_TX_MAILBOX_EMPTY |
                             CAN_IT_BUSOFF |
                             CAN_IT_ERROR);

  
  /* Inicialización de Timer para Input Capture (RPM) */
  MX_TIM3_Init();
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);

  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Prepara las tareas y colas */
  MX_FREERTOS_Init();

  /* Start scheduler - Control total a FreeRTOS */
  osKernelStart();

  /* Infinite loop */
  while (1)
  {
  }
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV8;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
  * @brief Callback para captura de Input Capture (RPM)
  */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        diff_ticks = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        new_read = 1;
    }
}

/**
  * @brief UART printf support
  * Redirige los printf a UART1
  */
int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        // Almacenamiento puro
        adc_buffer_corriente[indice_adc] = HAL_ADC_GetValue(hadc);
        indice_adc++;

        // Cuando llenamos el buffer (ej. 100 muestras = 100ms a 1kHz)
        if (indice_adc >= 100) {
            indice_adc = 0;
            buffer_listo = 1; // Avisamos a la tarea
        }
    }
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    printf("MAILBOX0 TX OK\r\n");
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
    printf("MAILBOX1 TX OK\r\n");
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
    printf("MAILBOX2 TX OK\r\n");
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    uint32_t err = HAL_CAN_GetError(hcan);

    printf("CAN ERROR: %lu\r\n", err);
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode (para Systick de FreeRTOS)
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    HAL_IncTick();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  __disable_irq(); // deshabilita interrupciones
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */