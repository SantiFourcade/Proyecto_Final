#include "features.h"
#include <math.h>

void Features_ComputeRMSPeak(const accel_window_t *window,
                             vib_features_t *features)
{
    float sum_sq = 0.0f;
    float peak = 0.0f;
    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        float a = (float)window->ax[i];
        sum_sq += a * a;
        if (fabsf(a) > peak)
            peak = fabsf(a);
    }
    features->rms = sqrtf(sum_sq / WINDOW_SIZE); // Valor RMS 
    features->peak = peak;                       // Valor Pico
    features->crest = peak / features->rms;      // Factor de cresta = Pico/RMS (bajo si la vibración persiste, alto si fue un pico aislado)
}

float ADC_To_Temp(uint16_t adc)
{
    float v = (float)adc * 3.3f / 4095.0f;
    return v / 0.01f;   // LM35: 10mV/°C
}