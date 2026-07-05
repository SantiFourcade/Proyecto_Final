/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications (Optimized & Decoupled)
  *                       + Gating de adquisición por broadcast CAN
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "queue.h"
#include "can.h"
#include "adxl345.h"
#include "math.h"

#include "i2c.h"
#include "usart.h"
#include "adc.h"

#include "features.h"
#include "can_app.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern volatile uint8_t  flag_pulso;
extern volatile uint32_t diff_ticks;
extern volatile uint8_t  new_read;
extern volatile uint16_t adc_buffer_corriente[100];
extern volatile uint8_t  buffer_listo;

static volatile float s_temp_filtrada   = 0.0f;
static volatile float s_corriente_actual = 0.0f;
static volatile float s_rpm_global       = 0.0f;

/* Contador de drops para diagnóstico */
//static volatile uint32_t s_rawQueue_drops = 0;

extern volatile uint8_t g_acquisition_active;   // Activacion de adquisición: 1=activo, 0=inactivo

/* USER CODE END Variables */

/* Thread handles */
osThreadId accelHandle;
osThreadId telemetryHandle;
osThreadId packageHandle;
osThreadId txCANHandle;
osThreadId samplerHandle;

/* Queue handles */
QueueHandle_t rawQueue;
QueueHandle_t canQueue;

extern CAN_HandleTypeDef hcan;
extern I2C_HandleTypeDef hi2c1;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

/* Private function prototypes -----------------------------------------------*/
float Procesar_Corriente_RMS(void);
void  I2C_Reset_Bus_Routine(void);
void  I2C_DelayUs(uint32_t us);   /* [FIX-3] delay sin SysTick */

/* Helpers de acceso seguro a telemetría [FIX-1] */
static inline float Telemetry_GetTemp(void);
static inline float Telemetry_GetCurrent(void);
static inline float Telemetry_GetRPM(void);
static inline void  Telemetry_SetTemp(float v);
static inline void  Telemetry_SetCurrent(float v);
static inline void  Telemetry_SetRPM(float v);

void StartAccelTask(void const * argument);
void StartTelemetryTask(void const * argument);
void StartPackageTask(void const * argument);
void StartTxCAN(void const * argument);
void StartSampler(void const * argument);
void MX_FREERTOS_Init(void);

/* Static Allocation Support for Idle Task */
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

static inline float Telemetry_GetTemp(void)
{
    float v;
    taskENTER_CRITICAL();
    v = s_temp_filtrada;
    taskEXIT_CRITICAL();
    return v;
}

static inline float Telemetry_GetCurrent(void)
{
    float v;
    taskENTER_CRITICAL();
    v = s_corriente_actual;
    taskEXIT_CRITICAL();
    return v;
}

static inline float Telemetry_GetRPM(void)
{
    float v;
    taskENTER_CRITICAL();
    v = s_rpm_global;
    taskEXIT_CRITICAL();
    return v;
}

static inline void Telemetry_SetTemp(float v)
{
    taskENTER_CRITICAL();
    s_temp_filtrada = v;
    taskEXIT_CRITICAL();
}

static inline void Telemetry_SetCurrent(float v)
{
    taskENTER_CRITICAL();
    s_corriente_actual = v;
    taskEXIT_CRITICAL();
}

static inline void Telemetry_SetRPM(float v)
{
    taskENTER_CRITICAL();
    s_rpm_global = v;
    taskEXIT_CRITICAL();
}

void MX_FREERTOS_Init(void)
{
    rawQueue = xQueueCreate(2, sizeof(raw_data_t));
    canQueue = xQueueCreate(5, sizeof(data_frame_t));

    configASSERT(rawQueue);
    configASSERT(canQueue);

    /* Sampler – Alta prioridad, dispara ADC1 cada 1 ms */
    osThreadDef(sampler, StartSampler, osPriorityHigh, 0, 128);
    samplerHandle = osThreadCreate(osThread(sampler), NULL);

    /* Accel – I2C exclusivo, 50 Hz */
    osThreadDef(accelTask, StartAccelTask, osPriorityNormal, 0, 512);
    accelHandle = osThreadCreate(osThread(accelTask), NULL);

    /* Telemetry – ADC2 polling + RPM, prioridad baja */
    osThreadDef(telemetryTask, StartTelemetryTask, osPriorityBelowNormal, 0, 256);
    telemetryHandle = osThreadCreate(osThread(telemetryTask), NULL);

    /* Package – Procesamiento matemático */
    osThreadDef(package, StartPackageTask, osPriorityNormal, 0, 256);
    packageHandle = osThreadCreate(osThread(package), NULL);

    /* TxCAN – Despacho de tramas */
    osThreadDef(txCAN, StartTxCAN, osPriorityNormal, 0, 512);
    txCANHandle = osThreadCreate(osThread(txCAN), NULL);
}

