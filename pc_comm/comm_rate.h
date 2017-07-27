#ifndef _COMM_RATE_
#define _COMM_RATE_

// STC ��Ƶ�������Ƶ��/ˢ�������ʾ��ʱ��Ƶ��
// 24009194  11059743
#define STC_CLK   (24002054)

// STC PCON �Ĵ��� ���λ (0,1)
#define STC_SMOD  1

// STC ��ʱ����ʼֵ
#define STC_TH    0xFE

// ���Ӧ��
#define COMM_MSG_OK     0x5A
#define COMM_MSG_TEST   0x5F
// ����Ӧ��
#define COMM_MSG_ERR    0xE0
// δѡ��оƬ
#define COMM_MSG_FLASH_NOSEL    0xE1
// оƬѡ�����
#define COMM_MSG_FLASH_SEL      0x5B

#endif // _COMM_RATE_
