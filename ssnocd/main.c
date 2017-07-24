#include "lib.h"
#include "mds.h"
#include "fs.h"
#include "cdstate.h"


int main() {
  assert(1 == sizeof(UCHAR));
  assert(2 == sizeof(USHORT));
  assert(4 == sizeof(unsigned));
  assert(8 == sizeof(ULONGLONG));
  assert(sizeof(MDSTrackBlock) == 80);
  assert(sizeof(MDSSessionBlock) == 24);
  assert(sizeof(MDSHeader) == 92);

  openMds("D:\\Saturn-cd-emu\\�󺽺�ʱ��2[��][V1.01]\\�󺽺�ʱ��2.mds");
  return 0;
}

