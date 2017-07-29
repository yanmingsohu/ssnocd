#include <stm32f4xx.h>


int main() {
	SystemInit();
	return 0;
}


void assert_param(int a) {
	for (;;) {}
}