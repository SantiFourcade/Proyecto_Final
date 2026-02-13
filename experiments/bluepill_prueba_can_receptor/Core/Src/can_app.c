#include "can.h" 
#include "can_app.h" 
#include <stdio.h>

extern CAN_HandleTypeDef hcan; // Usamos la hcan que ya configuramos

void CAN_SendFloat(uint16_t id, float value)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    
    // Unión para convertir float a bytes sin perder precisión
    union {
        float f;
        uint8_t bytes[4];
    } data;
    
    data.f = value;

    TxHeader.StdId = id;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 4; // Un float ocupa 4 bytes
    TxHeader.TransmitGlobalTime = DISABLE;

    // Enviamos
if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, data.bytes, &TxMailbox) != HAL_OK)    {
        // Si falla, al menos sabemos por qué
        // printf("Error TX CAN\r\n"); 
    }
}