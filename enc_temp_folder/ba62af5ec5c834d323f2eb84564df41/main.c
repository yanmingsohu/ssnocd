#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <math.h>
#include "comm_rate.h"

#define EQ_BYTE(a,b)            ((byte)a == (byte)b)
#define SEND_FILE_BUFF_SIZE     0xFF0

static unsigned int uart_rate[] = {
  600, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400,
  57600, 115200, 128000, 256000 
};


void welcome();
void read_char_print(HANDLE com);
void get_com_num(char* out);
int  get_comm_rate();
void debug_display(HANDLE com);


int main()
{
    HANDLE com1;
    DCB dcb;
    char comname[10];
    int timeout = 20000;
    int baud_rate;

    welcome();
    fflush(stdin);
    get_com_num(comname);

    baud_rate = get_comm_rate();
    printf("通讯速率 %d bit/s\n", baud_rate);

    com1 = CreateFile(comname, GENERIC_READ|GENERIC_WRITE, 0, 0,
                      OPEN_EXISTING, 0,0);

    if (com1 == INVALID_HANDLE_VALUE) {
        printf("打开 %s 失败\n", comname);
        return 3;
    }

    if (!GetCommState(com1, &dcb)) {
        printf("读取状态失败\n");
        return 4;
    }

    SetupComm(com1, 10, 10);

    dcb.BaudRate = baud_rate;
    dcb.ByteSize = 8;
    dcb.Parity = EVENPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(com1, &dcb);

    COMMTIMEOUTS TimeOuts;
    TimeOuts.ReadIntervalTimeout         = timeout;
    TimeOuts.ReadTotalTimeoutMultiplier  = timeout;
    TimeOuts.ReadTotalTimeoutConstant    = timeout;
    TimeOuts.WriteTotalTimeoutMultiplier = timeout;
    TimeOuts.WriteTotalTimeoutConstant   = timeout;
    SetCommTimeouts(com1, &TimeOuts);

    PurgeComm(com1, PURGE_TXCLEAR | PURGE_RXCLEAR);

    debug_display(com1);

    printf("\n bye\n");
    CloseHandle(com1);
    return 0;
}


int get_comm_rate() {
  int rate;
  for (int i=0; i<ARRAYSIZE(uart_rate); ++i) {
    printf("  %d - %d bit/s \n", i, uart_rate[i]);
  }
  fflush(stdin);
  printf("通讯速率 bit/s, 或索引: ");
  scanf("%d", &rate);

  if (rate < 0) {
    printf("无效的速率值 %d\n", rate);
    exit(2);
  } if (rate < ARRAYSIZE(uart_rate)) {
    rate = uart_rate[rate];
  }

  return rate;
}


void get_com_num(char* out) {
  int comnum;
  fflush(stdin);
  printf("输入 com 端口号 1-10: ");
  scanf("%d", &comnum);

  if (comnum <= 0 || comnum > 99) {
    printf("无效端口号 %d, 退出\n", comnum);
    return exit(1);
  }
  sprintf(out, "COM%d", comnum);
}


void print_process(int point, DWORD size, byte crc) {
    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
           "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
           "发送数据包: %xh - %.2f%c [crc %02xh]",
           point, ((float)point/size*100), '%', crc);
}


/*
 * 读取动态字符串
 */
void read_char_print(HANDLE com) {
  byte buff;
  DWORD count;
  int enter = 0;

  for (;;) {
    ReadFile(com, &buff, sizeof(buff), &count, 0);
    if (count < 1) 
      continue;

    if (buff == '\n') {
      ++enter;
    }
    
    if (buff != 0) {
      putch(buff);
    } else {
      break;
    }
  }

  if (!enter) putch(' ');
}


void read_int32(HANDLE com) {
  union {
    byte b[4];
    int i;
  } buff;
  DWORD count;

  ReadFile(com, &buff.b, sizeof(buff.b), &count, 0);
  printf("%d ", buff.i);
}


void read_double(HANDLE com) {
  union {
    byte b[8];
    double i;
  } buff;
  DWORD count;
  ReadFile(com, &buff.b, sizeof(buff.b), &count, 0);
  printf("%f ", buff.i);
}


void read_bin(HANDLE com) {
  byte len = 0, buff = 0;
  DWORD count = 0;
  char ch[17] = {0};

  printf("\n> ");
  ReadFile(com, &len, sizeof(len), &count, 0);
  for (int i=0; i<len; ++i) {
    ReadFile(com, &buff, sizeof(buff), &count, 0);
    printf("%02x ", buff);
    ch[i %16] = buff>0x1F && buff<0x80 ? buff : '.';
    if (i % 8 == 7) printf("  ");
    if (i % 16 == 15) {
      printf(" %s\n> ", ch);
    }
  }
  printf("\n");
}


/*
 * STm32f4 串口调试协议实现
 */
void debug_display(HANDLE com) {
  byte cmd;
  DWORD count;

  for (;;) {
    ReadFile(com, &cmd, sizeof(cmd), &count, 0);
    if (count < 1) continue;

    switch (cmd) {
      case 0x02:
        read_char_print(com);
        break;

      case 0x03:
        read_int32(com);
        break;

      case 0x04:
        read_double(com);
        break;

      case 0x05:
        read_bin(com);
    }
  }
}


void welcome() {
printf(
    "//---------------------------------------------------------------------------//\n" \
    "// CatfoOD 2017  STM32F4  串口调试协议 \n" \
    "// V 0.01 \n" \
    "//---------------------------------------------------------------------------//\n");
}
