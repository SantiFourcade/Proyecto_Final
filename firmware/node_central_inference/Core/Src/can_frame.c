#include "can_frame.h"
#include <string.h>
#include <stdio.h>
#include "queue.h"

void FrameAssembler_Init(can_frame_assembler_t *fa)
{
    memset(fa, 0, sizeof(can_frame_assembler_t));
}

/* ============================================================
 * FrameAssembler_Feed
 *
 * @param out_queue  Queue donde se publica ml_frame_t cuando el
 *                   frame está completo. MLTask la consume.
 * @retval 1 frame completo publicado en out_queue
 * @retval 0 frame incompleto todavía
 * ============================================================ */
uint8_t FrameAssembler_Feed(can_frame_assembler_t *fa,
                             uint32_t      can_id,
                             float         value,
                             uint32_t      now_ms,
                             QueueHandle_t out_queue)
{
    /* Primer campo del frame — registrar timestamp */
    if (fa->received_mask == 0)
        fa->first_field_tick = now_ms;

    /* Asignar campo y marcar bitmask */
    switch (can_id)
    {
        case CAN_ID_RMS:     fa->frame.rms         = value; fa->received_mask |= FIELD_RMS;     break;
        case CAN_ID_CREST:   fa->frame.crest       = value; fa->received_mask |= FIELD_CREST;   break;
        case CAN_ID_PEAK:    fa->frame.peak        = value; fa->received_mask |= FIELD_PEAK;     break;
        case CAN_ID_TEMP:    fa->frame.temperature = value; fa->received_mask |= FIELD_TEMP;     break;
        case CAN_ID_CURRENT: fa->frame.current     = value; fa->received_mask |= FIELD_CURRENT; break;
        case CAN_ID_SPEED:   fa->frame.speed       = value; fa->received_mask |= FIELD_SPEED;   break;
        default: return 0;  /* ID desconocido */
    }
    //printf("[INFO] Campo recibido: ID=0x%03lX, value=%.2f, mask=0x%02X\r\n",
    //       can_id, value, fa->received_mask);
    if (fa->received_mask != FRAME_COMPLETE_MASK)
        return 0;  /* frame todavía incompleto */
    //printf("[INFO] Frame completo recibido — publicando en MLTask\r\n");
    /* Frame completo — publicar en la queue de MLTask.
     * xQueueOverwrite: si MLTask no consumió el anterior, lo reemplaza.
     * Así nunca bloqueamos CANParserTask por culpa del modelo. */
    xQueueOverwrite(out_queue, &fa->frame);

    /* Resetear para el próximo frame */
    memset(fa, 0, sizeof(can_frame_assembler_t));
    return 1;
}

/* ============================================================
 * FrameAssembler_CheckTimeout
 * Descartar frame parcial si tardó más de FRAME_TIMEOUT_MS.
 * ============================================================ */
void FrameAssembler_CheckTimeout(can_frame_assembler_t *fa, uint32_t now_ms)
{
    if (fa->received_mask == 0)
        return;

    if ((now_ms - fa->first_field_tick) > FRAME_TIMEOUT_MS)
    {
        printf("[WARN] Frame timeout — mask=0x%02X — descartando\r\n",
               fa->received_mask);
        memset(fa, 0, sizeof(can_frame_assembler_t));
    }
}