#ifndef CAN_FRAME_H
#define CAN_FRAME_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

/* ============================================================
 * IDs CAN — deben coincidir con el nodo transmisor
 * ============================================================ */
#define CAN_ID_RMS      0x101
#define CAN_ID_CREST    0x102
#define CAN_ID_PEAK     0x103
#define CAN_ID_TEMP     0x104
#define CAN_ID_CURRENT  0x105
#define CAN_ID_SPEED    0x106

// Bitmask de campos recibidos
#define FIELD_RMS      (1U << 0)
#define FIELD_CREST    (1U << 1)
#define FIELD_PEAK     (1U << 2)
#define FIELD_TEMP     (1U << 3)
#define FIELD_CURRENT  (1U << 4)
#define FIELD_SPEED    (1U << 5)

#define FRAME_COMPLETE_MASK (FIELD_RMS | FIELD_CREST | FIELD_PEAK | \
                             FIELD_TEMP | FIELD_CURRENT | FIELD_SPEED)

/* Timeout: si no llega el frame completo en este tiempo se descarta.
 * 6 IDs × 10ms separación = 60ms mínimo → 2000ms es conservador. */
#define FRAME_TIMEOUT_MS  2000U

// Frame completo para el modelo
typedef struct {
    float rms;
    float crest;
    float peak;
    float temperature;
    float current;
    float speed;
} ml_frame_t;

// Estado interno del ensamblador
typedef struct {
    ml_frame_t frame;
    uint8_t    received_mask;
    uint32_t   first_field_tick;
} can_frame_assembler_t;

void    FrameAssembler_Init(can_frame_assembler_t *fa);
uint8_t FrameAssembler_Feed(can_frame_assembler_t *fa,
                             uint32_t can_id,
                             float    value,
                             uint32_t now_ms,
                             QueueHandle_t out_queue);
void    FrameAssembler_CheckTimeout(can_frame_assembler_t *fa, uint32_t now_ms);

#endif /* CAN_FRAME_ASSEMBLER_H */