/* ============================================================
 * StartSampler
 * Dispara ADC1 con interrupción cada 1 ms exacto.
 * ============================================================ */
void StartSampler(void const * argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1);
    uint8_t estaba_activo = 0;

    for (;;)
    {
        if (!g_acquisition_active)
        {
            estaba_activo = 0;
            vTaskDelay(pdMS_TO_TICKS(20));   /* inactivo: no golpear el ADC */
            continue;
        }

        if (!estaba_activo)
        {
            xLastWakeTime = xTaskGetTickCount();  /* re-sincronizar el período */
            estaba_activo = 1;
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        HAL_ADC_Start_IT(&hadc1);
    }
}

/* ============================================================
 * StartAccelTask
 * Tarea dedicada exclusivamente al bus I2C del ADXL345.
 * ============================================================ */
void StartAccelTask(void const * argument)
{
    static raw_data_t raw;
    uint16_t idx = 0;
    static uint32_t lecturas_ok = 0;
    static uint32_t lecturas_err = 0;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(20);
    uint8_t estaba_activo = 0;

    for (;;)
    {
        if (!g_acquisition_active)
        {
            idx = 0;               /* descartar ventana parcial */
            estaba_activo = 0;
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if (!estaba_activo)
        {
            xLastWakeTime = xTaskGetTickCount();
            estaba_activo = 1;
        }

        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        int16_t ax, ay, az;
        HAL_StatusTypeDef st = ADXL345_ReadXYZ(&ax, &ay, &az);

        // Caso 1: bus I2C roto → recuperar bus completo
        if (st != HAL_OK)
        {
            printf("[I2C Error] HAL st=%d  state=0x%02X\r\n",
                (int)st, (unsigned)hi2c1.State);
            I2C_Reset_Bus_Routine();  // pulsos SCL + reinit periférico + reinit sensor
            continue;
        }

        // Caso 2: bus OK pero sensor en standby (brownout) → solo reinit sensor
        int32_t mag_sq = (int32_t)ax*ax + (int32_t)ay*ay + (int32_t)az*az;
        if (mag_sq < 100*100)
        {
            printf("[ADXL Brownout] datos=(%d,%d,%d) — reiniciando sensor...\r\n",
                ax, ay, az);
            ADXL345_Init(&hi2c1);   // solo reescribir registros, no tocar el bus
            osDelay(10);      // dar tiempo al sensor para salir del standby
            continue;
        }

        // Datos válidos — procesar normalmente
        raw.accel.ax[idx] = ax;
        raw.accel.ay[idx] = ay;
        raw.accel.az[idx] = az;

        idx++;
        if (idx >= WINDOW_SIZE)
        {
            if (xQueueSend(rawQueue, &raw, pdMS_TO_TICKS(10)) != pdPASS)
                printf("[WARN] rawQueue drop\r\n");
            idx = 0;
        }
    }
}
/* ============================================================
 * StartTelemetryTask
 * Maneja RPM (pulsos) y telemetría lenta (ADC2 temperatura).
 * [NUEVO] Espera bloqueante al broadcast CAN de activación.
 * ============================================================ */
void StartTelemetryTask(void const * argument)
{
    printf("Iniciando Telemetría Lenta (ADC2 / RPM)...\r\n");

    uint32_t acumulador_temp      = 0;
    uint32_t cuenta_muestras_temp = 0;
    uint32_t acumulador_ticks     = 0;
    uint8_t  contador_pulsos      = 0;
    uint8_t  estaba_activo        = 0;
    static TickType_t ultimo_pulso = 0;

    for (;;)
    {
        if (!g_acquisition_active)
        {
            /* Adquisición detenida: limpiar acumuladores y dormir */
            acumulador_temp      = 0;
            cuenta_muestras_temp = 0;
            acumulador_ticks     = 0;
            contador_pulsos      = 0;
            Telemetry_SetRPM(0.0f);
            estaba_activo        = 0;

            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if (!estaba_activo)
        {
            ultimo_pulso = xTaskGetTickCount();
            taskENTER_CRITICAL();
            new_read = 0;
            taskEXIT_CRITICAL();
            estaba_activo = 1;
        }

        uint8_t hubo_pulso;
        uint32_t ticks_capturados;

        taskENTER_CRITICAL();
        hubo_pulso = new_read;
        if (hubo_pulso)
        {
            ticks_capturados = diff_ticks;
            new_read = 0;
        }
        taskEXIT_CRITICAL();

        if (hubo_pulso)
        {
            ultimo_pulso = xTaskGetTickCount();
            acumulador_ticks += ticks_capturados;
            contador_pulsos++;

            if (contador_pulsos >= 8)
            {
                float promedio = (float)acumulador_ticks / 8.0f;
                Telemetry_SetRPM(7500000.0f / promedio);
                acumulador_ticks = 0;
                contador_pulsos  = 0;
            }
        }
        if ((xTaskGetTickCount() - ultimo_pulso) > pdMS_TO_TICKS(1000)) {
            Telemetry_SetRPM(0.0f);
        }

        uint8_t buffer_disponible;

        taskENTER_CRITICAL();
        buffer_disponible = buffer_listo;
        if (buffer_disponible)
            buffer_listo = 0;   /* limpieza atómica */
        taskEXIT_CRITICAL();

        if (buffer_disponible)
        {

            Telemetry_SetCurrent(Procesar_Corriente_RMS());

            HAL_ADC_Start(&hadc2);
            if (HAL_ADC_PollForConversion(&hadc2, 2) == HAL_OK)
            {
                acumulador_temp += HAL_ADC_GetValue(&hadc2);
                cuenta_muestras_temp++;
            }
            HAL_ADC_Stop(&hadc2);

            if (cuenta_muestras_temp >= 32)
            {
                float promedio_adc = (float)acumulador_temp / 32.0f;
                float voltaje_mv   = promedio_adc * (3262.0f / 4096.0f);
                /* [FIX-1] escritura segura de temperatura */
                Telemetry_SetTemp(voltaje_mv / 10.0f);

                acumulador_temp      = 0;
                cuenta_muestras_temp = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/* ============================================================
 * StartPackageTask
 * Recibe ventana cruda, calcula features y encola trama CAN.
 * ============================================================ */
void StartPackageTask(void const * argument)
{
    printf("Package task started\r\n");

    raw_data_t   raw;
    data_frame_t frame;

    for (;;)
    {
        xQueueReceive(rawQueue, &raw, portMAX_DELAY);

        Features_ComputeRMSPeak(&raw.accel, &frame.vib);

        frame.temperature = Telemetry_GetTemp();
        frame.current     = Telemetry_GetCurrent();
        frame.speed       = Telemetry_GetRPM();

        xQueueSend(canQueue, &frame, portMAX_DELAY);
    }
}

/* ============================================================
 * StartTxCAN
 * Despacha una trama CAN por mensaje recibido de canQueue.
 * Igual que Package: bloqueada en xQueueReceive, sin gating
 * explícito necesario.
 * ============================================================ */
void StartTxCAN(void const * argument)
{
    data_frame_t frame;
    printf("TxCAN task started\r\n");

    for (;;)
    {
        xQueueReceive(canQueue, &frame, portMAX_DELAY);

        CAN_SendFloat(0x101, frame.vib.rms);     vTaskDelay(pdMS_TO_TICKS(10));
        CAN_SendFloat(0x102, frame.vib.crest);   vTaskDelay(pdMS_TO_TICKS(10));
        CAN_SendFloat(0x103, frame.vib.peak);    vTaskDelay(pdMS_TO_TICKS(10));
        CAN_SendFloat(0x104, frame.temperature); vTaskDelay(pdMS_TO_TICKS(10));
        CAN_SendFloat(0x105, frame.current);     vTaskDelay(pdMS_TO_TICKS(10));
        CAN_SendFloat(0x106, frame.speed);       vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ============================================================
 * Procesar_Corriente_RMS
 * Calcula la corriente eficaz sobre el buffer DMA de 100 muestras.
 * ============================================================ */
float Procesar_Corriente_RMS(void)
{
    float suma_cuadrados = 0.0f;
    for (int i = 0; i < 100; i++)
    {
        int32_t centrado = (int32_t)adc_buffer_corriente[i] - 2048;
        suma_cuadrados  += (float)(centrado * centrado);
    }
    float rms_raw = sqrtf(suma_cuadrados / 100.0f);
    return (rms_raw * (3.262f / 4096.0f));
}