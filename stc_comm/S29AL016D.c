#include <reg51.h>
#include "type.h"
#include "flash_chip.h"

/*
S29AL016D Flash 连接 STC90: 
    *[A] = 全部引脚,由低到高
    *{N} = 非控制引脚

A0-A7        A8-A15      A16-A19        A-1/DQ15
P0[A]        P1[A]       P3.[2 3 4 5]   P3.6

DQ0-DQ7      CE#         OE#            WE#
P2[A]        P4.4        P4.5           P4.6

RESET#       RY/BY#      BYTE#
{0->1}       P3.7        {0}(8bit)
*/
sfr   F_ADD1    = 0x80;
sfr   F_ADD2    = 0x90;
sfr   F_ADD3    = 0xB0;
sbit  F_ADD3_0  = P3 ^ 2;
sbit  F_ADD3_1  = P3 ^ 3;
sbit  F_ADD3_2  = P3 ^ 4;
sbit  F_ADD3_3  = P3 ^ 5;
sbit  F_ADD3_4  = P3 ^ 6;
sfr   F_DAT     = 0xA0;
sbit  F_CE      = P4 ^ 4;
sbit  F_OE      = P4 ^ 5;
sbit  F_WE      = P4 ^ 6;
sbit  F_READY   = P3 ^ 7;

#define READY 1
#define BUSY 0


static void f_setadder(pAddress a) {
    F_ADD1 = a->a8.m0;
    F_ADD2 = a->a8.m8;
    F_ADD3 = a->a8.m16;
}


static BYTE f_read(pAddress a) {
    while (F_READY == BUSY);
    f_setadder(a);
    F_WE = 1;
    F_CE = 0;
    F_OE = 0;
    return F_DAT;
}


static void f_write(pAddress a, BYTE dat) {
    while (F_READY == BUSY);  
    f_setadder(a);
    F_CE = 0;
    F_OE = 1;
    F_WE = 0;
    F_DAT = dat;
}


void s29al016d(pFlashChip f) {
	f->write = f_write;
	f->read = f_read;
	f->mem_size = 0x200000;
}