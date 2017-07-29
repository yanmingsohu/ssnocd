#ifndef ___LIB_H__
#define ___LIB_H__

#ifdef _WIN32_WINNT
	#include <stdio.h>
	#include <assert.h>
	#include <string.h>
	
	#define DEBUG printf
	#define INLINE _inline
	
	typedef enum {ERROR = 0, SUCCESS = !ERROR} ErrorStatus;
	typedef unsigned char       Byte;
	typedef Byte*               pByte;
	typedef Byte                UCHAR;
	typedef unsigned int        UINT;
	typedef unsigned short      USHORT;
	typedef unsigned long long  ULONGLONG;
	
#elif defined(STM32F40_41xxx)
	#include <stm32f4xx.h>
	
	#define _MCU
	#define DEBUG printf
	#define strcpy x_strcpy
	#define memcpy x_memcpy
	#define INLINE inline
		
	int x_strcpy(char* desc,char* src, int max);
	int x_memcpy(void* desc, void* src, int size);
	
	typedef uint8_t             Byte;
	typedef Byte*               pByte;
	typedef Byte                UCHAR;
	typedef uint32_t        		UINT;
	typedef uint16_t     				USHORT;
	typedef uint64_t			      ULONGLONG;
	
	#define assert(expression) \
					(!!(expression)) || \
					(printf("Assert fail %s, in file %s at %d", #expression, __FILE__, __LINE__) )
	
#else 
	#error Cannot support current platform.
#endif

#define __countof(x) ((sizeof(x) / sizeof((x)[0])))
#define FAILED   ERROR

#define SP1 "  "
#define SP2 SP1 SP1
#define SP3 SP1 SP2
#define SP4 SP1 SP3


#endif // ___LIB_H__
