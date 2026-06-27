#define TRUE 1
#define FALSE 0
#define bool BYTE

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_spi.h"
#include "stm32f1xx_hal_gpio.h"
#include "main.h"

#include "diskio.h"
#include "fatfs_sd.h"

extern SPI_HandleTypeDef hspi1;
extern volatile uint16_t Timer1, Timer2;     // Timer que decrementa cada 1 ms

static volatile DSTATUS Stat = STA_NOINIT;  // Flag de estado del disco
static uint8_t CardType;                    // Tipo SD  0:MMC, 1:SDC, 2:Block addressing */
static uint8_t PowerFlag = 0;               // Power Flag

/* SPI Chip Select */
static void SELECT(void)
{
  HAL_GPIO_WritePin(CS_SPI_GPIO_Port, CS_SPI_Pin, GPIO_PIN_RESET);
}

/* SPI Chip Deselect */
static void DESELECT(void)
{
  HAL_GPIO_WritePin(CS_SPI_GPIO_Port, CS_SPI_Pin, GPIO_PIN_SET);
}

/* Transferencia de datos SPI */
static void SPI_TxByte(BYTE data)
{
  while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY)
    ;
  HAL_SPI_Transmit(&hspi1, &data, 1, SPI_TIMEOUT);
}

/* Función tipo retorno para envío y recepción SPI */
static uint8_t SPI_RxByte(void)
{
  uint8_t dummy, data;
  dummy = 0xFF;
  data = 0;

  while ((HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY))
    ;
  HAL_SPI_TransmitReceive(&hspi1, &dummy, &data, 1, SPI_TIMEOUT);

  return data;
}

/* Función de tipo puntero para envío y recepción SPI */
static void SPI_RxBytePtr(uint8_t *buff)
{
  *buff = SPI_RxByte();
}

/* Espera de SD Card Ready (Lista) */
static uint8_t SD_ReadyWait(void)
{
  uint8_t res;

  Timer2 = 500;
  SPI_RxByte();

  do
  {
    // Comunicación SPI hasta que se reciba el valor 0xFF
    res = SPI_RxByte();
  } while ((res != 0xFF) && Timer2);

  return res;
}

/* Encendido */
static void SD_PowerOn(void)
{
  uint8_t cmd_arg[6];
  uint32_t Count = 0x1FFF;

  // Envía un mensaje SPI en estado Deselect (deseleccionado) para ponerlo en modo de espera (standby)
  DESELECT();

  for (int i = 0; i < 10; i++)
  {
    SPI_TxByte(0xFF);
  }

  // SPI Chips Select 
  SELECT();

  // Transicion al estado GO_IDLE_STATE
  cmd_arg[0] = (CMD0 | 0x40);
  cmd_arg[1] = 0;
  cmd_arg[2] = 0;
  cmd_arg[3] = 0;
  cmd_arg[4] = 0;
  cmd_arg[5] = 0x95;

  // Envío de comando
  for (int i = 0; i < 6; i++)
  {
    SPI_TxByte(cmd_arg[i]);
  }

  // Espera de respuesta
  while ((SPI_RxByte() != 0x01) && Count)
  {
    Count--;
  }

  DESELECT();
  SPI_TxByte(0XFF);

  PowerFlag = 1;
}

/* Apagado */
static void SD_PowerOff(void)
{
  PowerFlag = 0;
}

/* Verificación del estado de alimentación */
static uint8_t SD_CheckPower(void)
{
  //  0=off, 1=on
  return PowerFlag;
}

/* Recepción de paquetes de datos */
static bool SD_RxDataBlock(BYTE *buff, UINT btr)
{
  uint8_t token;

  Timer1 = 100;

  // Espera de respuesta
  do
  {
    token = SPI_RxByte();
  } while ((token == 0xFF) && Timer1);

  // Manejo de errores al recibir un token distinto de 0xFE
  if (token != 0xFE)
    return FALSE;

  // Recepción de datos en el búfer
  while (btr--)
  {
    SPI_RxBytePtr(buff++);
  }

  SPI_RxByte(); // Ignorar CRC
  SPI_RxByte();

  return TRUE;
}

/* Paquete de transferencia de datos */
#if _READONLY == 0
static bool SD_TxDataBlock(const BYTE *buff, BYTE token)
{
  uint8_t resp = 0xFF, i = 0; // Inicialización de respuesta
  uint16_t wc;

  // Espera de preparación de la tarjeta SD
  if (SD_ReadyWait() != 0xFF)
    return FALSE;

  // Envío de token
  SPI_TxByte(token);

  // En caso de ser un token de datos
  if (token != 0xFD)
  {
    wc = 512;

    // Transferencia de datos de 512 bytes
    while (wc--)
    {
      SPI_TxByte(*buff++);
    }

    SPI_RxByte(); // Ignorar CRC
    SPI_RxByte();

    // Recepción de respuesta de datos
    while (i <= 64)
    {
      resp = SPI_RxByte();

      if ((resp & 0x1F) == 0x05)
        break;

      i++;
    }

    // Limpieza del búfer de recepción SPI - Tiempo de espera (timeout) añadido
    Timer1 = 200;
    while (SPI_RxByte() == 0 && Timer1)
      ;
  }

  if ((resp & 0x1F) == 0x05)
    return TRUE;
  else
    return FALSE;
}
#endif /* _READONLY */

