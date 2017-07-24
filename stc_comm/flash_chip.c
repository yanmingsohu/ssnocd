#include "flash_chip.h"
#include "../pc_comm/comm_rate.h"
#include <reg51.h>

static FlashChip flash;


static void error() {
	for (;;)
		SendData(COMM_MSG_ERR);
}


static void null_func() {
}


void clearFlashPoint() {
	flash.read   	= null_func;
	flash.write  	= null_func;
	flash.init   	= null_func;
    flash.flush     = null_func;
	flash.state  	= COMM_MSG_FLASH_NOSEL;
	flash.mem_size 	= 0;
}


void flash_T1() {
	BYTE i;

	clearFlashPoint();

	SendData(0x88);
	SendData(0x77);
	SendData(0x22);

	i = ReadData();
	SendData(~i);
	flash_T2();
}


void flash_T2() {
	for (;;) {
		switch (ReadData()) {
		case 0xC1:
			SendData(COMM_MSG_TEST);
			break;

		case 0xC2:
			SendData(COMM_MSG_OK);
			return;

		case 0xC3:
			SendData(flash.state);
			if (flash.state == COMM_MSG_OK) {
				flash_T3();
				break;
			}
			break;

		case 0xC4:
			clearFlashPoint();
			flash.state = COMM_MSG_OK;

			switch(ReadData()) {
			case 0:
				s29al016d(&flash);
				break;
			case 1:
				at24c64a(&flash);
				break;
			case 2:
				flash_chip_test(&flash);
				break;
            case 3:
                AT45DB161B(&flash);
                break;
			default:
				flash.state = COMM_MSG_FLASH_NOSEL;
			}

			(*flash.init)(0, 0);
			SendData(flash.state);
			break;

		case 0xC5:
			SendData(flash.state);
			if (flash.state == COMM_MSG_OK) {
				flash_T4();
			}
			break;
		}
	}
}


void flash_T3() {
	ADDR file_len;    
    ADDR tmp;
	int i;             
	Address file_pos;
							  
	const int buff_size = UART_BUFF_SIZE;
	BYTE buff[UART_BUFF_SIZE];
    BYTE rec_data, cal_crc;

    // 请求文件长度
	SendData(0xA1);
	if (ReadData() != COMM_MSG_OK) error();
    file_len = 0;
	file_len |= ReadData();
	file_len |= ReadData() << 8;
    tmp       = ReadData();
	file_len |= tmp << 16;

    // 设置接收缓冲区长度
	SendData(0xA2);
	SendData(buff_size >> 4);
	if (ReadData() != COMM_MSG_OK) error();
                  
    // 发送文件指针	
	file_pos.a32 = 0;
    for (i=0; i<3; ++i) {
    	SendData(0xAA + i);
		tmp   = 0xFF;
		tmp <<= i*8;
		tmp  &= file_pos.a32;
		tmp >>= i*8;
    	SendData(tmp);
    	if (ReadData() != COMM_MSG_OK) error();
    }

    // 接收数据
	while (file_pos.a32 < file_len) {
		SendData(0xA3);
        cal_crc = 0;
		for (i=0; i<buff_size; ++i) {
			rec_data = ReadData();
			buff[i] = rec_data;
            cal_crc += rec_data;
		}

        // 校验错误会重新请求包
        if (ReadData() != cal_crc) {
            SendData(0xA4);
            if (ReadData() != COMM_MSG_OK) error();
            continue;
        }

		// 发送到 flash
		// 函数指针调用导致数据越位
		for (i=0; i<buff_size; ++i) {
			(*flash.write)(&file_pos, buff[i]);
			++file_pos.a32;
		}
	}
    
    (*flash.flush)();
    // 结束文件请求操作
	SendData(0xAF);
	if (ReadData() != COMM_MSG_OK) error();
	return;
}


void flash_T4() {
	BYTE cot, len, crc, i;
	ADDR old, d;
	Address a;

	
	for (;;) {
		switch(ReadData()) {
		case 0xA0:
			a.a8.m0 = ReadData();
			SendData(a.a8.m0);
			break;

		case 0xA1:
			a.a8.m8 = ReadData();
			SendData(a.a8.m8);
			break;

		case 0xA2:
			a.a16.h = ReadData();
			SendData(a.a16.h);
			break;

		case 0xAA:
			len = ReadData();  
			cot = 0;
			old = a.a32;

			while (cot < len) {
				// ! 调用函数越位 !
				d = (*flash.read)(&a);
				SendData(d);
				crc += d; 

				++cot;
				++a.a32;
			}

			a.a32 = old;
			SendData(crc);
			break;

		case 0xAB:
			d = (*flash.read)(&a);
			SendData(d);
			break;

		case 0xAC:
			d = flash.mem_size;
			for (i=0; (d ^ 0x01) && (i < 32); ++i) {
				d >>= 1;
			}
			SendData(i);
			break;

		case 0xAF:
			SendData(COMM_MSG_OK); 
			goto t4_end;
		}
	}
t4_end:
	return;
}