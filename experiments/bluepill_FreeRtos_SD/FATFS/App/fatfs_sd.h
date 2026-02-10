/* * fatfs_sd.h 
 * Driver para tarjetas SD sobre SPI compatible con FatFS y FreeRTOS 
 */

#ifndef __FATFS_SD_H
#define __FATFS_SD_H

#include "stm32f1xx_hal.h"
#include "diskio.h"
#include "stdbool.h"


/* --- CONFIGURACIÓN DE HARDWARE --- */
extern SPI_HandleTypeDef hspi1;           // El SPI que configuraste en CubeMX
#define SD_CS_GPIO_Port   GPIOA           // Puerto del Chip Select
#define SD_CS_Pin         GPIO_PIN_4      // Pin del Chip Select
#define SPI_TIMEOUT       100             // Timeout para funciones HAL

/* --- DEFINICIÓN DE COMANDOS SD  --- */
#define CMD0     (0)           /* GO_IDLE_STATE - Reset la tarjeta */
#define CMD1     (1)           /* SEND_OP_COND - Iniciar inicialización */
#define CMD8     (8)           /* SEND_IF_COND - Verificar voltaje (SDHC) */
#define CMD9     (9)           /* SEND_CSD - Leer información de la tarjeta */
#define CMD10    (10)          /* SEND_CID - Leer identificación de la tarjeta */
#define CMD12    (12)          /* STOP_TRANSMISSION - Parar lectura múltiple */
#define CMD16    (16)          /* SET_BLOCKLEN - Fijar tamaño de bloque (512) */
#define CMD17    (17)          /* READ_SINGLE_BLOCK - Leer un sector */
#define CMD18    (18)          /* READ_MULTIPLE_BLOCK - Leer varios sectores */
#define CMD23    (23)          /* SET_BLOCK_COUNT - Definir bloques para escribir */
#define CMD24    (24)          /* WRITE_BLOCK - Escribir un sector */
#define CMD25    (25)          /* WRITE_MULTIPLE_BLOCK - Escribir varios sectores */
#define CMD41    (41)          /* ACMD41: SD_SEND_OP_COND - Inicialización operativa */
#define CMD55    (55)          /* APP_CMD - Prefijo para comandos ACMD */
#define CMD58    (58)          /* READ_OCR - Leer registro de capacidad */

DSTATUS SD_disk_initialize (BYTE pdrv);
DSTATUS SD_disk_status (BYTE pdrv);
DRESULT SD_disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT SD_disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DRESULT SD_disk_ioctl (BYTE pdrv, BYTE ctrl, void* buff);

/* --- FUNCIONES INTERNAS DEL DRIVER (AUXILIARES) --- */
static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg);
static bool SD_RxDataBlock(BYTE *buff, UINT btr);
static bool SD_TxDataBlock(const BYTE *buff, BYTE token);

#endif /* __FATFS_SD_H */