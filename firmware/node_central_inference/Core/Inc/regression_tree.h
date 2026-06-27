#ifndef __REGRESSION_TREE_H__
#define __REGRESSION_TREE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

float PredictSpeed(
    float rms,
    float crest,
    float peak,
    float temp,
    float current);
    
#ifdef __cplusplus
}
#endif

#endif /* __REGRESSION_TREE_H__ */