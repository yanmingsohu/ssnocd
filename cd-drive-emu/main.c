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


/* ʵ�ָýӿ�, ������һ�����ݺ�, �ý�����ͨ���÷���ע��
   ���������ȿ��ܴ��� 2352 */
void cdi_sector_data_ready(pByte buf, int buflen, char is_audio) {
}

/* ʵ�ָýӿ�, cd ���µ�״̬����, �÷��������� */
void cdi_update_drive_bit() {
}


void assert_param(int a) {
	for (;;) {
	}
}