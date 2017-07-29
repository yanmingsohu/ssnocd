/**
 * fs 将记录一个打开的文件, 且只能打开一个文件
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

/* 获取当前状态 */
FileState fs_state();
/* 打开文件 */
void fs_open(pByte filename);
/* 关闭文件 */
void fs_close();
/* 设置读取指针 */
int fs_seek(ULONGLONG offset);
/* 读取数据到缓冲区, 返回实际读取字节 */
int fs_read(void* buf, int len);
/* 返回文件长度 */
int fs_size();
/* 读取字符串 */
int fs_read_string(pByte buf, int max);


#endif