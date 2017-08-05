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


/* 字符串长度不能超过 100, 写入读取超过串口速度会有乱码 */
void print(char *str) {
  for (int i=0; i<100; ++i) {
    if (str[i] == 0) break;;
    write_queue(&q, str[i]);
  }
  write_queue(&q, '\n');
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