/* Envío de paquete de comando (CMD) */
static BYTE SD_SendCmd(BYTE cmd, DWORD arg)
{
  uint8_t crc, res;

  if (SD_ReadyWait() != 0xFF)
    return 0xFF;

  // Envío de paquete de comando
  SPI_TxByte(cmd);               // Command
  SPI_TxByte((BYTE)(arg >> 24)); // Argument[31..24]
  SPI_TxByte((BYTE)(arg >> 16)); // Argument[23..16]
  SPI_TxByte((BYTE)(arg >> 8));  // Argument[15..8]
  SPI_TxByte((BYTE)arg);         // Argument[7..0]
  // Preparación de CRC según comando
  crc = 0;
  if (cmd == CMD0)
    crc = 0x95; // CRC for CMD0(0)

  if (cmd == CMD8)
    crc = 0x87; // CRC for CMD8(0x1AA)

  // Transferencia CRC
  SPI_TxByte(crc);

  // En caso del comando CMD12 (Stop Reading), se descarta un byte de respuesta
  if (cmd == CMD12)
    SPI_RxByte();

  // Se reciben datos normales dentro de 10 intentos
  uint8_t n = 10;
  do
  {
    res = SPI_RxByte();
  } while ((res & 0x80) && --n);

  return res;
}

/*-----------------------------------------------------------------------
  Funciones globales utilizadas en FatFS. Se utilizan en el archivo user_diskio.c.
-----------------------------------------------------------------------*/

/* Inicialización del disco SD */
DSTATUS SD_disk_initialize(BYTE drv)
{
  uint8_t n, type, ocr[4];

  // Soporte para un solo tipo de unidad
  if (drv)
    return STA_NOINIT;

  // Tarjeta SD no insertada
  if (Stat & STA_NODISK)
    return Stat;

  // SD Power On
  SD_PowerOn();

  // SPI Chip Select
  SELECT();

  // Inicialización de la variable de tipo de tarjeta SD
  type = 0;

  // Entrada al estado Idle
  if (SD_SendCmd(CMD0, 0) == 1)
  {

    Timer1 = 1000;

    // Verificación de las condiciones de funcionamiento de la interfaz SD
    if (SD_SendCmd(CMD8, 0x1AA) == 1)
    {
      // SDC Ver2+
      for (n = 0; n < 4; n++)
      {
        ocr[n] = SPI_RxByte();
      }

      if (ocr[2] == 0x01 && ocr[3] == 0xAA)
      {
        // Rango de voltaje de operación entre 2.7 y 3.6V
        do
        {
          if (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 1UL << 30) == 0)
            break; // ACMD41 with HCS bit
        } while (Timer1);

        if (Timer1 && SD_SendCmd(CMD58, 0) == 0)
        {
          // Check CCS bit
          for (n = 0; n < 4; n++)
          {
            ocr[n] = SPI_RxByte();
          }

          type = (ocr[0] & 0x40) ? 6 : 2;
        }
      }
    }
    else
    {
      // SDC Ver1 or MMC
      type = (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 0) <= 1) ? 2 : 1; /* SDC : MMC */

      do
      {
        if (type == 2)
        {
          if (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 0) == 0)
            break; // ACMD41 
        }
        else
        {
          if (SD_SendCmd(CMD1, 0) == 0)
            break; // CMD1
        }
      } while (Timer1);

      if (!Timer1 || SD_SendCmd(CMD16, 512) != 0)
      {
        // Selección de la longitud del bloque
        type = 0;
      }
    }
  }

  CardType = type;

  DESELECT();

  SPI_RxByte(); // Transición a Idle (Release DO)

  if (type)
  {
    // Clear STA_NOINIT
    Stat &= ~STA_NOINIT;
  }
  else
  {
    // Initialization failed
    SD_PowerOff();
  }

  return Stat;
}

/* Verificación del estado del disco */
DSTATUS SD_disk_status(BYTE drv)
{
  if (drv)
    return STA_NOINIT;

  return Stat;
}

