#include <reg51.h>
#include "type.h"
#include "flash_chip.h"


#define BUFF_LEN 0x10

sbit TEST  = P0 ^ 1;
sbit TEST2 = P0 ^ 0;
static BYTE buff[BUFF_LEN];


static BYTE _read(pAddress a) {
	return buff[a->a8.m0 & (BUFF_LEN-1)];
}


static void _write(pAddress a, BYTE dat) {
	buff[a->a8.m0 & (BUFF_LEN-1)] = dat;
}


static void _s_init() {
	int i;
	for (i=0; i<BUFF_LEN; ++i) {
		TEST = i & 1;
		buff[i] = i;
	}
}


void flash_chip_test(pFlashChip f) {
	f->read = _read;
	f->write = _write;
	f->init = _s_init;
	f->mem_size = 0x10;
}