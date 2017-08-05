#include "lcd.h"
#include <stm32f4xx_spi.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_exti.h>
#include <stm32f4xx_syscfg.h>
#include <misc.h>


#define LCD_SPI           SPI1
#define LCD_SPI_RCC       RCC_APB2Periph_SPI1
#define LCD_IO_PORT       GPIOA    
#define LCD_IO_PIN_SCK    GPIO_Pin_5
#define LCD_IO_PIN_MOSI   GPIO_Pin_7
#define LCD_IO_PIN_RS     GPIO_Pin_8
#define LCD_IO_RCC        RCC_AHB1Periph_GPIOA
#define BUFFER_SIZE       512

static uint8_t  data_buffer[BUFFER_SIZE] = {0};
static uint32_t rc[BUFFER_SIZE/32] = {0};
static uint32_t write_pos;
static uint32_t read_pos;


/*
 * ! 驱动不工作
 */
void init_lcd() {
  RCC_APB2PeriphClockCmd(LCD_SPI_RCC, ENABLE);
  RCC_AHB1PeriphClockCmd(LCD_IO_RCC, ENABLE);
  
  SPI_InitTypeDef spidef;
	
	spidef.SPI_Direction 					= SPI_Direction_1Line_Tx;
	spidef.SPI_Mode 							= SPI_Mode_Master;
	spidef.SPI_DataSize 					= SPI_DataSize_8b;
	/* 无数据时时钟线状态, 高 */
	spidef.SPI_CPOL 							= SPI_CPOL_Low;
	/* 在上升沿采样 */
	spidef.SPI_CPHA 							= SPI_CPHA_1Edge;
	spidef.SPI_NSS 								= SPI_NSS_Soft;
	spidef.SPI_BaudRatePrescaler 	= SPI_BaudRatePrescaler_256;
	spidef.SPI_FirstBit 					= SPI_FirstBit_MSB;
	spidef.SPI_CRCPolynomial 			= 1;
	
	SPI_Init(LCD_SPI, &spidef);
	SPI_I2S_ITConfig(LCD_SPI, SPI_I2S_IT_TXE, ENABLE);
  
  GPIO_InitTypeDef io;
  io.GPIO_Mode  = GPIO_Mode_OUT;
  io.GPIO_Speed = GPIO_Speed_50MHz;
  io.GPIO_OType = GPIO_OType_PP; // GPIO_OType_OD  GPIO_OType_PP
  io.GPIO_PuPd  = GPIO_PuPd_DOWN;
  
  io.GPIO_Pin = LCD_IO_PIN_SCK;
  GPIO_Init(LCD_IO_PORT, &io);
  
  io.GPIO_Pin = LCD_IO_PIN_MOSI;
  GPIO_Init(LCD_IO_PORT, &io);
  
  io.GPIO_Pin = LCD_IO_PIN_RS;
  GPIO_Init(LCD_IO_PORT, &io);
  
  NVIC_InitTypeDef it;
  it.NVIC_IRQChannel                    = SPI1_IRQn;
  it.NVIC_IRQChannelPreemptionPriority  = 5;
  it.NVIC_IRQChannelSubPriority         = 1;
  it.NVIC_IRQChannelCmd                 = ENABLE;
  NVIC_Init(&it);
  
  SPI_SSOutputCmd(LCD_SPI, ENABLE);
  SPI_Cmd(LCD_SPI, ENABLE);
  
  write_pos = 0;
  read_pos = 0;
  init_lcd_commands();
}


void init_lcd_commands() {
#define write_command(x)  write_byte(x, 1)
#define write_data(x)     write_byte(x, 0)
  
  write_command(0x11); // # Exit Sleep
	//time.sleep(0.02);
	write_command(0x26); // # Set Default Gamma
	write_data(0x04);
	write_command(0xB1); //# Set Frame Rate
	write_data(0x0e);
	write_data(0x10);
	write_command(0xC0); // # Set VRH1[4:0] & VC[2:0] for VCI1 & GVDD
	write_data(0x08);
	write_data(0x00);
	write_command(0xC1); // # Set BT[2:0] for AVDD & VCL & VGH & VGL
	write_data(0x05);
	write_command(0xC5); // # Set VMH[6:0] & VML[6:0] for VOMH & VCOML
	write_data(0x38);
	write_data(0x40);

	write_command(0x3a); // # Set Color Format
	write_data(0x05);
	write_command(0x36); // # RGB
	write_data(0xc8);

	write_command(0x2A); // # Set Column Address
	write_data(0x00);
	write_data(0x00);
	write_data(0x00);
	write_data(0x7F);
	write_command(0x2B); // # Set Page Address
	write_data(0x00);
	write_data(0x00);
	write_data(0x00);
	write_data(0x7F);

	write_command(0xB4);
	write_data(0x00);

	write_command(0xf2); // # Enable Gamma bit
	write_data(0x01);
	write_command(0xE0); // 
	write_data(0x3f);//# p1
	write_data(0x22);//# p2
	write_data(0x20);//# p3
	write_data(0x30);//# p4
	write_data(0x29);//# p5
	write_data(0x0c);//# p6
	write_data(0x4e);//# p7
	write_data(0xb7);//# p8
	write_data(0x3c);//# p9
	write_data(0x19);//# p10
	write_data(0x22);//# p11
	write_data(0x1e);//# p12
	write_data(0x02);//# p13
	write_data(0x01);//# p14
	write_data(0x00);//# p15
	write_command(0xE1) ;//
	write_data(0x00);//# p1
	write_data(0x1b);//# p2
	write_data(0x1f);//# p3
	write_data(0x0f);//# p4
	write_data(0x16);//# p5
	write_data(0x13);//# p6
	write_data(0x31);//# p7
	write_data(0x84);//# p8
	write_data(0x43);//# p9
	write_data(0x06);//# p10
	write_data(0x1d);//# p11
	write_data(0x21);//# p12
	write_data(0x3d);//# p13
	write_data(0x3e);//# p14
	write_data(0x3f);//# p15

	write_command(0x29);// #  Display On
	write_command(0x2C);//
    
  for (int i=0; i<128; ++i) {
    write_data(i);
  }
}


void write_byte(uint8_t d, uint32_t is_cmd) {
  data_buffer[write_pos] = d;
  uint32_t rcpos   = write_pos >> 5;
  uint32_t bitmask = 1 << (write_pos & 0x1F);
  if (is_cmd) {
    rc[rcpos] |= bitmask;
  } else {
    rc[rcpos] &= ~(bitmask);
  }
  if (++write_pos >= BUFFER_SIZE) {
    write_pos = 0;
  }
}


void SPI1_IRQHandler() {
  if (SET == SPI_I2S_GetFlagStatus(LCD_SPI, SPI_I2S_FLAG_TXE)) {
    if (read_pos != write_pos) {
      uint32_t rcpos   = read_pos >> 5;
      uint32_t bitmask = 1 << (read_pos & 0x1F);
      uint32_t is_cmd  = rc[rcpos] & bitmask;
      if (is_cmd) {
        GPIO_ResetBits(LCD_IO_PORT, LCD_IO_PIN_RS);
      } else {
        GPIO_SetBits(LCD_IO_PORT, LCD_IO_PIN_RS);
      }
      
      SPI_I2S_SendData(LCD_SPI, data_buffer[read_pos]);
      if (++read_pos >= BUFFER_SIZE) {
        read_pos = 0;
      }
    } else {
      SPI_I2S_SendData(LCD_SPI, 0);
    }
  }
}