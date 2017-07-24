#include "uart.h"
//#include "../pc_comm/comm_rate.h"

// STC 主频，晶振的频率/刷机软件显示的时钟频率
// 24009194  11059743
#define STC_CLK   24009194L
#define BAUD 	  115200

// STC PCON 寄存器 最高位 (0,1)
#define STC_SMOD  0

static int T12 = 12;
static int T6  = 6;

// sbit bit9 = P2^2;           //P2.2 show UART data bit9
// sbit SMOD = PCON ^ 7; don't do this!

static bit busy;
static BYTE recv_buff[UART_BUFF_SIZE];
static BYTE buff_point = 0; 


void init_uart() {
	int temp;

#if (PARITYBIT == NONE_PARITY)
    SCON = 0x50;            //8-bit variable UART
#elif (PARITYBIT == ODD_PARITY) || (PARITYBIT == EVEN_PARITY) || (PARITYBIT == MARK_PARITY)
    SCON = 0xda;            //9-bit variable UART, parity bit initial to 1
#elif (PARITYBIT == SPACE_PARITY)
    SCON = 0xd2;            //9-bit variable UART, parity bit initial to 0
#endif

    TMOD = 0x20;            //Set Timer1 as 8-bit auto reload mode
    //PCON |= (STC_SMOD << 7);

	// 不同的主频会有不同的串口速度，PC端跟随变动
    //TH1 = TL1 = -(STC_CLK/6/32/BAUD);
	temp = -(STC_CLK/6/32/BAUD);
	TH1 = (temp & 0xFF00) >> 8;
	TL1 = temp & 0xFF;

    TR1 = 1;                //Timer1 start run
    ES = 1;                 //Enable UART interrupt
    EA = 1;                 //Open master interrupt switch
}

/*----------------------------
UART interrupt service routine
----------------------------*/
void Uart_Isr() interrupt 4 using 1
{
    if (RI)
    {
        RI = 0;             //Clear receive interrupt flag
        //(*recv_func)(SBUF, RB8);
		if (buff_point >= UART_BUFF_SIZE) 
			--buff_point;
		recv_buff[buff_point++] = SBUF;
    }
    if (TI)
    {
        TI = 0;             //Clear transmit interrupt flag
        busy = 0;           //Clear transmit busy flag
    }
}

BYTE ReadData() {
	while (!buff_point);
	return recv_buff[--buff_point];
}

/*----------------------------
Send a byte data to UART
Input: dat (data to be sent)
Output:None
----------------------------*/
void SendData(BYTE dat)
{
    while (busy);           //Wait for the completion of the previous data is sent
    ACC = dat;              //Calculate the even parity bit P (PSW.0)
    if (P)                  //Set the parity bit according to P
    {
#if (PARITYBIT == ODD_PARITY)
        TB8 = 0;            //Set parity bit to 0
#elif (PARITYBIT == EVEN_PARITY)
        TB8 = 1;            //Set parity bit to 1
#endif
    }
    else
    {
#if (PARITYBIT == ODD_PARITY)
        TB8 = 1;            //Set parity bit to 1
#elif (PARITYBIT == EVEN_PARITY)
        TB8 = 0;            //Set parity bit to 0
#endif
    }
    busy = 1;
    SBUF = ACC;             //Send data to UART buffer
}

/*----------------------------
Send a string to UART
Input: s (address of string)
Output:None
----------------------------*/
void SendString(char *s)
{
    while (*s)              //Check the end of the string
    {
        SendData(*s++);     //Send current char and increment string ptr
    }
}
