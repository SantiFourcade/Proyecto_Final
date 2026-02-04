/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "can.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include <stdlib.h>
#include "adxl345.h"


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);


int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART1_UART_Init();

  //printf("\r\n=== BOOT OK ===\r\n");

  MX_I2C1_Init();
  //printf("I2C init OK\r\n");
  ADXL345_Init(&hi2c1);


  printf("\r\n=== SISTEMA INICIADO ===\r\n");
  //printf("Frecuencia CPU: %lu Hz\r\n", HAL_RCC_GetSysClockFreq());

  MX_CAN_Init();
  //printf("CAN init OK\r\n");

  if (HAL_CAN_Start(&hcan) == HAL_OK)
    printf("CAN started OK\r\n");
  else
    printf("CAN start ERROR\r\n");

  MX_FREERTOS_Init();
  printf("FreeRTOS init OK\r\n");

  osKernelStart();

  while (1) {
    
  }
}


/* UART printf support */
int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}

/* System Clock Configuration */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Inicializa los osciladores RCC según los parámetros especificados 
  * en la estructura RCC_OscInitTypeDef.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON; // Activamos el cristal de la placa
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE; // Fuente: Cristal externo
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;       // 8MHz * 4 = 32MHz (o usa MUL9 para 72MHz)

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Inicializa los relojes de CPU, AHB y APB 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1; // CAN corre a la frecuencia del PLL
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}


void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
