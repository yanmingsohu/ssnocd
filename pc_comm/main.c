#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <math.h>
#include "comm_rate.h"

#define EQ_BYTE(a,b)            ((byte)a == (byte)b)
#define SEND_FILE_BUFF_SIZE     0xFF0


void flash_t1(HANDLE com);
void flash_t2(HANDLE com);
void flash_t3(HANDLE com, HANDLE file);
void flash_t4(HANDLE com);
void welcome();
void read_char_print(HANDLE com);


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
    printf("���� com �˿ں� 1-10: ");
    scanf("%d", &comnum);

    if (comnum <= 0) {
        printf("��Ч�˿ں�, �˳�\n");
        return 2;
    }
    sprintf(buff, "COM%d", comnum);

    // baud_rate = pow(2, STC_SMOD)/ 32.0* (STC_CLK/ 12/ (256-STC_TH));
    baud_rate = BAUD_RATE;
    printf("ͨѶ���� %d bit/s\n", baud_rate);

    com1 = CreateFile(buff, GENERIC_READ|GENERIC_WRITE, 0, 0,
                      OPEN_EXISTING, 0,0);

    if (com1 == INVALID_HANDLE_VALUE) {
        printf("�� %s ʧ��\n", buff);
        return 3;
    }

    if (!GetCommState(com1, &dcb)) {
        printf("��ȡ״̬ʧ��\n");
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

    read_char_print(com1);

    printf("\n bye\n");
    CloseHandle(com1);
    return 0;
}


void print_process(int point, DWORD size, byte crc) {
    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
           "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
           "�������ݰ�: %xh - %.2f%c [crc %02xh]",
           point, ((float)point/size*100), '%', crc);
}


