#include "main.h" 
#include "adc.h"
#include <stdint.h>

/* ADC handle */
ADC_HandleTypeDef hadc1;

void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    /* Habilitar clock ADC1 */
    __HAL_RCC_ADC1_CLK_ENABLE();

    /* Configuración básica ADC */
    hadc1.Instance = ADC1;

    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;

    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;

    HAL_ADC_Init(&hadc1);

    /* Canal (CAMBIAR SEGÚN PIN) */
    sConfig.Channel = ADC_CHANNEL_1;   // PA1
    sConfig.Rank = ADC_REGULAR_RANK_1;

    /* Sampling largo para LM35 */
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;

    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /* Calibración */
    HAL_ADCEx_Calibration_Start(&hadc1);
}
