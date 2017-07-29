/**
 * fs ����¼һ���򿪵��ļ�, ��ֻ�ܴ�һ���ļ�
**/
#ifndef __FS_LIB_H__
#define __FS_LIB_H__

#include "lib.h"

#define FS_FAILED  1
#define FS_SUCCESS 0


typedef enum FileState_ {
  Closed,
  Opened,
  OpenFailed,
  CloseFailed,
} FileState;

/* ��ȡ��ǰ״̬ */
FileState fs_state();
/* ���ļ� */
void fs_open(pByte filename);
/* �ر��ļ� */
void fs_close();
/* ���ö�ȡָ�� */
int fs_seek(ULONGLONG offset);
/* ��ȡ���ݵ�������, ����ʵ�ʶ�ȡ�ֽ� */
int fs_read(void* buf, int len);
/* �����ļ����� */
int fs_size();
/* ��ȡ�ַ��� */
int fs_read_string(pByte buf, int max);


#endif