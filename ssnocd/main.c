#include "lib.h"
#include "mds.h"
#include "fs.h"
#include "cdstate.h"
#include "cd-img.h"

//#define MDS_PATH "D:\\Saturn-cd-emu\\大航海时代2[简][V1.01]\\大航海时代2.mds"
#define MDS_PATH "E:\\BaiduNetdiskDownload\\已传\\ss大航海时代2[简][V1.01]\\大航海时代2.mds"


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
    printf("\n");
  }

#undef C
}


void print_cd_state(pByte command, int n) {
  Byte state[13] = {0};
  Byte bit;
  for (int i=0; i<13; ++i) {
    pByte p = &state[i];

    for (int b=7; b>=0; --b) {
      bit = cd_drive_get_serial_bit();
      *p |= (bit << b);
      bit = command[i] & (1 << b);
      cd_drive_set_serial_bit(bit > 0);
    }
  }
  printf("\nCD Drive \nState: ");
  print_buf(state, 13, n);
  printf("Cmd  : ");
  print_buf(command, 13, n);
}


#ifdef _WIN32_WINNT
static Byte cmd[13] = {0};

void cdi_sector_data_ready(pByte buf, int buflen, char is_audio) {
  if (is_audio) {
    printf("Audio sector %d bytes:\n", buflen);
  } else {
    printf("Date sector %d bytes:\n", buflen);
  }
  print_buf(buf, 16, 0);
}

void cdi_update_drive_bit() {
  set_checksum(cmd);
  print_cd_state(cmd, 0);
}
#endif


int main() {
  assert(1 == sizeof(UCHAR));
  assert(2 == sizeof(USHORT));
  assert(4 == sizeof(unsigned));
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

  cdd_reset();
  for (int i=0; i<10; ++i) {
    cd_command_exec();
  }

  return 0;
}


