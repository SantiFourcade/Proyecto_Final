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
    features->rms = sqrtf(sum_sq / WINDOW_SIZE);
    features->peak = peak;
    features->crest = peak / features->rms;
}
