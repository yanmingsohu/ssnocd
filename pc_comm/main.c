#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <math.h>
#include "comm_rate.h"

#define EQ_BYTE(a,b)            ((byte)a == (byte)b)
#define SEND_FILE_BUFF_SIZE     0xFF0
#define STC_BAUD                128000L


void welcome();
void read_data_nostop(HANDLE com);
void test_uart();


int main()
{
    HANDLE com1;
    DCB dcb;
    int comnum;
    char buff[10];
    int timeout = 20000;
    int baud_rate;

    welcome();
    fflush(stdin);
    printf("输入 com 端口号 1-10: ");
    scanf("%d", &comnum);

    if (comnum <= 0) {
        printf("无效端口号, 退出\n");
        return 2;
    }
    sprintf(buff, "COM%d", comnum);

    //baud_rate = pow(2, STC_SMOD)/ 32.0* (STC_CLK/ 12/ (256-STC_TH));
    baud_rate = STC_BAUD;
    printf("通讯速率 %d bit/s\n", baud_rate);

    com1 = CreateFile(buff, GENERIC_READ|GENERIC_WRITE, 0, 0,
                      OPEN_EXISTING, 0,0);

    if (com1 == INVALID_HANDLE_VALUE) {
        printf("打开 %s 失败\n", buff);
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

    // test_uart(com1);
    read_data_nostop(com1);

    printf("\n bye\n");
    CloseHandle(com1);
    return 0;
}


//
// 测试串口通信有效性
//
void test_uart(HANDLE com) {
    byte r, a, b, buff;
    int c;
    DWORD count;

    printf("test\n");

    for (c=0;;++c) {
        ReadFile(com, &buff, 1, &count, 0);
        if (buff != 0 && a != 0xFF && buff-a != 1) {
            printf("bad at %d, %d:%d\n", c, buff, a);
        }
        if (c%10000 == 0) printf("success %d\n", c);
        a = buff;
    }
}


void read_data_nostop(HANDLE com) {
    byte buff;
    byte pbuff = 0;
    byte cdata, hdata, comreq, comsync, comclk;
    DWORD count;
    int i;

    byte wbuf[200];
    int wlen;
    HANDLE out = CreateFile("./out.txt", GENERIC_WRITE, 0, 0, CREATE_NEW, 0,0);

#define BIT_BOOL(index) ((buff & (1<<index)) >> index)

    for (i=0;; ++i) {
        ReadFile(com, &buff, 1, &count, 0);
/*      cdata   = BIT_BOOL(0);
        hdata   = BIT_BOOL(1);
        comreq  = BIT_BOOL(2);
        comsync = BIT_BOOL(3);
        comclk  = BIT_BOOL(4);*/
        //printf("%05d 0x%x cdata:%d hdata:%d req:%d sync:%d clk:%d\n", i, buff, cdata, hdata, comreq, comsync, comclk);

        if (i % 10000 == 0)
            //printf("%05d %03d %02x \n", i, buff, buff);
        //printf("%x\t", buff);

        pbuff = buff;

        sprintf(wbuf, "%d, ", buff);
        wlen = strlen(wbuf);
        WriteFile(out, &wbuf, wlen, &count, 0);
    }
}


void print_process(int point, DWORD size, byte crc) {
    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
           "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
           "发送数据包: %xh - %.2f%c [crc %02xh]",
           point, ((float)point/size*100), '%', crc);
}


void readFilePath(char *path, int max_size) {
    char c = 0;
    int pos = 0;
    fflush(stdin);

    while (pos < max_size) {
        c = getchar();
        if (c == '\n')
            break;
        if (c == '"' || c == '"')
            continue;
        path[pos++] = c;
    }
    path[pos] = 0;
}


BYTE crc(BYTE* buff, int size) {
    BYTE crc = 0;
    int i;
    for (i=0; i<size; ++i) {
        crc += buff[i];
    }
    return crc;
}


void welcome() {
printf(
    "//---------------------------------------------------------------------------//\n" \
    "// CatfoOD 2014. 刷机客户端\n" \
    "// V 0.21 \n" \
    "//---------------------------------------------------------------------------//\n");
}
