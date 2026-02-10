/* Incluye la cabecera de la librería */
#include "user_diskio.h"
#include "fatfs_sd.h"
#include <string.h>
#include "ff_gen_drv.h"
#include "spi.h"
/* Estructura que el Linker está buscando */

DSTATUS USER_initialize (BYTE pdrv) {
  return SD_disk_initialize(pdrv);
}

DSTATUS USER_status (BYTE pdrv) {
  return SD_disk_status(pdrv);
}

DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
  return SD_disk_read(pdrv, buff, sector, count);
}

#if _USE_WRITE == 1
DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
  return SD_disk_write(pdrv, buff, sector, count);
}
#endif

#if _USE_IOCTL == 1
DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff) {
  return SD_disk_ioctl(pdrv, cmd, buff);
}
#endif