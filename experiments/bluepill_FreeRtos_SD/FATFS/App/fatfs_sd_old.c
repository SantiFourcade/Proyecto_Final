#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_spi.h"
#include "stm32f1xx_hal_gpio.h"

#include "diskio.h"
#include "fatfs_sd.h"
#include "stdbool.h"

extern SPI_HandleTypeDef hspi1; 
extern volatile uint8_t Timer1, Timer2;  

static volatile DSTATUS Stat = STA_NOINIT;
static uint8_t CardType;
static uint8_t PowerFlag = 0;

/* --- FUNCIONES DE BAJO NIVEL --- */

static void SELECT(void) {
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
}

static void DESELECT(void) {
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}

static uint8_t SPI_RxByte(void) {
    uint8_t dummy = 0xFF, data = 0;
    HAL_SPI_TransmitReceive(&hspi1, &dummy, &data, 1, SPI_TIMEOUT);
    return data;
}

static void SPI_TxByte(uint8_t data) {
    HAL_SPI_Transmit(&hspi1, &data, 1, SPI_TIMEOUT);
}

static uint8_t SD_ReadyWait(void) {
    uint8_t res;
    Timer2 = 50; // Timeout de 500ms (50 * 10ms aprox, depende de tu lógica de decremento)
    do {
        res = SPI_RxByte();
    } while ((res != 0xFF) && Timer2);
    return res;
}

static void SD_PowerOn(void) {
    uint8_t cmd_arg[6];
    uint32_t wait = 0;

    DESELECT();
    for(int i = 0; i < 10; i++) SPI_TxByte(0xFF); // 80 pulsos de reloj iniciales

    SELECT();
    cmd_arg[0] = (CMD0 | 0x40);
    cmd_arg[1] = cmd_arg[2] = cmd_arg[3] = cmd_arg[4] = 0;
    cmd_arg[5] = 0x95;

    for (int i = 0; i < 6; i++) SPI_TxByte(cmd_arg[i]);

    wait = 0;
    while ((SPI_RxByte() != 0x01) && (wait < 0xFFFF)) wait++;

    DESELECT();
    SPI_TxByte(0xFF);
    PowerFlag = 1;
}

/* --- IMPLEMENTACIÓN DEL DISK I/O --- */

DSTATUS SD_disk_initialize(BYTE drv) {
    uint8_t n, type, ocr[4];

    if (drv) return STA_NOINIT;
    if (Stat & STA_NODISK) return Stat;

    SD_PowerOn();
    SELECT();
    type = 0;

    if (SD_SendCmd(CMD0, 0) == 1) {
        Timer1 = 100; // 1 segundo de timeout
        if (SD_SendCmd(CMD8, 0x1AA) == 1) { // SDC Ver2+
            for (n = 0; n < 4; n++) ocr[n] = SPI_RxByte();
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
                do {
                    if (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 1UL << 30) == 0) break;
                } while (Timer1);
                if (Timer1 && SD_SendCmd(CMD58, 0) == 0) {
                    for (n = 0; n < 4; n++) ocr[n] = SPI_RxByte();
                    type = (ocr[0] & 0x40) ? 6 : 2; // SDHC o SD Ver2
                }
            }
        } else { // SDC Ver1 o MMC
            type = (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 0) <= 1) ? 2 : 1;
            do {
                if (type == 2) {
                    if (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 0) == 0) break;
                } else {
                    if (SD_SendCmd(CMD1, 0) == 0) break;
                }
            } while (Timer1);
            if (!Timer1 || SD_SendCmd(CMD16, 512) != 0) type = 0;
        }
    }

    CardType = type;
    DESELECT();
    SPI_RxByte();

    if (type) Stat &= ~STA_NOINIT;
    else PowerFlag = 0;

    return Stat;
}

DSTATUS SD_disk_status(BYTE drv) {
    if (drv) return STA_NOINIT;
    return Stat;
}

