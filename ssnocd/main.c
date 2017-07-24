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
#define B(a,b) b(buf[i+a]), b(buf[i+a+1]), b(buf[i+a+2]), b(buf[i+a+3])
#define C(a)   (a>0x1F && a<0x80?a:'.')
#define X(a)   (a)

  for (int i=0; i<len; i+=16) {
    printf("%08x  %02x %02x %02x %02x %02x %02x %02x %02x   "
           "%02x %02x %02x %02x %02x %02x %02x %02x   ", 
           beingpos + i, B(0,X),B(4,X),B(8,X),B(12,X) );
    printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", 
           B(0,C),B(4,C),B(8,C),B(12,C));          
  }

#undef B
#undef C
#undef X
}


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
  int sector = 0;
  int rlen = cd_read_sector(sector, buf, MAX_SECTOR_SIZE);
  DEBUG("\nSector %d len: %d bytes\n", sector, rlen);
  print_buf(buf, rlen, 0);

  return 0;
}


