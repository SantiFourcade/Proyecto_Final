#ifndef CAN_APP_H
#define CAN_APP_H

#include "can.h"
#include "queue.h"

typedef struct {
    CAN_RxHeaderTypeDef header;
    uint8_t data[8];
} CAN_Message_t;

extern QueueHandle_t canRxQueue;

void CAN_Rx_Task(void *argument);

#endif
