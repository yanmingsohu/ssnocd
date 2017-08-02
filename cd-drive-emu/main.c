#include <stm32f4xx.h>
#include <lib.h>
#include <fs.h>
#include <mds.h>
#include <cd-img.h>
#include <cdstate.h>
#include <led.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_rcc.h>


int main() {
  /*
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  // GPIO_SetBits(GPIOC, GPIO_Pin_0);
  GPIO_ResetBits(GPIOC, GPIO_Pin_0);
  
  for (;;) {
    GPIO_SetBits(GPIOC, GPIO_Pin_0);
    GPIO_ResetBits(GPIOC, GPIO_Pin_0);
  }
  */
  
  init_led_splash();
  for (;;);
  return 0;
}


/* 实现该接口, 当请求一节数据后, 该节数据通过该方法注入
缓冲区长度可能大于 2352 */
void cdi_sector_data_ready(pByte buf, int buflen, char is_audio) {
}


void assert_param(int a) {
  if (a) return;
  for (;;) {
  }
}
