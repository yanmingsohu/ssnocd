#include <reg51.h>
#include "type.h"
#include "flash_chip.h"

/*
AT24C64 连接
    A0 A1 A2 = 0       
    WA 		 = 0
    SDA 	 = p2.1 
    SCL 	 = P2.0
	* p0 口是个强上拉，rom没法拉低电压
*/
sbit SDA = P4 ^ 5;
sbit SCL = P3 ^ 7;

// 主机读总线
#define I2C_R           1
// 主机写总线
#define I2C_W           0

#define DEV_ADDR(X,Y)   (0xA0 | ((X->a8.m16 & 0x07) << 1) | Y)

/* ---------------------------------------------------------- i2c ---- */

static void i2c_dly(void)
{
//	int i=0;
//	for (i=300; i>=0; --i);
//	++i;
//	++i;
}


static void i2c_start(void)
{
  SDA = 1;             // i2c start bit sequence
  i2c_dly();
  SCL = 1;
  i2c_dly();
  SDA = 0;
  i2c_dly();
  SCL = 0;
  i2c_dly();
}


static void i2c_stop(void)
{
  SDA = 0;             // i2c stop bit sequence
  i2c_dly();
  SCL = 1;
  i2c_dly();
  SDA = 1;
  i2c_dly();
}


static unsigned char i2c_rx(char ack)
{
  char x, d=0;

  SDA = 1; 
  for(x=0; x<8; x++) {
    d <<= 1;
    do {
      SCL = 1;
    } while(SCL==0);    // wait for any SCL clock stretching

    i2c_dly();
    if(SDA) d |= 1;
    SCL = 0;
  } 
  if(ack) SDA = 0;
  else SDA = 1;

  SCL = 1;
  i2c_dly();             // send (N)ACK bit
  SCL = 0;
  SDA = 1;
  return d;
}


static bit i2c_tx(unsigned char d)
{
char x;
static bit b;

  for(x=8; x; x--) {
    if(d&0x80) SDA = 1;
    else SDA = 0;
    SCL = 1;
    d <<= 1;
    SCL = 0;
  }

  SDA = 1;
  SCL = 1;
  i2c_dly();
  b = SDA;          // possible ACK bit
  SCL = 0;
  return b;
}

/* ------------------------------------------------------ i2c end ---- */


static void wait_for_wirte(pAddress a) {
	bit r = 1;
	do {
		i2c_start();            
		r = i2c_tx(DEV_ADDR(a, I2C_W));       
	} while (r);
}

static void _write(pAddress a, BYTE dat) {
	i2c_start();            
	i2c_tx(DEV_ADDR(a, I2C_W));        
	i2c_tx(a->a8.m8);            
	i2c_tx(a->a8.m0);           
	i2c_tx(dat);            
	i2c_stop();               
	wait_for_wirte(a);
}


static BYTE _read(pAddress a) {
	BYTE rec;

	i2c_start();              
	i2c_tx(DEV_ADDR(a, I2C_W));            
	i2c_tx(a->a8.m8);            
	i2c_tx(a->a8.m0);            

	i2c_start();             
	i2c_tx(DEV_ADDR(a, I2C_R)); 
	rec = i2c_rx(0);    
	i2c_stop();           

	return rec;
}

      
static void _init() {
	SDA = 1;
	SCL = 1;
}


void at24c64a(pFlashChip f) {
	f->read  = _read;
	f->write = _write;
	f->init  = _init;
	f->mem_size = 0x2000;
}