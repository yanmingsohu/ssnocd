#include "lib.h"
#include "mds.h"
#include "fs.h"
#include "cdstate.h"
#include "cd-img.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <conio.h>


/* 不同的主机, 文件不同, 在这里定义文件路径
 * #define MDS_PATH "mds文件的完整路径" 
 */
#include "file.h"
#define New_line  putchar('\n')

static Byte cmd[13] = {0};
static int state_updated = 0;
static int state_count = 1;

void test(pByte cmd);


void print_toc(Toc *toc) {
  DEBUG(SP1"TOC: %02x(AT) %02x(TNO) %02x(PT)  "
           "%02x %02x %02x %02x(0) %02x %02x %02x\n",
        toc->ctrladr, toc->tno,  toc->point, 
        toc->min,     toc->sec,  toc->frame, toc->zero,
        toc->pmin,    toc->psec, toc->pframe);
}


void print_buf(UCHAR *buf, int len, int beingpos) {
#define C(a)   (a>0x1F && a<0x80?a:'.')

  int loop = len < 16 ? len % 16 : 16;
  for (int i=0; i<len; i+=16) {
    printf("%08x"SP1, i + beingpos);
    for (int j=0; j<loop; ++j) {
      if (j == 8) printf(SP1);
      printf(" %02X", (buf[i+j]));
    }
    printf(SP1);
    for (int j=0; j<loop; ++j) {
      printf("%c", C(buf[i+j]) );
    }
    New_line;
  }

#undef C
}


void output_state(UCHAR *state) {
  printf("[%04d] %02x - %02x %02x %02x  %02x %02x %02x  (%02x)  %02x %02x %02x  %02x %02x", 
  state_count, state[0], state[1], state[2], state[3], state[4], state[5], state[6],
  state[7], state[8], state[9], state[10], state[11], state[12]);
  ++state_count;
}


void cdi_sector_data_ready(pByte buf, int buflen, char is_audio) {
  if (is_audio) {
    printf("\nAudio sector %d bytes:\n", buflen);
  } else {
    printf("\nDate sector %d bytes:\n", buflen);
  }
  print_buf(buf, 0x10 * 4, 0);
  New_line;
}


void cdi_update_drive_bit() {
  state_updated = 1;
  set_checksum(cmd);
  
  Byte state[13] = {0};
  Byte bit;

  /*
  for (int i=0; i<13; ++i) {
    pByte p = &state[i];

    for (int b=7; b>=0; --b) {
      bit = cd_drive_get_serial_bit();
      *p |= (bit << b);
      bit = cmd[i] & (1 << (7-b));
      cd_drive_set_serial_bit(bit > 0);
    }
  }
  */
  for (int i=0; i<13; ++i) {
    state[i] = cd_drive_get_serial_byte();
    cd_drive_set_serial_byte(cmd[i]);
  }

  printf("\n- CD Drive \n- Sta : ");
  output_state(state);
  printf("  | %s\n", get_status_string(state[0]));
  printf("- Cmd : ");
  output_state(cmd);
  printf("  | %s\n", get_command_string(cmd[0]));
  memset(cmd, 0, 13);
}


int asc_to_num(int ch) {
  if (ch >= 48 && ch < 58) {
    return ch - 48;
  } else if (ch >= 65 && ch < 71) {
    return ch - 65 + 0xA;
  } else if (ch >= 97 && ch < 103) {
    return ch - 97 + 0xA;
  }
  return -1;
}


void console() {
  cdd_reset();

  int ch;
  int count = 0;
  int low = 0;
  int n;
  int new_line = 0;

  printf("\n[HELP]\n\
    press a key to command:\n\
      q : Quit\n\
      t : Test suit\n\
      r : Reset Drive\n\
      i : Close Lid CD\n\
      o : Open Lid CD\n\
    press ENTER to do next command\n");

  printf("\n> ");

  for (;;) {
    new_line = 0;
    ch = getch();

    switch(ch) {
    case 'q':
      printf("quit\n");
      return 0;

    case ' ':
    case '\t':
      low = 0;
      if (++count >= 13) count = 0;
      break;

    case 'o':
      printf("Open lid\n");
      cdd_open_lid();
      new_line = 1;
      break;

    case 'i':
      printf("Insert disk and close lid\n");
      ccd_insert_disk();
      ccd_close_lid();
      new_line = 1;
      break;

    case 't':
      test(cmd);
      new_line = 1;
      break;

    case 'r':
      printf("reset\n");
      cdd_reset();
      new_line = 1;
      break;

    case '\n':
    case '\r':
      count = 0;
      low = 0;
      while (!state_updated) {
        printf("\n- cd command exec.");
        cd_command_exec();
      }
      state_updated = 0;
      new_line = 1;
      break;

    default:
      n = asc_to_num(ch);
      if (n < 0) {
        printf("bad command key '%c'\n", ch);
        count = 0;
        low = 0;
        break;
      }
      if (low) {
        cmd[count] |= n;
        low = 0;
      } else {
        cmd[count] = (n << 4);
        low = 1;
      }
      break;
    }

    if (new_line) printf("\n> ");
    else putch(ch);
  }
}


int main() {
  assert(1 == sizeof(UCHAR));
  assert(2 == sizeof(USHORT));
  assert(4 == sizeof(UINT));
  assert(8 == sizeof(ULONGLONG));
  assert(sizeof(MDSTrackBlock) == 80);
  assert(sizeof(MDSSessionBlock) == 24);
  assert(sizeof(MDSHeader) == 92);

  mds_open(MDS_PATH);
  int tocc = cd_get_toc_count();
  Toc *toc;
  DEBUG("\nToc has %d\n", tocc);

  for(int i=0; i<tocc; ++i) {
    cd_get_toc(&toc, i);
    print_toc(toc);
  }

  DEBUG("Sector Count %d\n", cd_get_sector_count());
  DEBUG("Data Track Count %d\n", cd_get_track_count());

  UCHAR buf[MAX_SECTOR_SIZE];

  /*
  for (int i=0; i<3; ++i) { // !
    int sector = i;
    int rlen = cd_read_sector(sector, buf, MAX_SECTOR_SIZE);
    DEBUG("\nSector %d len: %d bytes\n", sector, rlen);
    print_buf(buf, rlen, 0);
  } */
  
  int begin_sec = 150;
  for (int i=begin_sec; i<begin_sec + 5; ++i) {
    DEBUG("\nSector %d Heads: \n", i);
    int rlen = cd_read_sector(i, buf, MAX_SECTOR_SIZE);
    if (rlen == 0) {
      DEBUG("BAD cannot read sector %d %d\n", i, rlen);
      break;
    }
    print_buf(buf, 24, 0);
  }

  console();

  return 0;
}


