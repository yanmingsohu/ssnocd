#ifndef _UART_H_
#define _UART_H_

#include "reg51.h"
#include "type.h"
                                
#define UART_BUFF_SIZE	0x20

/*Define UART parity mode*/
#define NONE_PARITY     0   //None parity
#define ODD_PARITY      1   //Odd parity
#define EVEN_PARITY     2   //Even parity
#define MARK_PARITY     3   //Mark parity
#define SPACE_PARITY    4   //Space parity

#define PARITYBIT       EVEN_PARITY


void init_uart();
/* 直到缓冲区中接受到数据才返回， 否则一直阻塞 */
BYTE ReadData();
void SendData(BYTE dat);
void SendString(char *s);


#endif