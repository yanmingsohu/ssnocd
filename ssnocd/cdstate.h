#ifndef __CD_STATE_H__
#define __CD_STATE_H__
#include "lib.h"
#include "cd-img.h"

#pragma pack(push)
#pragma pack(1)

/* ״̬��7λ����, ���ݶ����� 0x0, ʵ������ 0x7, ģ���� 0x4. */
#define STATE_FIX_ZERO 0x07

typedef Toc            CDInterfaceToc10;
typedef unsigned long  u32;
typedef signed long    s32;
typedef unsigned short u16;
typedef signed short   s16;
typedef Byte           u8;
typedef signed char    s8;


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
/* ����ʱ����������, cycles ��������һ��������ӳ�, ��ʲôҲ���� */
s32 cd_drive_exec(s32 cycles);
/* ֱ��ִ������, ����ʱ�� */
int cd_command_exec();
void set_checksum(u8 * data);

u8 cd_drive_get_serial_bit();
void cd_drive_set_serial_bit(u8 bit);


/* ʵ�ָýӿ�, ������һ�����ݺ�, �ý�����ͨ���÷���ע��
   ���������ȿ��ܴ��� 2352 */
void cdi_sector_data_ready(pByte buf, int buflen, char is_audio);

/* ʵ�ָýӿ�, cd ���µ�״̬����, �÷��������� */
void cdi_update_drive_bit();


#pragma pack(pop)
#endif