#ifndef ___LIB_H__
#define ___LIB_H__

#ifdef _WIN32_WINNT
#include <stdio.h>
#include <assert.h>
#else
#error Cannot support Arduino.
#define _ARDUINO
#endif

#define __countof(x) ((sizeof(x) / sizeof((x)[0])))
#define SUCCESS 0
#define FAILED  1
#define DEBUG   printf

int strcpy(char* desc,char* src, int max);

typedef unsigned char   Byte;
typedef Byte*           pByte;
typedef Byte            UCHAR;
typedef unsigned short  USHORT;
typedef unsigned long long ULONGLONG;

#endif // ___LIB_H__