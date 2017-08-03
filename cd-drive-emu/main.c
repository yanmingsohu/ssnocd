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


/* ʵ�ָýӿ�, ������һ�����ݺ�, �ý�����ͨ���÷���ע��
���������ȿ��ܴ��� 2352 */
void cdi_sector_data_ready(pByte buf, int buflen, char is_audio) {
}


void assert_param(int a) {
  if (a) 
    return;
  
  for (;;) {
  }
}
