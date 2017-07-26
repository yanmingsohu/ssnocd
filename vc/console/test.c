#include "lib.h"


static Byte read_toc[] =
  { 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 };

static Byte read_fad[] = 
  { 0x06, 0x00, 0x00, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 };

static Byte pause[] =
  { 0x08, 0x00, 0x00, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 };


void test(pByte cmd) {
  int index = 0;
  printf("ID mean:"
  "\n - (03 : read toc)"
  "\n - (06 : read fad)"
  "\n - (08 : pause)"
  "\n");

  printf("Test ID(hex): ");
  flushall();
  scanf("%x", &index);

#define ADD_TEST(id, name) \
    case id: \
      memcpy(cmd, name, 11); \
      break;

  switch (index) {
    ADD_TEST(0x03, read_toc);
    ADD_TEST(0x06, read_fad);
    ADD_TEST(0x08, pause);

    default:
      printf("\nInvaild Test ID\n");
      return;
  }
  printf("OK\n");
}