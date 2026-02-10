#include "adxl345.h"

static I2C_HandleTypeDef *adxl_i2c;

HAL_StatusTypeDef ADXL345_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t data;

    adxl_i2c = hi2c;

    /* Modo medida */
    data = 0x08;
    HAL_I2C_Mem_Write(adxl_i2c, ADXL345_ADDR,
                      ADXL345_POWER_CTL,
                      I2C_MEMADD_SIZE_8BIT,
                      &data, 1, HAL_MAX_DELAY);

    /* ±2g, full resolution */
    data = 0x08;
    HAL_I2C_Mem_Write(adxl_i2c, ADXL345_ADDR,
                      ADXL345_DATA_FORMAT,
                      I2C_MEMADD_SIZE_8BIT,
                      &data, 1, HAL_MAX_DELAY);

    return HAL_OK;
}

HAL_StatusTypeDef ADXL345_ReadXYZ(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6];

    HAL_I2C_Mem_Read(adxl_i2c, ADXL345_ADDR,
                     ADXL345_DATAX0,
                     I2C_MEMADD_SIZE_8BIT,
                     buf, 6, HAL_MAX_DELAY);

    *x = (int16_t)((buf[1] << 8) | buf[0]);
    *y = (int16_t)((buf[3] << 8) | buf[2]);
    *z = (int16_t)((buf[5] << 8) | buf[4]);

    return HAL_OK;
}
