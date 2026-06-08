#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h" 
#include "stm32f1xx_hal_adc.h"
#include "stm32f1xx_hal_adc_ex.h"
#include "stm32f1xx_hal_rcc.h"

/* Handle ADC */
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

/* Init function */
void MX_ADC1_Init(void);
void MX_ADC2_Init(void);
void ADC_Select_Channel(uint32_t channel);

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H__ */
