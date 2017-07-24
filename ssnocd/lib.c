#include "lib.h"


int strcpy(char* desc,char* src, int max) {
  int i = 0;
  max -= 1;
  while (i < max && src[i]) {
    desc[i] = src[i];
    ++i;
  }
  desc[i+1] = 0;
  return i;
}