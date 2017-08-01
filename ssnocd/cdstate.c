#include "cdstate.h"

/*! \file cd_drive.c
\brief CD drive onboard microcontroller HLE
*/
#define RXTRACE DEBUG

//oe rising edge to falling edge
//26 usec
#define TIME_OE 26

//serial clock rising edge of the final bit in a transmission to falling edge of start signal
//13992 usec
#define TIME_PERIODIC 13992

//start  falling edge to rising edge
//187 usec
#define TIME_START 187

//start falling edge to rising edge (slow just after power on)
//3203 usec

//poweron stable signal to first start falling edge (reset time)
//451448 usec
#define TIME_POWER_ON 451448

//time from first start falling edge to first transmission
#define TIME_WAITING 416509

//from falling edge to rising edge of serial clock signal for 1 byte
#define TIME_BYTE 150

//"when disc is reading transactions are ~6600us apart"
#define TIME_READING 6600

//When reading sectors SH1 expects irq7 delta timing to be within a margin of -/+ 28 ticks. Adjust as required
#define TIME_READSECTOR 8730

//1 second / 75 sectors == 13333.333...
//2299 time for transferring 13 bytes, start signal etc
#define TIME_AUDIO_SECTOR (13333 - 2299)

#define CDLOG DEBUG

static struct CdDriveContext cdd_cxt;
static enum CommunicationState comm_state = NoTransfer;
static Byte lid_closed = 0;
static Byte has_disk = 0;

static INLINE u8 num2bcd(u8 num);
static INLINE void fad2msf_bcd(s32 fad, u8 *msf);

void make_status_data(struct CdState *state, u8* data);
void set_checksum(u8 * data);
s32 toc_10_get_track(s32 fad);
void state_set_msf_info(struct CdState *state, s32 track_fad, s32 disc_fad);
static INLINE u32 msf_bcd2fad(u8 min, u8 sec, u8 frame);
s32 get_track_fad(int track_num, s32 fad, int * index);
s32 get_track_start_fad(int track_num);
void update_seek_status();


static INLINE void fad2msf(s32 fad, u8 *msf) {
  msf[0] = fad / (75 * 60);
  fad -= msf[0] * (75 * 60);
  msf[1] = fad / 75;
  fad -= msf[1] * 75;
  msf[2] = fad;
}


// 转为 10 进制表示的数
static INLINE u8 num2bcd(u8 num) {
  return ((num / 10) << 4) | (num % 10);
}


static INLINE void fad2msf_bcd(s32 fad, u8 *msf) {
  fad2msf(fad, msf);
  msf[0] = num2bcd(msf[0]);
  msf[1] = num2bcd(msf[1]);
  msf[2] = num2bcd(msf[2]);
}


static INLINE u8 bcd2num(u8 bcd) {
  return (bcd >> 4) * 10 + (bcd & 0xf);
}


static INLINE u32 msf_bcd2fad(u8 min, u8 sec, u8 frame) {
  u32 fad = 0;
  fad += bcd2num(min);
  fad *= 60;
  fad += bcd2num(sec);
  fad *= 75;
  fad += bcd2num(frame);
  return fad;
}


u8 cd_drive_get_serial_byte() {
  return cdd_cxt.state_data[cdd_cxt.byte_counter];
}


/* 
 * 这个算法复制了 cd_drive_set_serial_bit 中的代码
 */
void cd_drive_set_serial_byte(u8 data) {
  cdd_cxt.received_data[cdd_cxt.byte_counter] = data;
  cdd_cxt.byte_counter++;
  cdd_cxt.bit_counter = 0;

  // sh1_set_output_enable_rising_edge();

  if (comm_state == SendingFirstByte)
    comm_state = WaitToOeFirstByte;
  else if (comm_state == SendingByte)
    comm_state = WaitToOe;

  if (cdd_cxt.byte_counter == 13)
    comm_state = WaitToRxio;
}


