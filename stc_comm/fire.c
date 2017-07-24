#include "reg51.h"
#include "intrins.h"
#include "uart.h"
#include "flash_chip.h"

   
sfr WDT_CONTR = 0xE1;

sbit CDATA 	 = P2^0;
sbit HDATA 	 = P2^1;
sbit COMREQ  = P2^2;
sbit COMSYNC = P2^3;
sbit COMCLK  = P2^4;

#define BUF_LEN 768						   
unsigned char xdata buff[BUF_LEN] = {0};


#define R1(x) buff[i] = P2;
#define R2(x) 	R1(x) 	R1(x+1)
#define R4(x) 	R2(x) 	R2(x+2)
#define R8(x) 	R4(x) 	R4(x+4)
#define R16(x)  R8(x) 	R8(x+8)
#define R32(x)  R16(x) 	R16(x+16)
#define R64(x)  R32(x) 	R32(x+32)
#define R128(x) R64(x)  R64(x+64)
#define R256(x) R128(x) R128(x+128)

BYTE rbit;
BYTE rbuff[20];
int ri;


//
// 当收到 req 信号时, 认为新的字节到达
//
void cd_on_req() interrupt 1 {
	// 当 comsync 被拉下, 是一个新指令序列的开始
	if (COMSYNC == 0) {
		ri = 0;
		rbit = 0;
	} else {
		rbuff[ri] = rbit;
		++ri;
		rbit = 0;
	}
	SendData(1); 
}


//
// 每个 clk 信号是一个 bit
//
void cd_on_clk() interrupt 0 {
	rbit <<= 1;
	rbit |= CDATA;
}


//
// 启动 cd-rom 读取
// 启动后, req 和 sync 有两次间隔的下降沿, 通知主控初始化完成
//
void cd_run() {
	int ori, change = 0, i;

	IT0 = 1;
	EX0 = 1;
	IT1 = 1;
	EX1 = 1;
	EA  = 1;

	ri = 0;
	ori = 0;
	rbit = 0;

	for(;;) {
		if (ori != ri) {
			ori = ri;
			change = 0;
		} else {
			if (++change > 10 ) {
				 for (i=0; i<20; ++i) {
					SendData(rbuff[i]);
					rbuff[i] = 0;
				}
				SendData(0xFF);
			}
		}
	}
}


void test_pin_slow() {
	BYTE a, b, c;
	int count = 0, i;
	for (;;) {	
		a = P2;
		b = P2;
		if (a != b) {
		  SendData(a);
		  SendData(b);
		  SendData((count & 0xFF00) >> 8);
		  SendData(count & 0xFF);
		  SendData(0xFF);
		  //SendData(0);
		  count = 0;
		} else {
			++count;
		}
	}
}


void test_buff() {
	int count = 0, i;
	for (;;) {
		for (i=0; i<BUF_LEN; i+=16) {
			buff[i] = P2;
			buff[i+1] = P2;
			buff[i+2] = P2;
			buff[i+3] = P2;
			buff[i+4] = P2;
			buff[i+5] = P2;
			buff[i+6] = P2;
			buff[i+7] = P2;
			buff[i+8] = P2;
			buff[i+9] = P2;
			buff[i+10] = P2;
			buff[i+11] = P2;
			buff[i+12] = P2;
			buff[i+13] = P2;
			buff[i+14] = P2;
			buff[i+15] = P2;
		}
		for (i=0; i<BUF_LEN; ++i) {
			SendData(buff[i]);
		}
	}
}


//
// 测试串口通信有效性
//
void test_uart() {
	BYTE a;
	for (a=0;; ++a) {
		SendData(a);
	}
}


void main()
{    
    //WDT_CONTR = 0x20;
	//P0 = 0x01;	 
	int buffi, i = 0;
	BYTE nextclk = 1;	 
	BYTE clk;

	P2 = 0x1F; //0x1F;				  
    init_uart();

	//cd_run();
	//test_uart();
	test_buff();
	//test_pin_slow();

	for (;;) {
		buffi = 0;
		for (;;) {
			clk = COMCLK;
			if (clk == nextclk) {
				buff[buffi] = P2;			
				nextclk = clk ? 0 : 1;
				if (++buffi >= BUF_LEN) break;
			}
		}
		for (i=0; i<BUF_LEN; ++i) {
			SendData(buff[i]);
		}
	}
}