#ifndef _COMM_RATE_
#define _COMM_RATE_

// STC 主频，晶振的频率/刷机软件显示的时钟频率
// 24009194  11059743
#define STC_CLK   (24002054)

// STC PCON 寄存器 最高位 (0,1)
#define STC_SMOD  1

// STC 计时器初始值
#define STC_TH    0xFE

// 完成应答
#define COMM_MSG_OK     0x5A
#define COMM_MSG_TEST   0x5F
// 错误应答
#define COMM_MSG_ERR    0xE0
// 未选择芯片
#define COMM_MSG_FLASH_NOSEL    0xE1
// 芯片选择完成
#define COMM_MSG_FLASH_SEL      0x5B

#endif // _COMM_RATE_
