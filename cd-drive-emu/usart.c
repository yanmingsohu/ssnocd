#include "usart.h"
#include "queue.h"
#include <stm32f4xx.h>
#include <stm32f4xx_usart.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_exti.h>
#include <stm32f4xx_syscfg.h>
#include <misc.h>


#define US_RCC        RCC_APB2Periph_USART1
#define US_IO_RCC     RCC_AHB1Periph_GPIOA
#define US_IO_PIN_RX  GPIO_Pin_10
#define US_IO_PIN_TX  GPIO_Pin_9
#define US_IO_PORT    GPIOA
#define US_NUM        USART1


static Queue q;


/*
 * 如果没有初始化串口, 所有的写函数可以调用, 但是没有效果.
 */
void init_usart() {
  RCC_APB2PeriphClockCmd(US_RCC, ENABLE);
  RCC_AHB1PeriphClockCmd(US_IO_RCC, ENABLE);
  
  GPIO_InitTypeDef io;
  io.GPIO_Mode   = GPIO_Mode_AF;
  io.GPIO_OType  = GPIO_OType_OD;
  io.GPIO_Speed  = GPIO_Speed_2MHz;
  io.GPIO_PuPd   = GPIO_PuPd_NOPULL;
  
  io.GPIO_Pin = US_IO_PIN_RX;
  GPIO_Init(US_IO_PORT, &io);
  io.GPIO_Pin = US_IO_PIN_TX;
  GPIO_Init(US_IO_PORT, &io);
  
  GPIO_PinAFConfig(US_IO_PORT, GPIO_PinSource9,  GPIO_AF_USART1);
  GPIO_PinAFConfig(US_IO_PORT, GPIO_PinSource10, GPIO_AF_USART1);
  
  NVIC_InitTypeDef it;
  it.NVIC_IRQChannel                    = USART1_IRQn;
  it.NVIC_IRQChannelPreemptionPriority  = 5;
  it.NVIC_IRQChannelSubPriority         = 5;
  it.NVIC_IRQChannelCmd                 = ENABLE;
  NVIC_Init(&it);
  
  USART_InitTypeDef us;
  us.USART_BaudRate   = 9600;
  us.USART_WordLength = USART_WordLength_9b;
  us.USART_StopBits   = USART_StopBits_1;
  us.USART_Parity     = USART_Parity_No;
  us.USART_Mode       = USART_Mode_Tx;
  us.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  
  USART_Init(US_NUM, &us);
  USART_ITConfig(US_NUM, USART_IT_TXE, ENABLE);
  USART_Cmd(US_NUM, ENABLE);
}


void USART1_IRQHandler() {
  if (SET == USART_GetFlagStatus(US_NUM, USART_FLAG_TXE)) {
    if (has_data(&q)) {
      USART_SendData(US_NUM, read_queue(&q));
    } else {
      USART_SendData(US_NUM, 0x00);
    }
  }
}


/* 
 * 字符串长度不能超过 100, 写入读取超过串口速度会有乱码 
 * 字符串必须以 '\0' 结尾
 */
void prints(char *str) {
  write_queue(&q, 0x02);
  for (int i=0; i<100; ++i) {
    write_queue(&q, str[i]);
    if (str[i] == 0) break;
  }
}


void printi(unsigned int i) {
  write_queue(&q, 0x03);
  write_queue(&q, i & 0xFF);
  write_queue(&q, (i & 0xFF00) >> 8);
  write_queue(&q, (i & 0xFF0000) >> 16);
  write_queue(&q, (i & 0xFF000000) >> 24);
}


void printd(double i) {
  union {
    double d;
    char b[8];
  } buff;
  buff.d = i;
  
  write_queue(&q, 0x04);
  write_queue(&q, buff.b[0]);
  write_queue(&q, buff.b[1]);
  write_queue(&q, buff.b[2]);
  write_queue(&q, buff.b[3]);
  write_queue(&q, buff.b[4]);
  write_queue(&q, buff.b[5]);
  write_queue(&q, buff.b[6]);
  write_queue(&q, buff.b[7]);
}


/*
 * 打印内存, 使用 16 进制格式, 长度不要超过 128
 */
void printb(char *bin, unsigned char len) {
  write_queue(&q, 0x05);
  write_queue(&q, len);
  for (int i=0; i<len; ++i) {
    write_queue(&q, bin[i]);
  }
}
