#include "can.h"
#include <stdio.h>

CAN_HandleTypeDef hcan;

void MX_CAN_Init(void)
{
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 4;      // 32MHz / 4 = 8MHz quanta clock
  hcan.Init.Mode = CAN_MODE_NORMAL;
  //hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = DISABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = ENABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = ENABLE;

//printf("Iniciando secuencia de despertar CAN...\r\n");

  /* --- MANIOBRA MANUAL --- */
  // 1. Encendemos el reloj manualmente antes de la HAL por si acaso
  __HAL_RCC_CAN1_CLK_ENABLE();
  
  // 2. Quitamos el bit de SLEEP
  CAN1->MCR &= ~CAN_MCR_SLEEP;
  
  // 3. Esperamos un poquito a que el hardware se despierte
  uint32_t timeout = 0xFFFF;
  while ((CAN1->MSR & CAN_MSR_SLAK) && timeout--); 
  
  //printf("Salida de Sleep: %s\r\n", (CAN1->MSR & CAN_MSR_SLAK) ? "FALLÓ" : "OK");
  /* ----------------------- */

  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    printf("ERROR CAN INIT\r\n");
    Error_Handler();
  }

  printf("CAN INIT exitoso!\r\n");

}


void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (canHandle->Instance == CAN1)
  {
    __HAL_RCC_USB_CLK_DISABLE();
    //__HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    /* Remap CAN: PB8 RX, PB9 TX */
    __HAL_AFIO_REMAP_CAN1_2();

    /* PA11 -> RX */
    //GPIO_InitStruct.Pin = GPIO_PIN_11;
    //GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
    //GPIO_InitStruct.Pull = GPIO_NOPULL;
    //HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA12 -> TX */
    //GPIO_InitStruct.Pin = GPIO_PIN_12;
    //GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    //GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    //HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* PB8 -> CAN_RX */
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PB9 -> CAN_TX */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* NVIC CAN RX */
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
  }
}