//
// ��Ϊ mcu ��׼�����, ��ʾ�ַ�.
//
void read_char_print(HANDLE com) {
  byte buff[80];
  DWORD count;

  for (;;) {
    ReadFile(com, &buff, sizeof(buff), &count, 0);
    if (count > 0) {
      for (int i=0; i<count; ++i) {
        if (buff[i] != 0) {
          putch(buff[i]);
        }
      }
    }
  }
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


void flash_t1(HANDLE com) {
    byte head[] = {0x88, 0x77, 0x22};
    byte buff;
    DWORD count;
    byte r, xr, i;

    srand ( time(NULL) );
    r = (rand() * 0xFF) | 0x08;
    xr = ~r;
    printf("��������, �ȴ�... ");

    for (i=0; i<3;) {
        ReadFile(com, &buff, 1, &count, 0);
        if (count != 1) goto _conn_fail_;

        if (buff == head[i]) {
            ++i;
            printf("[%xh] ", buff);
        } else {
            i = 0;
            printf("%x ", buff);
        }
    }

    WriteFile(com, &r, 1, &count, 0);
    ReadFile(com, &buff, 1, &count, 0);
    printf("[%0xh] ", buff);

    if ((count == 1 )&& (xr == buff)) {
        printf("���ֳɹ�\n");
        flash_t2(com);
    } else {
_conn_fail_:
        printf("����ʧ�� %x\n", buff);
    }
}


void flash_cmd_list() {
    printf("\nh - ����\n" \
           "f - �����ļ�\n" \
           "r - ��������\n" \
           "x - �Ͽ�����\n" \
           "t - ��������\n" \
           "s - �л�оƬ\n" );
}

void flash_t2(HANDLE com) {
    byte buff;
    DWORD count;
    char cmd;

#define SendData(x)     \
            buff = 0xFF & (x); \
            WriteFile(com, &buff, 1, &count, 0); \
        printf("[send %xh]", buff); \
            if (count != 1) goto len_err;
#define ReadData(name)  \
            ReadFile(com, &buff, 1, &count, 0); \
            if (count != 1) goto len_err;

    flash_cmd_list();

    for (;;) {
        fflush(stdin);
        printf("\n[�ȴ���������] >> ");
        cmd = getch();
        printf("%c\n", cmd);

        if (cmd == 'h') {
            flash_cmd_list();
        }

        else if (cmd == 'x') {
            SendData(0xC2);
            ReadData(buff);
            printf("�Ͽ�������\n");
            return;
        }

        else if (cmd == 't') {
            printf("��������,�ȴ�Ӧ��... ");
            SendData(0xC1);
            ReadData(buff);

            if (buff==COMM_MSG_TEST) {
                printf("�ɹ�\n");
            } else {
                printf("ʧ�� [%xh]\n", buff);
            }
        }

        else if (cmd == 'f') {
            char path[255];
            HANDLE file;

            printf("�����ļ�·��:");
            readFilePath(path, 255);

            file = CreateFile(path, GENERIC_READ, 0, 0, OPEN_EXISTING, 0,0);
            if (file == INVALID_HANDLE_VALUE) {
                printf("�� %s ʧ��\n", path);
            } else {
                SendData(0xC3);
                ReadData(buff);

                if (buff==COMM_MSG_OK) {
                    flash_t3(com, file);
                    CloseHandle(file);
                } else if (buff == COMM_MSG_FLASH_NOSEL) {
                    printf("δѡ��оƬ�����ܷ���\n");
                } else {
                    printf("δ���ܵ���Ч��Ӧ\n");
                }

            }
        }

        else if (cmd == 'r') {
            SendData(0xC5);
            ReadData(buff);

            if (buff==COMM_MSG_OK) {
                flash_t4(com);
            } else if (buff == COMM_MSG_FLASH_NOSEL) {
                printf("δѡ��оƬ�����ܽ���\n");
            } else {
                printf("δ���ܵ���Ч��Ӧ\n");
            }
        }

        else if (cmd == 's') {
            int num = 0;
            printf("����оƬ���� [0:s29al016d 1:at24c64a 2:Test 3:AT45DB161B] > ");
            scanf("%d", &num);

            if (num < 0 || num > 20) {
                printf("��ЧоƬ\n");
            } else {
                SendData(0xC4);
                SendData(num);
                ReadData(buff);

                if (buff==COMM_MSG_OK) {
                    printf("�ɹ� %d\n", num);
                } else {
                    printf("ʧ��, δָ���κ�оƬ [%xh]\n", buff);
                }
            }
        }
    }
    return;
#undef SendData
#undef ReadData

len_err:
    printf("����, ���ӳ�ʱ.\n");
    return;
}


void flash_t3(HANDLE com, HANDLE file) {

    int file_point = 0;
    DWORD filesize;
    unsigned int rec_buff_size = 0x100;
    byte buff[SEND_FILE_BUFF_SIZE];
    DWORD count;
    UINT mask, left;

    filesize = GetFileSize(file, 0);
    printf("����: %ld(%xh) byte\n", filesize, (int)filesize);

#define CHECK_RW_COUNT(x)  \
                    if (count != (x)) { \
                        printf("not rw %d[%d].\n", x, (int)count); \
                        goto rw_err; \
                    }
#define SEND_OK \
                    buff[0] = COMM_MSG_OK; \
                    WriteFile(com, &buff, 1, &count, 0); \
                    CHECK_RW_COUNT(1);

    for (;;) {
        ReadFile(com, &buff, 1, &count, 0);
        CHECK_RW_COUNT(1);

        switch (buff[0]) {
            case 0xA1:
                buff[0] = COMM_MSG_OK;
                buff[1] = filesize & 0xFF;
                buff[2] = (filesize & 0xFF00) >> 8;
                buff[3] = (filesize & 0xFF0000) >> 16;

                WriteFile(com, buff, 4, &count, 0);
                CHECK_RW_COUNT(4);
                printf("�����ļ��ߴ����. \n");
                break;

            case 0xAB:
            case 0xAA:
            case 0xAC:
                left = (buff[0] - 0xAA) * 8;
                mask = 0xFF << left;

                ReadFile(com, &buff[1], 1, &count, 0);
                CHECK_RW_COUNT(1);
                file_point = (file_point & (~mask)) | (buff[1] << left);
                SEND_OK;
                printf("%06xh �ļ�ָ��: %xh\n", mask, file_point);
                break;

            case 0xA2:
                ReadFile(com, buff, 1, &count, 0);
                CHECK_RW_COUNT(1);
                rec_buff_size = buff[0] << 4;
                SEND_OK;
                printf("���û����� %xh\n", rec_buff_size);
                break;

            case 0xA3:
                // �ٽ��ļ�ĩβ, δ�ﵽ������������ 0
                memset(buff, 0, rec_buff_size);
                SetFilePointer(file, file_point, 0, FILE_BEGIN);
                ReadFile(file, buff, rec_buff_size, &count, 0);

                WriteFile(com, buff, rec_buff_size, &count, 0);
                CHECK_RW_COUNT(rec_buff_size);

                // ����У��
                buff[0] = crc(buff, rec_buff_size);
                WriteFile(com, buff, 1, &count, 0);
                CHECK_RW_COUNT(1);

                file_point += rec_buff_size;

                if (file_point % (20 * rec_buff_size) == 0
                    || file_point >= filesize) {
                    print_process(file_point, filesize, buff[0]);
                }
                break;

            case 0xA4:
                file_point -= rec_buff_size;
                SEND_OK;
                printf(" << ������һ�����ݰ� [curr: %xh] \n", file_point);
                break;

            case 0xAF:
                SEND_OK;
                printf(" ... �ļ��������\n");
                return;

            default:
                printf("\n��Ч��ָ�� %xh\n", buff[0]);
                break;
        }
    }

    return;

#undef CHECK_RW_COUNT
#undef SEND_OK
rw_err:
    printf("��/д���ݴ���!\n");
    return;
}


void flash_t4(HANDLE com) {
    byte buff;
    BYTE crc;
    UINT mem_size, tmp;
    DWORD count;
    int offset;
    int i;

#define SendData(x)     \
            buff = (x); \
            WriteFile(com, &buff, 1, &count, 0); \
            if (count != 1) goto rw_err;
#define ReadData(name)  \
            ReadFile(com, &buff, 1, &count, 0); \
            if (count != 1) goto rw_err; \
            name = buff;

    SendData(0xAC);
    ReadData(mem_size);
    mem_size = 0x1 << mem_size;
    printf("оƬ���� %xh(%d) �ֽ�\n��ʼƫ��(16����):", mem_size, mem_size);

    scanf("%x", &offset);

    for (i=0; i<3; ++i) {
        SendData(0xA0 + i);
        SendData(offset << (i*8));
        ReadData(tmp);
    }

    tmp = 0x10 * 10;
    SendData(0xAA);
    SendData(tmp);
    for (i=0; i<tmp; ++i) {
        ReadData(buff);
        crc += buff;

        printf("%02x ", buff);
        if ((i+1)%16 == 0) printf("\n");
    }
    ReadData(crc);
    printf("\n[crc %02xh] ", crc);

    SendData(0xAF);
    ReadData(tmp);
    if (tmp != COMM_MSG_OK) goto rw_err;
    printf("����оƬ��ȡ\n");
    return;

rw_err:
    printf("��/д���ݴ���!\n");
    return;
#undef SendData
#undef ReadData
}


void welcome() {
printf(
    "//---------------------------------------------------------------------------//\n" \
    "// CatfoOD 2014. ˢ���ͻ���\n" \
    "// V 0.21 \n" \
    "//---------------------------------------------------------------------------//\n");
}
