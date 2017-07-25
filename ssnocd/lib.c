#include "lib.h"


int x_strcpy(char* desc,char* src, int max) {
  int i = 0;
  max -= 1;
  while (i < max && src[i]) {
    desc[i] = src[i];
    ++i;
  }
  desc[i+1] = 0;
  return i;
}


int x_memcpy(void* desc, void* src, int size) {
  for (int i=0; i<size; ++i) {
    ((Byte*)desc)[i] = ((Byte*)src)[i];
  }
  return size;
}