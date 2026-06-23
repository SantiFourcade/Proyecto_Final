  /**
  ******************************************************************************
  * @file    fixed_point.h
  * @brief   This file contains all the function prototypes for
  *          the fixed_point.c file
  ******************************************************************************
  **/
#ifndef __FIXED_POINT_H__
#define __FIXED_POINT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int16_t FloatTo_Q8_7(float valor_float);
int16_t FloatTo_Q11_4(float valor_float);


#ifdef __cplusplus
}
#endif

#endif /* __FIXED_POINT_H__ */