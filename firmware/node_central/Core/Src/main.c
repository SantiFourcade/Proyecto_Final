/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "can.h"
#include "fatfs.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "can_app.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
/* Variables para los contadores de la SD */
volatile uint8_t FatFsCnt = 0;
volatile uint8_t Timer1, Timer2;

/* USER CODE BEGIN PV */
extern CAN_HandleTypeDef hcan; 
extern QueueHandle_t canRxQueueSD;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
//void CAN_Filter_Init(void);

/* USER CODE BEGIN PFP */
/* Función de procesamiento de recepción CAN (ISR) */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_Message_t msg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    //printf(">> [ISR] Mensaje CAN recibido\r\n");

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &msg.header, msg.data) == HAL_OK)
    {
        // Enviamos a la cola para que la tarea SD la procese
        xQueueSendFromISR(canRxQueueSD, &msg, &xHigherPriorityTaskWoken);
        //printf(">> [ISR] Mensaje CAN en cola para SD\r\n");
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* Manejador de errores del bus */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    printf("CAN ERROR: 0x%08lX\r\n", HAL_CAN_GetError(hcan));
}

/* Manejador de tiempos de la SD (se llama cada 10ms) */
void SDTimer_Handler(void)
{  
    if(Timer1 > 0) Timer1--;
    if(Timer2 > 0) Timer2--;
}
/* USER CODE END PFP */

/**
  * @brief  The application entry point.
  */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_SPI1_Init();
    MX_FATFS_Init();
    MX_CAN_Init();
    CAN_Filter_Init();

    if (HAL_CAN_Start(&hcan) != HAL_OK) {
        printf("Error iniciando CAN receptor\r\n");
        Error_Handler();
    }
    CAN_RxHeaderTypeDef rxHeader_limpieza;
    uint8_t rxData_limpieza[8];
    // Sacamos hasta 3 mensajes viejos para dejar el buffer en cero
    while (HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rxHeader_limpieza, rxData_limpieza) == HAL_OK) {
        // Solo consumimos el buffer, no hace falta hacer nada con el dato
    }
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

    MX_FREERTOS_Init();


    printf("FMR=0x%08lX FM1R=0x%08lX\r\n", CAN1->FMR, CAN1->FM1R);
    printf("\r\n=== NODO CENTRAL READY ===\r\n");

    /* 4. Arrancar el scheduler */
    osKernelStart();

    while (1) {}
}

/**
  * @brief  Callback de Timers (Maneja el Tick de HAL y los de la SD)
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) // Timer configurado como Timebase Source
    {
        HAL_IncTick();
        
        // Generamos el tick de 10ms para el driver de la SD
        FatFsCnt++;
        if(FatFsCnt >= 10)
        {
            FatFsCnt = 0;
            SDTimer_Handler();
        }
    }
}

int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Inicializa los osciladores RCC según los parámetros del cristal de la placa
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9; // 8MHz * 9 = 72MHz
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Inicializa los relojes de los buses CPU, AHB y APB
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2; // APB1 = 36MHz (Para que el CAN use Prescaler=4)
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1; // APB2 = 72MHz (Para que el SPI1 vuele)

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void) {
/* USER CODE BEGIN Error_Handler_Debug */

/* User can add his own implementation to report the HAL error return state */
__disable_irq();
while (1)
{
}
/* USER CODE END Error_Handler_Debug */
}