/* Lectura del sector */
DRESULT SD_disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
  if (pdrv || !count)
    return RES_PARERR;

  if (Stat & STA_NOINIT)
    return RES_NOTRDY;

  if (!(CardType & 4))
    sector *= 512; // Conversión del sector especificado a unidades de direccionamiento por bytes (Byte addressing)

  SELECT();

  if (count == 1)
  {
    // Lectura de bloque único
    if ((SD_SendCmd(CMD17, sector) == 0) && SD_RxDataBlock(buff, 512))
      count = 0;
  }
  else
  {
    // Lectura de múltiples bloques
    if (SD_SendCmd(CMD18, sector) == 0)
    {
      do
      {
        if (!SD_RxDataBlock(buff, 512))
          break;

        buff += 512;
      } while (--count);

      // STOP_TRANSMISSION, todos los bloques fueron leídos, se detiene la transmisión
      SD_SendCmd(CMD12, 0);
    }
  }

  DESELECT();
  SPI_RxByte(); // Transición a Idle (Release DO)

  return count ? RES_ERROR : RES_OK;
}

/* Escritura del sector */
#if _READONLY == 0
DRESULT SD_disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
  if (pdrv || !count)
    return RES_PARERR;

  if (Stat & STA_NOINIT)
    return RES_NOTRDY;

  if (Stat & STA_PROTECT)
    return RES_WRPRT;

  if (!(CardType & 4))
    sector *= 512; // Conversión del sector especificado a unidades de direccionamiento por bytes (Byte addressing) 

  SELECT();

  if (count == 1)
  {
    // Escritura de bloque único
    if ((SD_SendCmd(CMD24, sector) == 0) && SD_TxDataBlock(buff, 0xFE))
      count = 0;
  }
  else
  {
    // Escritura de múltiples bloques
    if (CardType & 2)
    {
      SD_SendCmd(CMD55, 0);
      SD_SendCmd(CMD23, count); // ACMD23
    }

    if (SD_SendCmd(CMD25, sector) == 0)
    {
      do
      {
        if (!SD_TxDataBlock(buff, 0xFC))
          break;

        buff += 512;
      } while (--count);

      if (!SD_TxDataBlock(0, 0xFD))
      {
        count = 1;
      }
    }
  }

  DESELECT();
  SPI_RxByte();

  return count ? RES_ERROR : RES_OK;
}
#endif /* _READONLY */

/* Otras funciones */
DRESULT SD_disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
  DRESULT res;
  BYTE n, csd[16], *ptr = buff;
  WORD csize;

  if (drv)
    return RES_PARERR;

  res = RES_ERROR;

  if (ctrl == CTRL_POWER)
  {
    switch (*ptr)
    {
    case 0:
      if (SD_CheckPower())
        SD_PowerOff(); // Power Off
      res = RES_OK;
      break;
    case 1:
      SD_PowerOn(); // Power On
      res = RES_OK;
      break;
    case 2:
      *(ptr + 1) = (BYTE)SD_CheckPower();
      res = RES_OK; // Power Check
      break;
    default:
      res = RES_PARERR;
    }
  }
  else
  {
    if (Stat & STA_NOINIT)
      return RES_NOTRDY;

    SELECT();

    switch (ctrl)
    {
    case GET_SECTOR_COUNT:
      // Número de sectores en la tarjeta SD (DWORD)
      if ((SD_SendCmd(CMD9, 0) == 0) && SD_RxDataBlock(csd, 16))
      {
        if ((csd[0] >> 6) == 1)
        {
          // SDC ver 2.00 - Parsing preciso del campo C_SIZE
          csize = ((DWORD)(csd[7] & 0x3F) << 16) | ((DWORD)csd[8] << 8) | csd[9];
          *(DWORD *)buff = (csize + 1) << 10;
        }
        else
        {
          // MMC or SDC ver 1.XX 
          n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
          csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
          *(DWORD *)buff = (DWORD)csize << (n - 9);
        }

        res = RES_OK;
      }
      break;

    case GET_SECTOR_SIZE:
      // Tamaño de unidad del sector (WORD)
      *(WORD *)buff = 512;
      res = RES_OK;
      break;

    case CTRL_SYNC:
      // Sincronización de escritura
      if (SD_ReadyWait() == 0xFF)
        res = RES_OK;
      break;

    case MMC_GET_CSD:
      // CSD información recibida (16 bytes) 
      if (SD_SendCmd(CMD9, 0) == 0 && SD_RxDataBlock(ptr, 16))
        res = RES_OK;
      break;

    case MMC_GET_CID:
      // CID información recibida (16 bytes) 
      if (SD_SendCmd(CMD10, 0) == 0 && SD_RxDataBlock(ptr, 16))
        res = RES_OK;
      break;

    case MMC_GET_OCR:
      // OCR información recibida (4 bytes) 
      if (SD_SendCmd(CMD58, 0) == 0)
      {
        for (n = 0; n < 4; n++)
        {
          *ptr++ = SPI_RxByte();
        }

        res = RES_OK;
      }
      break;

    default:
      res = RES_PARERR;
    }

    DESELECT();
    SPI_RxByte();
  }

  return res;
}