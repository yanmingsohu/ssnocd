#include <stm32f4xx.h>
#include <lib.h>
#include <fs.h>
#include <mds.h>
#include <cd-img.h>
#include <cdstate.h>


int main() {
  SystemInit();
  return 0;
}


/* 实现该接口, 当请求一节数据后, 该节数据通过该方法注入
缓冲区长度可能大于 2352 */
void cdi_sector_data_ready(pByte buf, int buflen, char is_audio) {
}


void assert_param(int a) {
  for (;;) {
  }
}

