#ifndef FEATURES_H
#define FEATURES_H

#include <stdint.h>

#define WINDOW_SIZE 10

typedef struct {
    int16_t ax[WINDOW_SIZE];
    int16_t ay[WINDOW_SIZE];
    int16_t az[WINDOW_SIZE];
} accel_window_t;

typedef struct {
    float rms;
    float peak;
    float crest;
} vib_features_t;

typedef struct {
    accel_window_t accel;
    uint16_t temp;
} raw_data_t;

typedef struct {
    vib_features_t vib;
    float temperature;
} data_frame_t;

void Features_ComputeRMSPeak(const accel_window_t *window,
                             vib_features_t *features);

float ADC_To_Temp(uint16_t adc);

#endif
