#include <stdint.h>
#include <math.h>

/**
 * @brief Formato Q8.7 para Corriente, Temperatura y Aceleracion
 * Rango: -256.0 a 255.99 | Precision: 0.0078
 */
/*
Logica:
    valor_float * N desplaza la coma N bits
    roundf redondea al entero mas cercano
    int16_t convierte a formato S(15-N,N)
*/

int16_t FloatTo_Q8_7(float valor_float)
{
    if (valor_float < -256.0f) valor_float = -256.0f;
    if (valor_float > 255.99f) valor_float = 255.99f;

    return (int16_t)roundf(valor_float * 128.0f); // 2^7 = 128
}

/**
 * @brief Formato Q11.4 para Velocidad (RPM)
 * Rango: -2048.0 a 2047.93 | Precision: 0.0625
 */
int16_t FloatTo_Q11_4(float valor_float)
{
    if (valor_float < -2048.0f) valor_float = -2048.0f;
    if (valor_float > 2047.93f) valor_float = 2047.93f;

    return (int16_t)roundf(valor_float * 16.0f); // 2^4 = 16
}