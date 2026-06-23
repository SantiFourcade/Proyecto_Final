#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

/* Dirección I2C del ADXL345 (0x53 << 1) */
#define ADXL345_ADDR        (0x53 << 1)

/* Registros */
#define ADXL345_DEVID       0x00  
#define ADXL345_POWER_CTL   0x2D
#define ADXL345_DATA_FORMAT 0x31
#define ADXL345_DATAX0      0x32

HAL_StatusTypeDef ADXL345_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef ADXL345_ReadXYZ(int16_t *x, int16_t *y, int16_t *z);

#endif