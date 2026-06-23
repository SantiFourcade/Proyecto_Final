#include "adxl345.h"

/* Timeout en ms para operaciones I2C.
 * Debe ser mayor que el tiempo de un byte a 100kHz (~0.1ms)
 * pero lo suficientemente corto para que FreeRTOS pueda recuperarse.
 * 10ms es conservador y seguro. */
#define ADXL345_I2C_TIMEOUT 10U

static I2C_HandleTypeDef *adxl_i2c;

/**
 * @brief Inicializa el ADXL345 y verifica que el sensor responda.
 * @retval HAL_OK si el sensor fue configurado correctamente.
 *         HAL_ERROR si el sensor no responde o no se puede verificar.
 */
HAL_StatusTypeDef ADXL345_Init(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef ret;
    uint8_t data;

    adxl_i2c = hi2c;

    /* Verificar que el sensor responde leyendo el registro DEVID (debe ser 0xE5) */
    ret = HAL_I2C_Mem_Read(adxl_i2c, ADXL345_ADDR,
                           ADXL345_DEVID,
                           I2C_MEMADD_SIZE_8BIT,
                           &data, 1, ADXL345_I2C_TIMEOUT);
    if (ret != HAL_OK)
        return ret;

    if (data != 0xE5)
        return HAL_ERROR;  /* Sensor no es un ADXL345 o está en mal estado */

    /* Modo medida (salir de standby) */
    data = 0x08;
    ret = HAL_I2C_Mem_Write(adxl_i2c, ADXL345_ADDR,
                            ADXL345_POWER_CTL,
                            I2C_MEMADD_SIZE_8BIT,
                            &data, 1, ADXL345_I2C_TIMEOUT);
    if (ret != HAL_OK)
        return ret;

    /* ±2g, full resolution */
    data = 0x08;
    ret = HAL_I2C_Mem_Write(adxl_i2c, ADXL345_ADDR,
                            ADXL345_DATA_FORMAT,
                            I2C_MEMADD_SIZE_8BIT,
                            &data, 1, ADXL345_I2C_TIMEOUT);
    if (ret != HAL_OK)
        return ret;

    return HAL_OK;
}

/**
 * @brief Lee los ejes X, Y, Z del ADXL345.
 * @retval HAL_OK si la lectura fue exitosa.
 *         HAL_TIMEOUT si el sensor no respondió en ADXL345_I2C_TIMEOUT ms.
 *         HAL_ERROR / HAL_BUSY en otros errores de bus.
 */
HAL_StatusTypeDef ADXL345_ReadXYZ(int16_t *x, int16_t *y, int16_t *z)
{
    HAL_StatusTypeDef ret;
    uint8_t buf[6];

    ret = HAL_I2C_Mem_Read(adxl_i2c, ADXL345_ADDR,
                           ADXL345_DATAX0,
                           I2C_MEMADD_SIZE_8BIT,
                           buf, 6, ADXL345_I2C_TIMEOUT);

    if (ret != HAL_OK)
        return ret;  /* Propagar el error — no retornar HAL_OK a ciegas */

    *x = (int16_t)((buf[1] << 8) | buf[0]);
    *y = (int16_t)((buf[3] << 8) | buf[2]);
    *z = (int16_t)((buf[5] << 8) | buf[4]);

    return HAL_OK;
}