u8 cd_drive_get_serial_bit()
{
  u8 bit = 1 << (7 - cdd_cxt.bit_counter);
  return (cdd_cxt.state_data[cdd_cxt.byte_counter] & bit) != 0;
}


void cd_drive_set_serial_bit(u8 bit)
{
  cdd_cxt.received_data[cdd_cxt.byte_counter] |= bit << cdd_cxt.bit_counter;
  cdd_cxt.bit_counter++;

  if (cdd_cxt.bit_counter == 8)
  {
    // tsunami_log_value("CMD", cdd_cxt.received_data[cdd_cxt.byte_counter], 8);

    cdd_cxt.byte_counter++;
    cdd_cxt.bit_counter = 0;

    // sh1_set_output_enable_rising_edge();

    if (comm_state == SendingFirstByte)
      comm_state = WaitToOeFirstByte;
    else if (comm_state == SendingByte)
      comm_state = WaitToOe;

    if (cdd_cxt.byte_counter == 13)
      comm_state = WaitToRxio;
  }
}


void do_toc()
{
  int toc_entry;
  cdd_cxt.state_data[0] = cdd_cxt.state.current_operation = ReadToc;
  comm_state = NoTransfer;
  //fill cdd_cxt.state_data with toc info

  toc_entry = cdd_cxt.toc_entry++;
  memcpy(cdd_cxt.state_data + 1, &cdd_cxt.toc[toc_entry], 10);

  set_checksum(cdd_cxt.state_data);

  if (cdd_cxt.toc_entry >= cdd_cxt.num_toc_entries)
  {
    cdd_cxt.state.current_operation = Idle;
    make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
  }
}


void update_status_info()
{
  int index = 0;
  s32 track_num = 0;
  s32 track_fad = 0;
  s32 track_start_fad = 0;
  if (cdd_cxt.disc_fad >= get_track_start_fad(-1)) {
    track_fad = cdd_cxt.disc_fad;
  } else {
    track_num = toc_10_get_track(cdd_cxt.disc_fad);
    track_fad = get_track_fad(track_num, cdd_cxt.disc_fad + 4, &index);
    track_start_fad = get_track_start_fad(track_num);
    track_fad = cdd_cxt.disc_fad - track_start_fad;
  }

  if (track_fad < 0)
    track_fad = -track_fad;
  else
    index = 1;

  state_set_msf_info(&cdd_cxt.state, track_fad, cdd_cxt.disc_fad);

  if (cdd_cxt.disc_fad >= get_track_start_fad(-1)) {   // leadout
    cdd_cxt.state.q_subcode = 1;
    cdd_cxt.state.index_field = 1;
    cdd_cxt.state.track_number = 0xaa;
  } else {
    cdd_cxt.state.q_subcode = cdd_cxt.tracks[track_num - 1].ctrladr;
    cdd_cxt.state.index_field = index;
    cdd_cxt.state.track_number = num2bcd(track_num);
  }
}


static u8 ror(u8 in) 
{
  return (in>>1) | (in<<(7));
}


static void make_ring_data(u8 *buf) 
{
  u32 i,j;
  u16 lfsr = 1;
  u8 a;
  for (i=12; i<2352; i++) 
  {
    a = (i & 1) ? 0x59 : 0xa8;
    for (j=0; j<8; j++) 
    {
      u32 x = a;
      u32 var2 = lfsr & 1;
      a ^= var2;
      a = ror(a);

      x = lfsr;
      x >>= 1;
      var2 = lfsr;
      x ^= var2;
      lfsr |= x << 15;
      lfsr >>= 1;
    }
    buf[i-12] = a;
  }
}

