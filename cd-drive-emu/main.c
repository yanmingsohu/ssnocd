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
  init_led2_key_ctrl();
  init_led1_splash();
  
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
