#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"

#include "can_app.h"
#include "features.h"
#include "gpio.h"
#include <stdio.h>

/* Queues */
QueueHandle_t canQueue;

/* Tasks */
void StartTxCAN(void const * argument);

void MX_FREERTOS_Init(void)
{
  /* Cola de features */
  canQueue = xQueueCreate(2, sizeof(vib_features_t));
  configASSERT(canQueue);

  /* Task CAN TX */
  osThreadDef(txCAN, StartTxCAN, osPriorityAboveNormal, 0, 256);
  osThreadCreate(osThread(txCAN), NULL);
}


/* ================= TASK CAN TX ================= */
void StartTxCAN(void const * argument)
{
  vib_features_t features;

  for (;;)
  {
    /* Simulación simple si no hay productor todavía */
    features.rms   = 1.23f;
    features.crest = 2.34f;
    features.peak  = 3.45f;

    CAN_SendFloat(0x101, features.rms);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

    CAN_SendFloat(0x102, features.crest);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

    CAN_SendFloat(0x103, features.peak);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

    //printf("CAN TX -> RMS %.2f | CREST %.2f | PEAK %.2f\r\n",
      //      features.rms, features.crest, features.peak);

    osDelay(500);
  }
}