void do_dataread()
{
  u8 buf[2448];		
  u32 dest;
  int i;

  CDLOG("running DMA to %X(FAD: %d)\n", 0, cdd_cxt.disc_fad);

  if (cdd_cxt.disc_fad < 150)
  {
    memset(buf, 0, sizeof(buf));
    fad2msf_bcd(cdd_cxt.disc_fad, buf+12);
    buf[15] = 1;
  }
  else if (cdd_cxt.disc_fad >= get_track_start_fad(-1))
  {
    u8 *subbuf=buf+12;
    // fills all 2352 bytes
    make_ring_data(subbuf);

    fad2msf_bcd(cdd_cxt.disc_fad, subbuf);
    subbuf[3] = 2;	// Mode 2, Form 2
                    // 8 byte subheader (unknown purpose)
    subbuf[4] = 0; subbuf[5] = 0; subbuf[6] = 28; subbuf[7] = 0;
    subbuf[8] = 0; subbuf[9] = 0; subbuf[10] = 28; subbuf[11] = 0;

    // 4 byte error code at end
    subbuf[2352-4] = 0;
    subbuf[2352-3] = 0;
    subbuf[2352-2] = 0;
    subbuf[2352-1] = 0;
  }
  else {
    // Cs2Area->cdi->ReadSectorFAD(cdd_cxt.disc_fad, buf);
    cd_read_sector(cdd_cxt.disc_fad, buf, sizeof(buf));
  }

  cdi_sector_data_ready(buf, sizeof(buf), 0);
    
  printf("sector head:");
  for (i=12; i<16; i++)
    printf(" %02X", buf[i]);
  printf("\n");

  cdd_cxt.disc_fad++;
}

void make_ring_status()
{
  u32 fad = cdd_cxt.disc_fad + 4;
  u8 msf_buf[3] = { 0 };
  fad2msf_bcd(cdd_cxt.disc_fad, msf_buf);

  cdd_cxt.state_data[0] = SeekSecurityRing2;
  cdd_cxt.state_data[1] = 0x44;
  cdd_cxt.state_data[2] = 0xf1;
  cdd_cxt.state_data[3] = fad >> 16;
  cdd_cxt.state_data[4] = fad >> 8;
  cdd_cxt.state_data[5] = fad;
  cdd_cxt.state_data[6] = 0x09;
  cdd_cxt.state_data[7] = 0x09;
  cdd_cxt.state_data[8] = 0x09;
  cdd_cxt.state_data[9] = 0x09;
  cdd_cxt.state_data[10] = msf_buf[2];

  set_checksum(cdd_cxt.state_data);
}


u32 get_fad_from_command(u8 * buf)
{
  u32 fad = buf[1];
  fad <<= 8;
  fad |= buf[2];
  fad <<= 8;
  fad |= buf[3];

  return fad;
}


s32 get_track_start_fad(int track_num)
{
  s32 fad;
  if (track_num == -1) // leadout
    track_num = cdd_cxt.num_tracks;
  else                 // normal track (1-based)
    track_num--;
  fad = msf_bcd2fad(cdd_cxt.tracks[track_num].pmin, cdd_cxt.tracks[track_num].psec, cdd_cxt.tracks[track_num].pframe);
  return fad;
}

s32 get_track_fad(int track_num, s32 fad, int * index)
{
  s32 track_start_fad = get_track_start_fad(track_num);
  s32 track_fad = fad - track_start_fad;

  if (track_fad < 0)
    *index = 1;

  return track_fad;
}


void update_seek_status()
{
  update_status_info();
  cdd_cxt.state.current_operation = Seeking;

  make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
  comm_state = NoTransfer;
}


void do_seek_common(u8 post_seek_state)
{
  s32 fad = get_fad_from_command(cdd_cxt.received_data);
  cdd_cxt.disc_fad = fad - 4;
  cdd_cxt.target_fad = cdd_cxt.disc_fad;
  cdd_cxt.seek_time = 0;
  cdd_cxt.post_seek_state = post_seek_state;
}


void do_seek()
{
  do_seek_common(Idle);
  update_seek_status();
}


