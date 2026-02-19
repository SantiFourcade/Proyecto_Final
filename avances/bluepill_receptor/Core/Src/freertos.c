 /* USER CODE BEGIN Header */

/**

******************************************************************************

* File Name : freertos.c

* Description : Code for freertos applications

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

#include "FreeRTOS.h"

#include "task.h"

#include "main.h"

#include "cmsis_os.h"
#include <stdio.h>    // Para printf
#include <string.h>   // Para memcpy
#include "can.h"      // Para que reconozca 'hcan'
#include "can_app.h"  // Para CAN_Message_t y CAN_Rx_Task

/* Private includes ----------------------------------------------------------*/

/* USER CODE BEGIN Includes */


/* USER CODE END Includes */


/* Private typedef -----------------------------------------------------------*/

/* USER CODE BEGIN PTD */


/* USER CODE END PTD */


/* Private define ------------------------------------------------------------*/

/* USER CODE BEGIN PD */


/* USER CODE END PD */


/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PM */


/* USER CODE END PM */


/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN Variables */


/* USER CODE END Variables */

osThreadId acquireHandle;

osThreadId packageHandle;

osThreadId txCANHandle;


osMessageQId rawQueueHandle;

osMessageQId canQueueHandle;


QueueHandle_t rawQueue;

QueueHandle_t canQueue;

QueueHandle_t canRxQueue;



/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN FunctionPrototypes */


/* USER CODE END FunctionPrototypes */


void StartTxCAN(void const * argument);


void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */


/* GetIdleTaskMemory prototype (linked to static allocation support) */

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );


/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */

static StaticTask_t xIdleTaskTCBBuffer;

static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];


void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )

{

*ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;

*ppxIdleTaskStackBuffer = &xIdleStack[0];

*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;

/* place for user code */

}

/* USER CODE END GET_IDLE_TASK_MEMORY */


/**

* @brief FreeRTOS initialization

* @param None

* @retval None

*/

void MX_FREERTOS_Init(void) {

/* USER CODE BEGIN Init */


/* USER CODE END Init */


/* USER CODE BEGIN RTOS_MUTEX */

/* add mutexes, ... */

/* USER CODE END RTOS_MUTEX */


/* USER CODE BEGIN RTOS_SEMAPHORES */

/* add semaphores, ... */

/* USER CODE END RTOS_SEMAPHORES */


/* USER CODE BEGIN RTOS_TIMERS */

/* start timers, add new ones, ... */

/* USER CODE END RTOS_TIMERS */


/* USER CODE BEGIN RTOS_QUEUES */

canRxQueue = xQueueCreate(16, sizeof(CAN_Message_t));

/* USER CODE END RTOS_QUEUES */


/* Create the thread(s) */

/* definition and creation of defaultTask */

// osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);

// defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);


/* USER CODE BEGIN RTOS_THREADS */

xTaskCreate(

CAN_Rx_Task,

"CAN_RX",

512,

NULL,

osPriorityNormal,

NULL

); /* USER CODE END RTOS_THREADS */


}


/* USER CODE BEGIN Header_StartDefaultTask */

/**

* @brief Function implementing the defaultTask thread.

* @param argument: Not used

* @retval None

*/

/* USER CODE END Header_StartDefaultTask */

void CAN_Rx_Task(void *argument)
{
    CAN_Message_t msg;
    float value;

    for (;;)
    {
        //uint32_t esr = CAN1->ESR;
        //printf("ESR: 0x%08lX\r\n", esr);

        // 1. Monitoreo constante de errores de hardware
        uint32_t can_error = HAL_CAN_GetError(&hcan);
        if (can_error != HAL_CAN_ERROR_NONE) {
            // Si el error es 0x04, es falta de ACK (problema de cables o resistencias)
            // Si el error es 0x01, es Stuff Error (desincronización de baudrate)
            printf("!!! Error de Bus detectado: 0x%08lX\r\n", can_error);
        }

        // 2. Espera de mensajes en la cola
        if (xQueueReceive(canRxQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            // Si entra aquí, la comunicación física existe
            memcpy(&value, msg.data, sizeof(float));

            const char *label = "DESCONOCIDO";
            if (msg.header.StdId == 0x101) label = "RMS";
            else if (msg.header.StdId == 0x102) label = "CREST";
            else if (msg.header.StdId == 0x103) label = "PEAK";
            else if (msg.header.StdId == 0x104) label = "TEMP";

            printf(">> [RX] %s (ID: 0x%03lX) -> Val: %.3f\r\n", 
                   label, msg.header.StdId, value);
        }
    }
}



/* Private application code --------------------------------------------------*/

/* USER CODE BEGIN Application */


/* USER CODE END Application */ 