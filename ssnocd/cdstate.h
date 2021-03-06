#ifndef __CD_STATE_H__
#define __CD_STATE_H__
#include "lib.h"
#include "cd-img.h"

#pragma pack(push)
#pragma pack(1)

/* 状态第7位数据, 数据定义是 0x0, 实机返回 0x7, 模拟器 0x4. */
#define STATE_FIX_ZERO 0x07

typedef Toc            CDInterfaceToc10;

#ifdef _WIN32_WINNT
typedef unsigned long  u32;
typedef signed long    s32;
typedef unsigned short u16;
typedef signed short   s16;
typedef Byte           u8;
typedef signed char    s8;
#endif

#ifdef STM32F40_41xxx
#include <stm32f4xx.h>
/* All defined in 'stm32f4xx.h'
typedef unsigned long  u32;
typedef signed long    s32;
typedef unsigned short u16;
typedef signed short   s16;
typedef Byte           u8;
typedef signed char    s8;
*/
#endif


enum CdStatusOperations
{
  ReadToc = 0x04, // 0x06 ?
  Idle = 0x46,
  Stopped = 0x12,
  Seeking = 0x22,
  LidOpen = 0x80,
  NoDisc = 0x83,
  ReadingDataSectors = 0x36,
  ReadingAudioData = 0x34,
  Unknown = 0x30,
  SeekSecurityRing1 = 0xB2,
  SeekSecurityRing2 = 0xB6,
  Waiting = 0x42, // 正在读取数据时的延迟周期
};


enum CommunicationState
{
  NoTransfer,
  Reset,
  Started,
  SendingFirstByte,
  ByteFinished,
  FirstByteFinished,
  SendingByte,
  SendingByteFinished,
  Running,
  NewTransfer,
  WaitToOe,
  WaitToOeFirstByte,
  WaitToRxio
};


struct CdState
{
  u8 current_operation;//0
  u8 q_subcode;//1
  u8 track_number;//2
  u8 index_field;//3
  u8 minutes;//4
  u8 seconds;//5
  u8 frame;//6
  //7 zero
  u8 absolute_minutes;//8
  u8 absolute_seconds;//9
  u8 absolute_frame;//10
  //11 parity
  //12 zero
};


struct CdDriveContext
{
  s32 cycles_remainder;

  int num_execs;
  int output_enabled;
  int bit_counter;
  int byte_counter;

  struct CdState state;
  u8 state_data[13];
  u8 received_data[13];
  int received_data_counter;
  u8 post_seek_state;

  CDInterfaceToc10 toc[MAX_TOC_NUM*3], tracks[MAX_TOC_NUM];
  int toc_entry;
  int num_toc_entries, num_tracks;

  u32 disc_fad;
  u32 target_fad;

  int seek_time;
  int speed;
};


void cdd_reset();
/* 按照时序运行命令, cycles 不到达上一条命令的延迟, 则什么也不做 */
s32 cd_drive_exec(s32 cycles);
/* 直接执行命令, 无视时序 */
int cd_command_exec();
void set_checksum(u8 * data);

u8 cd_drive_get_serial_bit();
void cd_drive_set_serial_bit(u8 bit);

void cdd_open_lid();
void ccd_close_lid();
void ccd_insert_disk();

/* 代替 cd_drive_XXX_serial_bit 直接操作一个字节 */
u8 cd_drive_get_serial_byte();
void cd_drive_set_serial_byte(u8 data);


/* 实现该接口, 当请求一节数据后, 该节数据通过该方法注入
   缓冲区长度可能大于 2352 */
void cdi_sector_data_ready(pByte buf, int buflen, char is_audio);

/* 实现该接口, cd 有新的状态发送, 该方法被调用 */
void cdi_update_drive_bit();


char* get_status_string(int status);
char * get_command_string(int command);

#pragma pack(pop)
#endif