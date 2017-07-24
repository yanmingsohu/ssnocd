/*******************************************************************************/    
/* ����������ѹΪ2.7~3.6V��ʵ���з��ֵ���ѹ����4.25V�������״̬�ֽ�Ϊ9A������ */    
/* ��״̬�ֽ�ֵΪ9D�������Ҷ�д���ݾ���׼ȷ������Ӧ����֤��Ƭ�Ĺ����ѹ������  */    
/* 4.25V��                                                                     */    
/* SPI�淶��Data is always clocked into the device on the rising edge of SCK a-  
/*    nd clocked out of the device on the falling edge of SCK.All instruction-  
/*    s,addresses and data are transferred with the most significant bit(MSB)   
/*    first.                                                                    
/*                                                                  2005-06-02  
/ ******************************************************************************/    

#include <reg51.h>
#include "type.h"
#include "flash_chip.h"

#define DONT_CARE   0x00
#define BEGIN_SPI	SPI_SCK = 0; SPI_CS = 0
#define OVER_SPI	SPI_CS = 1

sbit        SPI_SCK         = P3 ^ 6;   //����ʱ��   
sbit        SPI_SO          = P4 ^ 6;   //�������   
sbit        SPI_SI          = P4 ^ 4;   //��������   
sbit        SPI_CS          = P3 ^ 5;   //Ƭѡ��0   ��Ч   

/*
sbit        SPI_REST        = P2^2;   //оƬ�ظ�λ   
sbit        SPI_WP          = P2^7;   //д����0����������д , 
                                      //�������ڲ������ߣ���������
*/

/* ------------------------------------------------------------- spi base ----*/

/* ��һ�ֽ� */
static BYTE spiRead()                              
{    
    BYTE i, rByte = 0;
	SPI_SO = 1;
	    
    for(i=0; i<8; i++) { 
        SPI_SCK = 1;  
        rByte = (rByte << 1) | SPI_SO;
        SPI_SCK = 0;
    }    
    return rByte;        
}    

/* дһ�ֽ� */
static void spiWrite(BYTE wByte) {    
    BYTE i;       

    for(i=0; i<8; i++) {  
        if(wByte & 0x80) SPI_SI = 1;   
        else SPI_SI = 0;
        SPI_SCK = 1;   
        SPI_SCK = 0;
		wByte <<= 1; 
    }   
}

/* ----------------------------------------- AT 45 api ---------------------- */

/******************************************************************************/    
/*Status Register Format:  ״̬��������ʽ                                     */    
/*   -----------------------------------------------------------------------  */    
/*  |  bit7  |  bit6 | bit5| bit4| bit3| bit2| bit1| bit0|                    */    
/*  |--------|-------|-----|-----|-----|-----|-----|-----|                    */    
/*  |RDY/BUSY|  COMP | 0   | 1   | 1   | 1   | X   | X   |                    */    
/*   -----------------------------------------------------------------------  */    
/*  bit7 - æ��ǣ�0Ϊæ1Ϊ��æ��                                             */    
/*         ��Status Register��λ0�Ƴ�֮�󣬽�������ʱ���������н�ʹSPI��������*/    
/*         �����µ�״̬�ֽ��ͳ���                                             */    
/*  bit6 - ������һ��Main Memory Page��Buffer�ıȽϽ����0��ͬ��1��ͬ��     */    
/******************************************************************************/    
static BYTE at45ReadStatus() {
    BYTE i;   
     
    BEGIN_SPI;                                                
    spiWrite(0xD7);                                    
    i = spiRead();    
    OVER_SPI;    
    return i;        
}

static void at45waitReady() {
    BEGIN_SPI;                                      
    spiWrite(0xD7);
    for(;;) {                                            
        if (spiRead() & 0x80) break;    
    }
    OVER_SPI;
}

/**
 * addr &= 0x1FF 512
 * page &= 0x0FFF
 *
 * send:
 * 8bit cmd | 8bit page high | 8bit page low + addr high | 8bit addr low
 */
