#ifndef _cdemu_state
#define _cdemu_state
#include <stm32f4xx_spi.h>

/* 必须同时修改这些 spi 的定义 */
#define STATE_SPI       SPI2
#define STATE_IRQ_FUNC  SPI2_IRQHandler

#endif