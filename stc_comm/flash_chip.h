#ifndef _FLASH_CHIP_H_
#define _FLASH_CHIP_H_

#include "uart.h"


typedef union _address {
	ADDR a32;

	struct {
		WORD h;
		WORD l; } a16;

	struct {
		BYTE m24;
		BYTE m16;
		BYTE m8;
		BYTE m0; } a8;

} Address, *pAddress;
   

typedef struct _flash_chip {
	BYTE (*read)   (pAddress addr);
	void (*write)  (pAddress addr, BYTE dat);
	void (*init)   (BYTE a, BYTE b);
    void (*flush)  ();
	BYTE state;
	ADDR mem_size;
} FlashChip, *pFlashChip;


void flash_T1();
void flash_T2();
void flash_T3();
void flash_T4();
void null_func();


void s29al016d(pFlashChip);
void at24c64a(pFlashChip);
void flash_chip_test(pFlashChip);
void AT45DB161B(pFlashChip);


#endif