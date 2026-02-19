 /* USER CODE BEGIN Header */

/**

******************************************************************************

* @file : main.c

* @brief : Main program body

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

#include "main.h"

#include "cmsis_os.h"

#include "can.h"
#include "can_app.h"


#include "usart.h"

#include "gpio.h"
#include <stdio.h>
#include <string.h>

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


/* USER CODE BEGIN PV */

extern CAN_HandleTypeDef hcan;
extern QueueHandle_t canRxQueue;

/* USER CODE END PV */


/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

void MX_FREERTOS_Init(void);

void CAN_Filter_Init(void);

/* USER CODE BEGIN PFP */

/* Función de procesamiento de recepción */

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)

{

printf(">> INT CAN <<\r\n");

CAN_Message_t msg;

BaseType_t xHigherPriorityTaskWoken = pdFALSE;


if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &msg.header, msg.data) == HAL_OK)

{

xQueueSendFromISR(canRxQueue, &msg, &xHigherPriorityTaskWoken);

}


portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}

/* USER CODE END PFP */


/* Private user code ---------------------------------------------------------*/

/* USER CODE BEGIN 0 */


/* USER CODE END 0 */


/**

* @brief The application entry point.

* @retval int

*/

int main(void)
{
  /* 1. Inicialización básica del sistema */
  HAL_Init();
  SystemClock_Config(); // Configurado a 32MHz para sincronizar con el emisor

  /* 2. Reloj de AFIO para el remap de pines PB8/PB9 */
  __HAL_RCC_AFIO_CLK_ENABLE();

  /* 3. Inicialización de periféricos de hardware */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  
  /* Imprimimos el inicio una vez que la UART está lista */
  printf("\r\n=== RECEPTOR MANTENIMIENTO PREDICTIVO ===\r\n");

  /* 4. Configuración del CAN (Debe estar en CAN_MODE_NORMAL en can.c) */
  MX_CAN_Init();
  CAN_Filter_Init(); // Filtro abierto para recibir RMS, PEAK, etc.

  /* 5. Inicializar objetos de FreeRTOS (Crea la cola canRxQueue) */
  /* Es vital que esto ocurra ANTES de activar las interrupciones del CAN */
  MX_FREERTOS_Init();

  /* 6. Arrancar el periférico CAN y activar notificaciones */
  if (HAL_CAN_Start(&hcan) == HAL_OK)
  {
      printf("CAN Iniciado correctamente en Modo Normal\r\n");
      
      /* Activamos la interrupción de recepción para el FIFO 0 */
      HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_ERROR);
      
      uint32_t state = HAL_CAN_GetState(&hcan);
      printf("Estado inicial del bus: %ld\r\n", state);
  }
  else
  {
      printf("Error crítico: No se pudo arrancar el CAN\r\n");
      Error_Handler();
  }

  /* 7. Iniciar el Scheduler de FreeRTOS */
  /* A partir de aquí, la tarea CAN_Rx_Task tomará el control de los datos */
  osKernelStart();

  /* El sistema no debería llegar nunca a este punto */
  while (1);
}

/* La función del filtro se mantiene igual, está perfecta */

void CAN_Filter_Init(void)

{

CAN_FilterTypeDef filter;


filter.FilterBank = 0; // Usamos el banco 0

filter.FilterMode = CAN_FILTERMODE_IDMASK;

filter.FilterScale = CAN_FILTERSCALE_32BIT;

filter.FilterIdHigh = 0x0000;

filter.FilterIdLow = 0x0000;

filter.FilterMaskIdHigh = 0x0000;

filter.FilterMaskIdLow = 0x0000; // Máscara en 0 deja pasar TODO

filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;

filter.FilterActivation = ENABLE;

filter.SlaveStartFilterBank = 14;


if (HAL_CAN_ConfigFilter(&hcan, &filter) != HAL_OK)

{

printf("Error configurando filtro en main\r\n");

}

}


/* El _write para printf se mantiene igual al final */

/* UART printf support */

int _write(int file, char *ptr, int len)

{

HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);

return len;

}


/**

* @brief System Clock Configuration

* @retval None

*/

void SystemClock_Config(void)

{

RCC_OscInitTypeDef RCC_OscInitStruct = {0};

RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};


RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;

RCC_OscInitStruct.HSEState = RCC_HSE_ON;

RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;

RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;

RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4; // 8MHz * 4 = 32MHz (Igual que el emisor)


if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)

{

Error_Handler();

}


RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK

|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;

RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;

RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;

RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1; // APB1 a 32MHz

RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;


if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)

{

Error_Handler();

}

}

/* USER CODE BEGIN 4 */


/* USER CODE END 4 */


/**

* @brief Period elapsed callback in non blocking mode

* @note This function is called when TIM1 interrupt took place, inside

* HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment

* a global variable "uwTick" used as application time base.

* @param htim : TIM handle

* @retval None

*/

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)

{

/* USER CODE BEGIN Callback 0 */


/* USER CODE END Callback 0 */

if (htim->Instance == TIM1)

{

HAL_IncTick();

}

/* USER CODE BEGIN Callback 1 */


/* USER CODE END Callback 1 */

}


/**

* @brief This function is executed in case of error occurrence.

* @retval None

*/

void Error_Handler(void)

{

/* USER CODE BEGIN Error_Handler_Debug */

/* User can add his own implementation to report the HAL error return state */

__disable_irq();

while (1)

{

}

/* USER CODE END Error_Handler_Debug */

}

#ifdef USE_FULL_ASSERT

/**

* @brief Reports the name of the source file and the source line number

* where the assert_param error has occurred.

* @param file: pointer to the source file name

* @param line: assert_param error line source number

* @retval None

*/

void assert_failed(uint8_t *file, uint32_t line)

{

/* USER CODE BEGIN 6 */

/* User can add his own implementation to report the file name and line number,

ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

/* USER CODE END 6 */

}

#endif /* USE_FULL_ASSERT */ 