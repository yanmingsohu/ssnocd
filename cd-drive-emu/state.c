#include "state.h"
#include <stm32f4xx_spi.h>
#include <cdstate.h>


static u16 state_updated = 0;


/* 
 * cd 光驱状态模拟使用硬件 spi 实现
 */
void init_cd_drive_state_module() {
	SPI_InitTypeDef spidef;
	
	spidef.SPI_Direction 					= SPI_Direction_2Lines_FullDuplex;
	spidef.SPI_Mode 							= SPI_Mode_Slave;
	spidef.SPI_DataSize 					= SPI_DataSize_8b;
	/* 无数据时时钟线状态, 高 */
	spidef.SPI_CPOL 							= SPI_CPOL_High;
	/* 在上升沿采样 */
	spidef.SPI_CPHA 							= SPI_CPHA_2Edge;
	spidef.SPI_NSS 								= SPI_NSS_Soft;
	spidef.SPI_BaudRatePrescaler 	= SPI_BaudRatePrescaler_256;
	spidef.SPI_FirstBit 					= SPI_FirstBit_LSB;
	spidef.SPI_CRCPolynomial 			= 1;
	
	SPI_Init(STATE_SPI, &spidef);
	SPI_I2S_ITConfig(STATE_SPI, SPI_I2S_IT_TXE | SPI_I2S_IT_RXNE, ENABLE);
}


void STATE_IRQ_FUNC() {
	if (SPI_I2S_GetFlagStatus(STATE_SPI, SPI_I2S_IT_TXE) == SET) {
		/* 读取cd状态, 并写入spi */
    SPI_I2S_SendData(STATE_SPI, cd_drive_get_serial_byte());
	}
	else if (SPI_I2S_GetFlagStatus(STATE_SPI, SPI_I2S_IT_RXNE) == SET) {
		/* 读取spi, 写入cd */
    cd_drive_set_serial_byte(SPI_I2S_ReceiveData(STATE_SPI));
    
    if (--state_updated <= 0) {
    }
	}
}



/* 实现该接口, cd 有新的状态发送, 该方法被调用 */
void cdi_update_drive_bit() {
  assert(state_updated == 0);
  state_updated = 13;
}
