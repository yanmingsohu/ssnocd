/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/
/* 改文件与 sd 卡驱动链接, 只读取 sd */

#include <misc.h>
#include <diskio.h>
#include <stm324x7i_eval_sdio_sd.h>
#include <ffconf.h>

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */

#define MMC		0



/*-----------------------------------------------------------------------*/
/* Inicializes a Drive                                                   */

DSTATUS disk_initialize(BYTE drv) {    /* Physical drive nmuber (0..) */
  if (drv != 0)
    return STA_NODISK;
	
  if (SD_OK == SD_Init()) {
    return RES_OK;
  }
	
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status(BYTE drv) {		/* Physical drive nmuber (0..) */
  if (drv != 0)
    return STA_NODISK;
  
  if (SD_CARD_ERROR == SD_GetState())
    return STA_NODISK;
  
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

/* drv -- Physical drive nmuber (0..) */
/* buf -- Data buffer to store read data */
/* sec -- Sector address (LBA) */
/* cnt -- Number of sectors to read (1..255) */
DRESULT disk_read(BYTE drv, BYTE *buf, DWORD sec, BYTE cnt) {
  if (drv != 0)
    return RES_NOTRDY;
      
  if (SD_OK == SD_ReadMultiBlocks(buf, sec * _MAX_SS, _MAX_SS, cnt))
    return RES_OK;
    
	return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
/* drv -- Physical drive nmuber (0..) */
/* buf -- Data buffer to store read data */
/* sec -- Sector address (LBA) */
/* cnt -- Number of sectors to read (1..255) */
DRESULT disk_write(BYTE drv, const BYTE *buf, DWORD sec, BYTE cnt) {
	if (drv != 0)
    return RES_NOTRDY;
      
  if (SD_OK == SD_WriteMultiBlocks((uint8_t*) buf, sec * _MAX_SS, _MAX_SS, cnt)) 
    return RES_OK;
    
	return RES_ERROR;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

/* drv  -- Physical drive nmuber (0..) */
/* ctrl -- Control code */
/* buf  -- Buffer to send/receive control data */
DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void *buf) {
  if (drv != 0)
    return RES_NOTRDY;
  
  switch (ctrl) {
    case GET_SECTOR_SIZE:
      return _MAX_SS;
  }
  
  return RES_ERROR;
}


DWORD get_fattime() {
	return 0;
}