//
// 当命令p4 为 00 的时候执行
//
int continue_command()
{
  if (cdd_cxt.state.current_operation == Idle)
  {
    comm_state = NoTransfer;
    cdd_cxt.disc_fad++;

    //drive head moves back when fad is too high
    if (cdd_cxt.disc_fad > cdd_cxt.target_fad + 5)
      cdd_cxt.disc_fad = cdd_cxt.target_fad;

    update_status_info();
    make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
    return TIME_PERIODIC/cdd_cxt.speed;
  }
  else if (cdd_cxt.state.current_operation == ReadingDataSectors ||
    cdd_cxt.state.current_operation == ReadingAudioData)
  {
    int is_audio = 1;

    if (cdd_cxt.disc_fad < 150)
      is_audio = 0;

    if (cdd_cxt.disc_fad >= get_track_start_fad(-1))
      is_audio = 0;

    if (cdd_cxt.state.current_operation != ReadingAudioData)
      is_audio = 0;

    comm_state = NoTransfer;

    if (is_audio)
    {
      u8 buf[2448];		
      //Cs2Area->cdi->ReadSectorFAD(cdd_cxt.disc_fad, buf);
      cd_read_sector(cdd_cxt.disc_fad, buf, sizeof(buf));
      //ScspReceiveCDDA(buf);
      cdi_sector_data_ready(buf, sizeof(buf), 1);
      cdd_cxt.disc_fad++;
      //Cs2Area->cdi->ReadAheadFAD(cdd_cxt.disc_fad);
    }
    else
      do_dataread();

    update_status_info();
    cdd_cxt.state.current_operation = (cdd_cxt.state.q_subcode & 0x40) ? ReadingDataSectors : ReadingAudioData;
    make_status_data(&cdd_cxt.state, cdd_cxt.state_data);

    if (is_audio)
      return TIME_AUDIO_SECTOR;
    else
      return TIME_READSECTOR / cdd_cxt.speed;
  }
  else if (cdd_cxt.state.current_operation == Stopped)
  {
    comm_state = NoTransfer;
    return TIME_PERIODIC / cdd_cxt.speed;
  }
  else if (cdd_cxt.state.current_operation == ReadToc)
  {
    do_toc();
    return TIME_READING;
  }
  else if (cdd_cxt.state.current_operation == Seeking || 
    cdd_cxt.state.current_operation == SeekSecurityRing2)
  {

    cdd_cxt.seek_time++;
    update_seek_status();

    if (cdd_cxt.seek_time > 9)
    {
      //seek completed, change for next status
      cdd_cxt.state.current_operation = cdd_cxt.post_seek_state;
    }
    comm_state = NoTransfer;
    return TIME_READING;
  }
  else
  {
    comm_state = NoTransfer;
    return TIME_PERIODIC / cdd_cxt.speed;
  }
}


