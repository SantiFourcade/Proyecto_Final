#include "features.h"
#include <math.h>

/**
 * @brief Calcula RMS, Peak y Crest Factor sobre la magnitud vectorial
 *        de los tres ejes del acelerómetro, con remoción de offset DC.
 *
 *        Usar magnitud en lugar de un solo eje tiene dos ventajas:
 *          - La orientación física del sensor no afecta el resultado.
 *          - Captura vibración en cualquier dirección simultáneamente.
 *
 *        El offset DC (gravedad + posición estática) se remueve
 *        restando la media de la ventana antes de calcular RMS/Peak,
 *        dejando solo la componente dinámica (vibración real).
 */
void Features_ComputeRMSPeak(const accel_window_t *window, vib_features_t *features)
{
    // --- Paso 1: calcular la magnitud cruda de cada muestra ---
    float mag[WINDOW_SIZE];

    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        float ax = (float)window->ax[i];
        float ay = (float)window->ay[i];
        float az = (float)window->az[i];
        mag[i] = sqrtf(ax*ax + ay*ay + az*az);
    }

    // --- Paso 2: restar media (eliminar componente DC = gravedad) ---
    float mean = 0.0f;
    for (int i = 0; i < WINDOW_SIZE; i++)
        mean += mag[i];
    mean /= WINDOW_SIZE;

    // --- Paso 3: RMS y Peak sobre la componente dinámica ---
    float sum_sq = 0.0f;
    float peak   = 0.0f;

    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        float a = mag[i] - mean;        /* componente AC de la vibración */
        sum_sq += a * a;
        if (fabsf(a) > peak)
            peak = fabsf(a);
    }

    features->rms   = sqrtf(sum_sq / WINDOW_SIZE);
    features->peak  = peak;

    /* Crest Factor = Pico / RMS
     * - Cercano a 1.4 : vibración sinusoidal pura (desequilibrio)
     * - 3–6           : vibración con algunos golpes (desalineación)
     * - >6            : impactos aislados, falla de rodamiento incipiente */
    features->crest = (features->rms > 0.0f) ? (peak / features->rms) : 0.0f;
}

/**
 * @brief Convierte una lectura ADC del LM35 a temperatura en °C.
 *        LM35: 10 mV/°C, referencia 3.3V, ADC 12 bits.
 */
float ADC_To_Temp(uint16_t adc)
{
    float v = (float)adc * 3.3f / 4095.0f;
    return v / 0.01f;
}

/**
 * @brief Calcula corriente RMS a partir de muestras ADC centradas.
 *        Asume señal AC centrada en 1.65V (mitad de 3.3V).
 *        Sensibilidad del sensor de corriente: 0.2 V/A.
 */
float ADC_To_Current(uint16_t *samples)
{
    float sum_sq = 0.0f;

    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        float v = ((float)samples[i] * 3.3f / 4095.0f) - 1.65f;
        sum_sq += v * v;
    }

    float v_rms = sqrtf(sum_sq / WINDOW_SIZE);
    return v_rms / 0.2f;
}