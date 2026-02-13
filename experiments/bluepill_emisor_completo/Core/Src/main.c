/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "cmsis_os.h" // Interfaz para FreeRTOS

#include "can.h"
#include "i2c.h"
#include "adc.h"
#include "usart.h"
#include "gpio.h"

#include <stdlib.h>
#include <stdio.h>

#include "adxl345.h"


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);


int main(void)
{
  HAL_Init(); // Inicializa la Flash, las interrupciones y los perisfericos de bajo nivel
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  ADXL345_Init(&hi2c1);
  MX_CAN_Init();
  MX_ADC1_Init(); // Activa el perisferico CAN para que puede enviar y recibir
  MX_FREERTOS_Init(); // Prepara las tareas y colas
  osKernelStart(); // Control total a FreeRTOS

  while (1) {
  }
}


/* UART printf support */
// Redirige los printf a UART1
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
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4; // 8MHz * 4 = 32MHz 

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Inicializa los relojes de CPU, AHB y APB 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1; // Velocidad del procesador y memoria (32M)
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1; // CAN corre a la frecuencia del PLL
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1; // ADC y GPIO

  // La flash responde mas lento que el procesador por lo que se espera un ciclo de reloj
  // para poder leer en ella y no tire HardFault
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}


void Error_Handler(void)
{
  __disable_irq(); // deshabilita interrupciones
  while (1) {}
}