//
// 执行 cpu 发来的命令
// 前置 cd_command_exec
//
int do_command()
{
  int command = cdd_cxt.received_data[0];

  if (cdd_cxt.received_data[10] == 1)
    cdd_cxt.speed = 1;
  else
    cdd_cxt.speed = 2;

  switch (command) // cpu 发来的命令 p3
  {
  case 0x0:
    //nop
    return continue_command();
    break;
  case 0x2:
    //seeking ring
    CDLOG("seek ring\n");
    cdd_cxt.state.current_operation = SeekSecurityRing2;
    do_seek_common(Idle);
    make_ring_status();
    comm_state = NoTransfer;
    return TIME_READING / cdd_cxt.speed;
    break;
  case 0x3:
  {
    int i, track_num, max_track = 0;
    CDInterfaceToc10 *toc;
    CDLOG("read toc\n");
    cdd_cxt.toc_entry = 0;

    // 从光盘上读取 toc
    //Cs2Area->cdi->ReadTOC10(toc);
    cdd_cxt.num_toc_entries = cd_get_toc_count();

    // Read and sort the track entries, so you get one entry for each track + leadout, in starting FAD order.
    for (i = 0; i < cdd_cxt.num_toc_entries; i++)
    {
      cd_get_toc(&toc, i);
      CDInterfaceToc10 *entry = &cdd_cxt.toc[i*3];
      entry->ctrladr = toc->ctrladr;
      entry->tno = toc->tno;
      if(toc->point > 0x99)
        entry->point = toc->point;
      else
        entry->point = toc->point = num2bcd(toc->point);
      entry->min = num2bcd(toc->min);
      entry->sec = num2bcd(toc->sec);
      entry->frame = num2bcd(i*3);
      entry->zero = 0;
      entry->pmin = num2bcd(toc->pmin);
      entry->psec = num2bcd(toc->psec);
      entry->pframe = num2bcd(toc->pframe);

      memcpy(&cdd_cxt.toc[i*3+1], entry, sizeof(*entry));
      cdd_cxt.toc[i*3+1].frame = num2bcd(i*3+1);
      memcpy(&cdd_cxt.toc[i*3+2], entry, sizeof(*entry));
      cdd_cxt.toc[i*3+2].frame = num2bcd(i*3+2);

      if (entry->point <= 0x99) 
      {
        track_num = bcd2num(entry->point);
        if (track_num > max_track)
          max_track = track_num;
        memcpy(&cdd_cxt.tracks[track_num-1], entry, sizeof(*entry));
      }
    }
    cdd_cxt.num_toc_entries *= 3;
    for (i = 0; i < cdd_cxt.num_toc_entries; i++)
    {
      if (cdd_cxt.toc[i].point == 0xa2) {  // leadout
        memcpy(&cdd_cxt.tracks[max_track], &cdd_cxt.toc[i], sizeof(cdd_cxt.toc[i]));
        break;
      }
    }
    cdd_cxt.num_tracks = max_track;
    do_toc();

    return TIME_READING;
    break;
  }
  case 0x4:
    //stop disc
    CDLOG("stop disc\n");
    cdd_cxt.state.current_operation = Stopped;
    make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
    comm_state = NoTransfer;
    return TIME_PERIODIC / cdd_cxt.speed;
    break;
  case 0x5:
    //prevent scan from crashing the drive for now
  case 0xa:
  case 0xb:
    CDLOG("unknown command 5\n");
    //just idle for now
    cdd_cxt.state.current_operation = Idle;
    make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
    comm_state = NoTransfer;
    return TIME_PERIODIC / cdd_cxt.speed;
    break;
  case 0x6:
  {
    do_seek_common(ReadingDataSectors);
    update_seek_status();

    comm_state = NoTransfer;
    return TIME_READSECTOR / cdd_cxt.speed;
  }
  case 0x8:
    //pause
    CDLOG("pause\n");
    cdd_cxt.state.current_operation = Idle;
    make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
    comm_state = NoTransfer;
    return TIME_PERIODIC / cdd_cxt.speed;
    break;
  case 0x9:
    //seek
  {
    CDLOG("seek\n");
    do_seek();
    return TIME_READING;
    break;
  }
#if 0
  case 0xa:
    //scan forward
    CDLOG("scan forward\n");
    break;
  case 0xb:
    //scan backwards
    CDLOG("scan backwards\n");
    break;
#endif
  }

  DEBUG("Unknow command '0x%x'\n", command);
  assert(0);

  return TIME_PERIODIC;
}


