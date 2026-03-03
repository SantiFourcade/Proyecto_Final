/* Includes ------------------------------------------------------------------*/
#include "can.h"
#include <stdio.h>

CAN_HandleTypeDef hcan;

/* CAN init function */
void MX_CAN_Init(void)
{
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 8;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = ENABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = ENABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;

  /* --- MANIOBRA MANUAL DE DESPERTADO --- */
  // Obligatorio en F103 para asegurar que el periférico responda
  __HAL_RCC_CAN1_CLK_ENABLE();
  
  CAN1->MCR &= ~CAN_MCR_SLEEP;
  
  uint32_t timeout = 0xFFFF;
  while ((CAN1->MSR & CAN_MSR_SLAK) && timeout--); 
  /* ------------------------------------ */

  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    printf("Error crítico: HAL_CAN_Init falló\r\n");
    Error_Handler();
  }
  printf("CAN Hardware inicializado (Modo Normal)\r\n");
}

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN1)
  {
    /* 1. Habilitar Relojes */
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    
    /* 2. Desactivar USB para evitar conflictos de memoria en F103 */
    __HAL_RCC_USB_CLK_DISABLE();

    /* 3. Configurar Remap a PB8 y PB9 */
    __HAL_AFIO_REMAP_CAN1_2();

    /** CAN GPIO Configuration
    PB8     ------> CAN_RX
    PB9     ------> CAN_TX
    */
    // RX: Usamos PULLUP del primer archivo para mayor estabilidad
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP; 
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // TX: Push-Pull de alta velocidad
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* 4. Configurar interrupciones para FreeRTOS */
    // Prioridad 6: superior a la del Kernel (5) para poder usar colas
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
    
    // Opcional: Interrupción de error para diagnóstico
    HAL_NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{
  if(canHandle->Instance==CAN1)
  {
    __HAL_RCC_CAN1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8|GPIO_PIN_9);
    HAL_NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
  }
}