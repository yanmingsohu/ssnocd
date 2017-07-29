#ifndef ___LIB_H__
#define ___LIB_H__

#ifdef _WIN32_WINNT
#include <stdio.h>
#include <assert.h>
#include <string.h>
#define DEBUG printf
#define INLINE _inline
#else
// #error Cannot support Arduino.
#define _ARDUINO
#define DEBUG
#define strcpy x_strcpy
#define memcpy x_memcpy
#define INLINE inline
int x_strcpy(char* desc,char* src, int max);
int x_memcpy(void* desc, void* src, int size);
#endif

#define __countof(x) ((sizeof(x) / sizeof((x)[0])))
#define SUCCESS 0
#define FAILED  1

#define SP1 "  "
#define SP2 SP1 SP1
#define SP3 SP1 SP2
#define SP4 SP1 SP3

typedef unsigned char       Byte;
typedef Byte*               pByte;
typedef Byte                UCHAR;
typedef unsigned int        UINT;
typedef unsigned short      USHORT;
typedef unsigned long long  ULONGLONG;


#endif // ___LIB_H__