//
// 根据当前状态 comm_state 执行不同的行为
// 后置 do_command
//
int cd_command_exec()
{
  if (comm_state == Reset)
  {
    if (lid_closed) {
      if (has_disk) {
        cdd_cxt.state.current_operation = Stopped;
      } else {
        cdd_cxt.state.current_operation = NoDisc;
      }
    } else {
      cdd_cxt.state.current_operation = LidOpen;
    }
    // cdd_cxt.state.current_operation = Idle;
    make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
    comm_state = NoTransfer;
    return TIME_POWER_ON + TIME_WAITING;
  }
  else if (
    comm_state == SendingFirstByte || 
    comm_state == SendingByte)
  {
    return TIME_BYTE;
  }
  else if (
    comm_state == NoTransfer)
  {
    cdd_cxt.bit_counter = 0;
    cdd_cxt.byte_counter = 0;
    comm_state = SendingFirstByte;

    memset(&cdd_cxt.received_data, 0, sizeof(u8) * 13);

    // sh1_set_start(1);
    // sh1_set_output_enable_falling_edge();
    cdi_update_drive_bit();

    return TIME_START;
  }
  //it is required to wait to assert output enable
  //otherwise some sort of race condition occurs
  //and breaks the transfer
  else if (comm_state == WaitToOeFirstByte)
  {
    // sh1_set_output_enable_falling_edge();
    // sh1_set_start(0);
    comm_state = SendingByte;
    return TIME_OE;
  }
  else if (comm_state == WaitToOe)
  {
    // sh1_set_output_enable_falling_edge();
    comm_state = SendingByte;

    return TIME_OE;
  }
  else if (comm_state == WaitToRxio)
  {
#ifdef CDDEBUG
    //debug logging
    static u8 previous_state = 0;

    if (cdd_cxt.received_data[11] != 0xff && cdd_cxt.received_data[0])
    {
      char str[1024] = { 0 }; 
      int i;

      for (i = 0; i < 13; i++)
        sprintf(str + strlen(str), "%02X ", cdd_cxt.received_data[i]);

      CDLOG("CMD: %s\n", str);
    }

    if (cdd_cxt.state_data[0] != previous_state)
    {
      char str[1024] = { 0 };
      int i;

      previous_state = cdd_cxt.state_data[0];

      for (i = 0; i < 13; i++)
        sprintf(str + strlen(str), "%02X ", cdd_cxt.state_data[i]);

      CDLOG("STA: %s\n", str);
    }
#endif

#ifdef DO_LOGGING
    do_cd_logging();
#endif

    //handle the command
    return do_command();
  }

  assert(0);

  cdd_cxt.num_execs++;
  return 1;
}


s32 cd_drive_exec(s32 cycles)
{
  s32 cycles_temp = cdd_cxt.cycles_remainder - cycles;
  while (cycles_temp < 0)
  {
    int cycles_exec = cd_command_exec();
    cycles_temp += cycles_exec;
  }
  cdd_cxt.cycles_remainder = cycles_temp;
  return cycles_temp;
}


void set_checksum(u8 * data)
{
  u8 parity = 0;
  int i = 0;
  for (i = 0; i < 11; i++)
    parity += data[i];
  data[11] = ~parity;
  data[12] = 0;
}


s32 toc_10_get_track(s32 fad)
{
  int i = 0;
  if (!cdd_cxt.num_toc_entries)
    return 1;

  for (i = 0; i < cdd_cxt.num_toc_entries; i++)
  {
    s32 track_start = get_track_start_fad(i+1);
    s32 track_end  = get_track_start_fad(i+2);

    if (fad >= track_start && fad < track_end)
      return (i + 1);

    if (i == 0 && fad < track_start)
      return 1;//lead in
  }

  return 0;
}


void state_set_msf_info(struct CdState *state, s32 track_fad, s32 abs_fad)
{
  u8 msf_buf[3] = { 0 };
  fad2msf_bcd(track_fad, msf_buf);
  state->minutes = msf_buf[0];
  state->seconds = msf_buf[1];
  state->frame = msf_buf[2];
  fad2msf_bcd(abs_fad, msf_buf);
  state->absolute_minutes = msf_buf[0];
  state->absolute_seconds = msf_buf[1];
  state->absolute_frame = msf_buf[2];
}


