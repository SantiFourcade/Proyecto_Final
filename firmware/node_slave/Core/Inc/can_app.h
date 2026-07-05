#ifndef CAN_APP_H
#define CAN_APP_H

#include <stdint.h>

typedef struct {
    CAN_RxHeaderTypeDef header;
    uint8_t data[8];
} CAN_Message_t;

void CAN_SendFloat(uint16_t id, float value);

#endif
