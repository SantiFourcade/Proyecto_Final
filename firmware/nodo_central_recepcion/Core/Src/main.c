/* USER CODE BEGIN Header */
// ... (mismo header)
/* USER CODE END Header */

#include "main.h"
#include "cmsis_os.h"
#include "can.h"    // Asegúrate de incluir esto
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>
#include "can_app.h" 

/* Variables privadas */
extern CAN_HandleTypeDef hcan; // Declarada en can.c

/* Función de procesamiento de recepción */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_Message_t msg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &msg.header, msg.data) == HAL_OK)
    {
        xQueueSendFromISR(canRxQueue, &msg, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void MX_FREERTOS_Init(void);
void SystemClock_Config(void);
void CAN_Filter_Init(void);

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    uint32_t err = HAL_CAN_GetError(hcan);

    printf("🔥 CAN ERROR: 0x%08lX\r\n", err);

    if (err & HAL_CAN_ERROR_ACK)
        printf("  - ACK error\r\n");

    if (err & HAL_CAN_ERROR_BOF)
        printf("  - Bus Off\r\n");

    if (err & HAL_CAN_ERROR_EPV)
        printf("  - Error Passive\r\n");

    if (err & HAL_CAN_ERROR_STF)
        printf("  - Stuff error\r\n");

    if (err & HAL_CAN_ERROR_FOR)
        printf("  - Form error\r\n");
}


int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_CAN_Init(); // Recordá ponerlo en CAN_MODE_NORMAL dentro de can.c

    /* 1. Configurar Filtro */
    CAN_Filter_Init();     

    /* 2. Arrancar el periférico */
    if (HAL_CAN_Start(&hcan) != HAL_OK) {
        printf("Error iniciando CAN receptor\r\n");
        Error_Handler();
    }

    /* 3. Activar Interrupción */
if (HAL_CAN_ActivateNotification(&hcan,
        CAN_IT_RX_FIFO0_MSG_PENDING |
        CAN_IT_ERROR |
        CAN_IT_BUSOFF |
        CAN_IT_LAST_ERROR_CODE
    ) != HAL_OK)
{
    printf("Error activando notificaciones CAN\r\n");
    Error_Handler();
}

    printf("=== RECEPTOR CAN LISTO (MODO NORMAL) ===\r\n");

    /* Si vas a usar FreeRTOS, el scheduler toma el control aquí */
    MX_FREERTOS_Init();
    osKernelStart();

    /* Si NO usás FreeRTOS en esta placa, el while puede quedar vacío */
    while (1)
    {
        // Las interrupciones se encargan de imprimir
    }
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