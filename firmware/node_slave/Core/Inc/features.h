#ifndef FEATURES_H
#define FEATURES_H

#include <stdint.h>

#define WINDOW_SIZE 128

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

void Features_ComputeRMSPeak(const accel_window_t *window,
                             vib_features_t *features);

#endif