static void at45WriteAddr(BYTE cmd, pAddress a) {
	spiWrite(cmd);
	spiWrite(DONT_CARE);
	spiWrite(a->a8.m8 & 1);
	spiWrite(a->a8.m0);
}

/** 
 * buffId is 0, buffer 1 else buffer 2
 */
static void at45WriteBuff(bit buffId, pAddress a, BYTE dat) {
    BYTE cmd = buffId ? 0x84 : 0x87;

    BEGIN_SPI;

	at45WriteAddr(cmd, a);
    spiWrite(dat);

    OVER_SPI;
}

static BYTE at45ReadBuff(bit buffId, pAddress a) {
    BYTE dat;
    BYTE cmd = buffId ? 0xD4 : 0xD6;

    BEGIN_SPI;

	at45WriteAddr(cmd, a);
	spiWrite(DONT_CARE);
    dat = spiRead();

    OVER_SPI;
    return dat;
}

static void at45writePage(BYTE cmd, pAddress a) {
	BYTE cy = a->a8.m8 & 0x80;
	BYTE l  = a->a8.m8 << 1;
	BYTE h  =(a->a8.m16<< 1) | cy;
	
	spiWrite(cmd);
	spiWrite(h);
	spiWrite(l);
	spiWrite(DONT_CARE); 
}

/* time: 20ms */
static void at45Buff2Flash(bit buffId, pAddress a) {
    BYTE cmd = buffId ? 0x88 : 0x89;

    BEGIN_SPI;
	at45writePage(cmd, a);
    OVER_SPI; 
}

/* time: 250um */
static void at45Flash2Buff(bit buffId, pAddress a) {
    BYTE cmd = buffId ? 0x53 : 0x55;

    BEGIN_SPI;
	at45writePage(cmd, a);
    OVER_SPI; 
}

/* time: 20ms */
/* оƬ�е����� 1 ���Ա��Ϊ 0����֮���ɣ�����ʹ�ò����� 0 ��ԭΪ 1 */
static void at45ErasePage(pAddress a) {
	BEGIN_SPI;
	at45writePage(0x81, a);
    OVER_SPI; 
}

/*----------------------------------------------------- smart drv ----------- */
static bit cbuff_id = 0;    
static bit cpage_modify = 0;
static Address addr;


#define PAGE_CHANGED(x)		((addr.a8.m16 != x->a8.m16) && \
							((addr.a8.m8 & 0xFC) != (x->a8.m8 & 0xFC)) )


static void _swap_page(pAddress a) {
    bit load_buff_id = !cbuff_id;

    // ���µ�ҳ��װ����һ������
    at45waitReady();
    at45Flash2Buff(load_buff_id, a);
    at45waitReady();
    
    if (cpage_modify) {
        // ����֮ǰ�Ļ��嵽 flash
        at45ErasePage(&addr);
    	at45waitReady();
        at45Buff2Flash(cbuff_id, &addr);  
        cpage_modify = 0;
    }

    cbuff_id = load_buff_id;
	addr.a32 = a->a32; 
}


static void _write(pAddress a, BYTE dat) {
    if (PAGE_CHANGED(a)) _swap_page(a);
    at45WriteBuff(cbuff_id, a, dat);
    cpage_modify = 1;
}

static BYTE _read(pAddress a) {
    BYTE dat;

    if (PAGE_CHANGED(a)) _swap_page(a);
    dat = at45ReadBuff(cbuff_id, a);
    return dat;
}


static void _init() {
	addr.a32 = 0;
    cbuff_id = 0;
    cpage_modify = 0;

    at45waitReady();
    at45Flash2Buff(cbuff_id, &addr);
    at45waitReady();
}


static void _flush() {
    at45waitReady();
    at45ErasePage(&addr);
    at45waitReady();
    at45Buff2Flash(cbuff_id, &addr);
}                    


#undef PAGE_CHANGED

// ��ҳ�ķ������� 512=0x200��
// ��ǰʹ�õĵ�ַ��λ 256Ϊһ��ҳ������
void AT45DB161B(pFlashChip f) {
    f->read  = _read;
	f->write = _write;
	f->init  = _init;
    f->flush = _flush;
	f->mem_size = 0x200000;
}