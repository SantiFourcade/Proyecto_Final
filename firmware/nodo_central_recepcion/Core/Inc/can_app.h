#ifndef CAN_APP_H
#define CAN_APP_H

#include "can.h"
#include "queue.h"

/* Mensaje CAN genérico para RX */
typedef struct {
    CAN_RxHeaderTypeDef header;
    uint8_t data[8];
} CAN_Message_t;

/* Cola de recepción CAN */
extern QueueHandle_t canRxQueue;

/* Task de recepción CAN */
void CAN_Rx_Task(void *argument);

#endif
