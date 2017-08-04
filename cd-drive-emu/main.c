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
 * ��ȫ�ֺ��м��붨��:
 * �����ͺŶ���:  STM32F40_41xxx, 
 * �ⲿ����Ƶ��:  HSE_VALUE=8000000
 * �ⲿ���� PLL �ķ�Ƶ:   PLL_M_RE_VAL=8
 * ԭ���� HSE_VALUE / PLL_M_RE_VAL = 1 MHz
 */
int main() {
  init_led2_key_ctrl();
  init_led1_splash();
  
  printf("Hi \n");
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
