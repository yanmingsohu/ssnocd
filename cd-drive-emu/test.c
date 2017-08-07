#include "usart.h"
#include <ff.h>


void test_print_with_usart() {
  printi(1000);
  printi(10000);
  printi(100000);
  printi(1000000);
  printi(10000000);
  printi(sizeof(double));
  printd(9871234.56711);
  printb(&test_print_with_usart, 64);
}


void test_file_op() {
  FATFS fs;
  if (FR_OK != f_mount(0, &fs)) {
    prints("Mount fs failed.\n");
    return;
  }
  
  FIL file;
  if (FR_OK != f_open(&file, "/test.txt", FA_WRITE | FA_CREATE_NEW)) {
    prints("Open 'test.txt' failed\n");
    return;
  }
}