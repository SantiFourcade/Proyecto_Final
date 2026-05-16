#include "can.h"
#include "can_app.h"
#include <stdio.h>

extern CAN_HandleTypeDef hcan;

void CAN_SendFloat(uint16_t id, float value)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;

    union {
        float f;
        uint8_t bytes[4];
    } data;

    data.f = value;

    TxHeader.StdId = id;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 4;
    TxHeader.TransmitGlobalTime = DISABLE;

    /* Esperar mailbox libre */
    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0)
    {
        //taskYIELD();   // o osDelay(1)
    }

    if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, data.bytes, &TxMailbox) == HAL_OK)
    {
        //printf(
          //  "[CAN SEND] ID=0x%03X  DLC=%d  DATA=%02X %02X %02X %02X  (%.3f)\r\n",
           // id,
           // TxHeader.DLC,
           // data.bytes[0],
           // data.bytes[1],
           // data.bytes[2],
           // data.bytes[3],
           // value
        //);
    }
    else
    {
        printf("[CAN TX] ERROR al enviar ID=0x%03X\r\n", id);
    }
}