DRESULT SD_disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count) {
    if (pdrv || !count) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    if (!(CardType & 4)) sector *= 512; // Convertir a byte addressing si no es SDHC

    SELECT();
    if (count == 1) {
        if ((SD_SendCmd(CMD17, sector) == 0) && SD_RxDataBlock(buff, 512)) count = 0;
    } else {
        if (SD_SendCmd(CMD18, sector) == 0) {
            do {
                if (!SD_RxDataBlock(buff, 512)) break;
                buff += 512;
            } while (--count);
            SD_SendCmd(CMD12, 0); // Stop Transmission
        }
    }
    DESELECT();
    SPI_RxByte();
    return count ? RES_ERROR : RES_OK;
}

DRESULT SD_disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count) {
    if (pdrv || !count) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    if (!(CardType & 4)) sector *= 512;

    SELECT();
    if (count == 1) {
        if ((SD_SendCmd(CMD24, sector) == 0) && SD_TxDataBlock(buff, 0xFE)) count = 0;
    } else {
        if (CardType & 2) { SD_SendCmd(CMD55, 0); SD_SendCmd(CMD23, count); }
        if (SD_SendCmd(CMD25, sector) == 0) {
            do {
                if(!SD_TxDataBlock(buff, 0xFC)) break;
                buff += 512;
            } while (--count);
            if(!SD_TxDataBlock(0, 0xFD)) count = 1;
        }
    }
    DESELECT();
    SPI_RxByte();
    return count ? RES_ERROR : RES_OK;
}

DRESULT SD_disk_ioctl(BYTE drv, BYTE ctrl, void *buff) {
    DRESULT res = RES_ERROR;
    BYTE n, csd[16];
    DWORD csize;

    if (drv) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    SELECT();
    switch (ctrl) {
        case GET_SECTOR_COUNT:
            if ((SD_SendCmd(CMD9, 0) == 0) && SD_RxDataBlock(csd, 16)) {
                if ((csd[0] >> 6) == 1) { // SDC Ver 2.00
                    csize = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
                    *(DWORD*)buff = csize << 10;
                } else { // SDC Ver 1.XX / MMC
                    n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
                    csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
                    *(DWORD*)buff = csize << (n - 9);
                }
                res = RES_OK;
            }
            break;
        case GET_SECTOR_SIZE: *(WORD*)buff = 512; res = RES_OK; break;
        case CTRL_SYNC: if (SD_ReadyWait() == 0xFF) res = RES_OK; break;
        default: res = RES_PARERR; break;
    }
    DESELECT();
    SPI_RxByte();
    return res;
}

/* --- FUNCIONES PRIVADAS DE DATOS --- */

static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg) {
    uint8_t res, n;
    if (SD_ReadyWait() != 0xFF) return 0xFF;

    SPI_TxByte(cmd);
    SPI_TxByte((uint8_t)(arg >> 24));
    SPI_TxByte((uint8_t)(arg >> 16));
    SPI_TxByte((uint8_t)(arg >> 8));
    SPI_TxByte((uint8_t)arg);
    SPI_TxByte((cmd == CMD0) ? 0x95 : (cmd == CMD8 ? 0x87 : 0x01));

    n = 10;
    do { res = SPI_RxByte(); } while ((res & 0x80) && --n);
    return res;
}

static bool SD_RxDataBlock(BYTE *buff, UINT btr) {
    uint8_t token;
    Timer1 = 10;
    do { token = SPI_RxByte(); } while((token == 0xFF) && Timer1);
    if(token != 0xFE) return false;

    while(btr--) *buff++ = SPI_RxByte();
    SPI_RxByte(); SPI_RxByte(); // Descartar CRC
    return true;
}

static bool SD_TxDataBlock(const BYTE *buff, BYTE token) {
    uint8_t resp;
    if (SD_ReadyWait() != 0xFF) return false;
    SPI_TxByte(token);
    if (token != 0xFD) {
        for(int i=0; i<512; i++) SPI_TxByte(*buff++);
        SPI_RxByte(); SPI_RxByte(); // Dummy CRC
        resp = SPI_RxByte();
        if ((resp & 0x1F) != 0x05) return false;
    }
    return true;
}