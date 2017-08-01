/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/
/* 改文件与 sd 卡驱动链接, 只读取 sd */

#include <diskio.h>
#include <stm32f4xx.h>
#include <stm32f4xx_sdio.h>
#include <misc.h>

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */

#define ATA		0
#define MMC		1
#define USB		2



/*-----------------------------------------------------------------------*/
/* Inicializes a Drive                                                    */

DSTATUS disk_initialize(BYTE drv) {    /* Physical drive nmuber (0..) */
	if (drv == 0) {
		/*
		SD_SDIO_DMA_IRQn 等定义在 STM32F4xx_DSP_StdPeriph_Lib_V1.8.0\Utilities\STM32_EVAL\STM324x7I_EVAL
		这些代码对应 st 自己的开发板, 移植需要修改.
		*/
		NVIC_InitTypeDef NVIC_InitStructure;

		/* Configure the NVIC Preemption Priority Bits */
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

		NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
//		NVIC_InitStructure.NVIC_IRQChannel = SD_SDIO_DMA_IRQn; // !! 问题未解决
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
		NVIC_Init(&NVIC_InitStructure);  
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status(BYTE drv) {		/* Physical drive nmuber (0..) */
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

/* drv -- Physical drive nmuber (0..) */
/* buf -- Data buffer to store read data */
/* sec -- Sector address (LBA) */
/* cnt -- Number of sectors to read (1..255) */
DRESULT disk_read(BYTE drv, BYTE *buf, DWORD sec, BYTE cnt) {
	DRESULT res = RES_PARERR;
	// SD_ReadMultiBlocks();
	return res;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
/* drv -- Physical drive nmuber (0..) */
/* buf -- Data buffer to store read data */
/* sec -- Sector address (LBA) */
/* cnt -- Number of sectors to read (1..255) */
DRESULT disk_write(BYTE drv, const BYTE *buf, DWORD sec, BYTE cnt) {
	DRESULT res = RES_PARERR;
	return res;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

/* drv  -- Physical drive nmuber (0..) */
/* ctrl -- Control code */
/* buf  -- Buffer to send/receive control data */
DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void *buf) {
	DRESULT res = RES_PARERR;
	return res;
}


DWORD get_fattime() {
	return 0;
}
