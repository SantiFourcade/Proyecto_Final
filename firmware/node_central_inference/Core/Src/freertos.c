/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */
#include "can_app.h"
#include "can_frame.h"
#include "regression_tree.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "can_frame.h"
/* USER CODE END Includes */

/* USER CODE BEGIN Variables */
extern CAN_HandleTypeDef hcan;

/* Ensamblador de frame CAN — compartido entre CANParser y MLTask */
static can_frame_assembler_t  s_assembler;

/* Queue que avisa a MLTask cuando un frame está completo */
static QueueHandle_t s_mlQueue;  /* tamaño 1 ml_frame_t — el resultado del modelo */

/* USER CODE END Variables */

osThreadId CANParserHandle;
osThreadId MLTaskHandle;

QueueHandle_t canRxQueueSD;

/* Private function prototypes -----------------------------------------------*/
void StartCANParserTask(void const * argument);
void StartMLTask(void const * argument);
void MX_FREERTOS_Init(void);

/* Static allocation for Idle task */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t  xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t  **ppxIdleTaskStackBuffer,
                                    uint32_t      *pulIdleTaskStackSize )
{
    *ppxIdleTaskTCBBuffer  = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = &xIdleStack[0];
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

/* ============================================================
 * MX_FREERTOS_Init
 * ============================================================ */
void MX_FREERTOS_Init(void)
{
    /* Inicializar el ensamblador de frame */
    FrameAssembler_Init(&s_assembler);

    /* Cola de mensajes CAN crudos → CANParserTask */
    canRxQueueSD = xQueueCreate(64, sizeof(CAN_Message_t));
    configASSERT(canRxQueueSD);

    /* Cola de resultado del modelo → MLTask (profundidad 1, sobrescribible) */
    s_mlQueue = xQueueCreate(1, sizeof(ml_frame_t));
    configASSERT(s_mlQueue);

    /* Tarea CAN Parser — alta prioridad para no perder mensajes */
    osThreadDef(CANParser, StartCANParserTask, osPriorityAboveNormal, 0, 512);
    CANParserHandle = osThreadCreate(osThread(CANParser), NULL);

    /* Tarea ML — prioridad normal, se activa cuando el frame está completo */
    osThreadDef(MLTask, StartMLTask, osPriorityNormal, 0, 512);
    MLTaskHandle = osThreadCreate(osThread(MLTask), NULL);
}

/* ============================================================
 * StartCANParserTask
 *
 * Recibe mensajes CAN de la ISR, los alimenta al ensamblador,
 * y cuando el frame está completo ejecuta el modelo y publica
 * el resultado en s_mlQueue.
 * ============================================================ */
void StartCANParserTask(void const * argument)
{
    CAN_Message_t rxMsg;
    float         canValue;

    for (;;)
    {
        /* Bloquear hasta recibir un mensaje CAN */
        if (xQueueReceive(canRxQueueSD, &rxMsg, portMAX_DELAY) != pdPASS)
            continue;

        /* Verificar timeout del frame en curso antes de procesar */
        FrameAssembler_CheckTimeout(&s_assembler, HAL_GetTick());

        /* Decodificar float del payload */
        memcpy(&canValue, rxMsg.data, sizeof(float));
        //HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);  /* LED de actividad */
        /* Log por UART (igual que antes) */
        const char *label = "UNKNOWN";
        if      (rxMsg.header.StdId == CAN_ID_RMS)     label = "RMS";
        else if (rxMsg.header.StdId == CAN_ID_CREST)   label = "CREST";
        else if (rxMsg.header.StdId == CAN_ID_PEAK)    label = "PEAK";
        else if (rxMsg.header.StdId == CAN_ID_TEMP)    label = "TEMP";
        else if (rxMsg.header.StdId == CAN_ID_CURRENT) label = "CURRENT";
        else if (rxMsg.header.StdId == CAN_ID_SPEED)   label = "SPEED";

        /* Alimentar el ensamblador con el campo recibido.
         * Cuando retorna 1 el frame está completo y el modelo ya fue ejecutado
         * internamente — publicamos el resultado a MLTask. */
        /* Alimentar ensamblador — cuando completa los 6 campos publica
         * ml_frame_t en s_mlQueue y MLTask se desbloquea automáticamente */
        FrameAssembler_Feed(&s_assembler,
                            rxMsg.header.StdId,
                            canValue,
                            HAL_GetTick(),
                            s_mlQueue);
    }
}

/* ============================================================
 * StartMLTask
 *
 * Se desbloquea cuando llega un frame completo por s_mlQueue,
 * compara predicción con velocidad real y genera alarma si corresponde.
 * ============================================================ */
void StartMLTask(void const * argument)
{
    ml_frame_t frame;
    printf("MLTask iniciada — esperando frames completos...\r\n");

    for (;;)
    {
        /* Bloquear hasta que CANParserTask publique un frame completo */
        if (xQueueReceive(s_mlQueue, &frame, portMAX_DELAY) != pdPASS)
            continue;
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);  /* LED de actividad */
        float speed_pred = PredictSpeed(frame.rms,
                                        frame.crest,
                                        frame.peak,
                                        frame.temperature,
                                        frame.current);

        float error = fabsf(frame.speed - speed_pred);

        if (error > 100.0f)
        {
            printf("ALARMA -> Pred=%.1f Real=%.1f Error=%.1f\r\n",
                   speed_pred, frame.speed, error);
        }
        else
        {
            printf("OK     -> Pred=%.1f Real=%.1f Error=%.1f\r\n",
                   speed_pred, frame.speed, error);
        }
    }
}