void make_status_data(struct CdState *state, u8* data)
{
  int i = 0;
  data[0] = state->current_operation;
  data[1] = state->q_subcode;
  data[2] = state->track_number;
  data[3] = state->index_field;
  data[4] = state->minutes;
  data[5] = state->seconds;
  data[6] = state->frame;
  data[7] = STATE_FIX_ZERO; //or zero?
  data[8] = state->absolute_minutes;
  data[9] = state->absolute_seconds;
  data[10] = state->absolute_frame;

  set_checksum(data); 

  if (data[0] && data[0] != 0x46) {
    RXTRACE("STA: ");

    for (i = 0; i < 13; i++)
      RXTRACE(" %02X", data[i]);
    RXTRACE("\n");
  }
}


char* get_status_string(int status)
{
  u32 track_fad = msf_bcd2fad(cdd_cxt.state_data[4], cdd_cxt.state_data[5], cdd_cxt.state_data[6]);
  u32 abs_fad = msf_bcd2fad(cdd_cxt.state_data[8], cdd_cxt.state_data[9], cdd_cxt.state_data[10]);

  static char str[256] = { 0 };

  switch (status)
  {
  case Idle:
    sprintf(str, "%s %d %d", "Idle", track_fad, abs_fad);
    return str;
  case Stopped:
    return "Stopped";
  case Seeking:
    sprintf(str, "%s %d %d", "Seeking", track_fad, abs_fad);
    return str;
  case ReadingDataSectors:
    sprintf(str, "%s %d %d", "Reading Data Sectors", track_fad, abs_fad);
    return str;
  case ReadingAudioData:
    sprintf(str, "%s %d %d", "Reading Audio Data", track_fad, abs_fad);
    return str;
  case LidOpen:
    return "Lid Opend";
  case NoDisc:
    return "No Disc";
  case ReadToc:
    return "Read Toc";
  default:
    return "";
  }
  return "";
}


char * get_command_string(int command)
{
  u32 fad = get_fad_from_command(cdd_cxt.received_data);

  static char str[256] = { 0 };

  switch (command)
  {
  case 0x0:
    return "";
  case 0x2:
    return "Seeking Ring";
  case 0x3:
    return "Read TOC";
  case 0x4:
    return "Stop Disc";
  case 0x6:
    sprintf(str, "%s %d", "Read", fad);
    return str;
  case 0x8:
    return "Pause";
  case 0x9:
    sprintf(str, "%s %d", "Seek", fad);
    return str;
  default:
    return "";
  break;
  }
  return "";
}


void do_cd_logging()
{
  char str[1024] = { 0 };
  int i;

  for (i = 0; i < 12; i++)
    sprintf(str + strlen(str), "%02X ", cdd_cxt.received_data[i]);

  sprintf(str + strlen(str), " || ");

  for (i = 0; i < 12; i++)
    sprintf(str + strlen(str), "%02X ", cdd_cxt.state_data[i]);

  sprintf(str + strlen(str), " %s ||  %s", get_command_string(cdd_cxt.received_data[0]), get_status_string(cdd_cxt.state_data[0]));

  CDLOG("%s\n", str);
}


void cdd_reset()
{
  memset(&cdd_cxt, 0, sizeof(struct CdDriveContext));
  has_disk = 0;
  lid_closed = 0;
  comm_state = Reset;
}


void cdd_open_lid() {
  lid_closed = 0;
  has_disk = 0;
  cdd_cxt.state.current_operation = LidOpen;
  make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
  comm_state = NoTransfer;
}


void ccd_close_lid() {
  lid_closed = 1;
  cdd_cxt.state.current_operation = Stopped;
  make_status_data(&cdd_cxt.state, cdd_cxt.state_data);
  comm_state = NoTransfer;
}


void ccd_insert_disk() {
  if (!lid_closed) {
    has_disk = 0;
  }
};