#include <stm32f4xx.h>
#include <lib.h>
#include <fs.h>
#include <mds.h>
#include <cd-img.h>
#include <cdstate.h>
#include <led.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_rcc.h>
#include <stdio.h>


/*
 * 在全局宏中加入定义:
 * 器件型号定义:  STM32F40_41xxx, 
 * 外部晶振频率:  HSE_VALUE=8000000
 * 外部晶振到 PLL 的分频:   PLL_M_RE_VAL=8
 * 原则是 HSE_VALUE / PLL_M_RE_VAL = 1 MHz
 */
int main() {
  init_led2_key_ctrl();
  init_led1_splash();
  
  printf("Hi \n");
  for (;;);
  return 0;
}


/* 实现该接口, 当请求一节数据后, 该节数据通过该方法注入
缓冲区长度可能大于 2352 */
void cdi_sector_data_ready(pByte buf, int buflen, char is_audio) {
}


void assert_param(int a) {
  if (a) 
    return;
  
  for (;;) {
  }
}
