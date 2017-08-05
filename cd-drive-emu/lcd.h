#ifndef _H_CDEMU_LCD
#define _H_CDEMU_LCD
#include <stm32f4xx.h>

void init_lcd();
void init_lcd_commands();
void write_byte(uint8_t d, uint32_t is_cmd);

#endif