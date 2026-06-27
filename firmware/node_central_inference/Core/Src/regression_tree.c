/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications (Merged Version)
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "math.h"
#include <stdio.h>

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
float PredictSpeed(
    float rms,
    float crest,
    float peak,
    float temp,
    float current);


float PredictSpeed(
    float rms,
    float crest,
    float peak,
    float temp,
    float current)
{
    if(current <= 0.18f)
    {
        if(temp <= 46.95f)
        {
            return 0.0f;
        }
        else
        {
            if(temp <= 47.01f)
            {
                if(rms <= 1.94f)
                {
                    return 236.19f;
                }
                else
                {
                    if(current <= 0.07f)
                    {
                        return 844.11f;
                    }
                    else
                    {
                        return 579.04f;
                    }
                }
            }
            else
            {
                if(peak <= 6.44f)
                {
                    if(current <= 0.07f)
                    {
                        return 45.70f;
                    }
                    else
                    {
                        return 4.35f;
                    }
                }
                else
                {
                    if(rms <= 2.58f)
                    {
                        return 458.38f;
                    }
                    else
                    {
                        return 0.0f;
                    }
                }
            }
        }
    }
    else
    {
        if(current <= 0.35f)
        {
            if(temp <= 23.48f)
            {
                if(temp <= 21.37f)
                {
                    if(temp <= 21.07f)
                    {
                        return 1440.49f;
                    }
                    else
                    {
                        return 1428.96f;
                    }
                }
                else
                {
                    if(temp <= 21.76f)
                    {
                        return 1448.71f;
                    }
                    else
                    {
                        return 1435.06f;
                    }
                }
            }
            else
            {
                if(temp <= 41.41f)
                {
                    if(temp <= 30.16f)
                    {
                        return 1451.64f;
                    }
                    else
                    {
                        return 1445.59f;
                    }
                }
                else
                {
                    if(temp <= 42.83f)
                    {
                        return 1455.13f;
                    }
                    else
                    {
                        return 1449.16f;
                    }
                }
            }
        }
        else
        {
            if(temp <= 45.21f)
            {
                if(temp <= 40.12f)
                {
                    if(current <= 0.35f)
                    {
                        return 1431.21f;
                    }
                    else
                    {
                        return 1444.92f;
                    }
                }
                else
                {
                    if(temp <= 43.18f)
                    {
                        return 1463.65f;
                    }
                    else
                    {
                        return 1450.40f;
                    }
                }
            }
            else
            {
                return 896.06f;
            }
        }
    }
}

/* USER CODE BEGIN Application */

/* USER CODE END Application */