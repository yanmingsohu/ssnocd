#include "fs.h"
#include "lib.h"


static FileState fstate = Closed;
static FILE * fd = 0;


FileState fs_state() {
  return fstate;
}


void fs_open(pByte filename) {
  assert(fstate != Opened);

#ifdef _ARDUINO
#endif

#ifdef _WIN32_WINNT
  FILE * f = fopen(filename, "rb");
  if (!f) {
    fstate = OpenFailed;
    fd = 0;
  } else {
    fstate = Opened;
    fd = f;
  }
#endif
}


void fs_close() {
  assert(fstate == Opened);
#ifdef _ARDUINO
#endif

#ifdef _WIN32_WINNT
  if (fclose(fd) == 0) {
    fstate = Closed;
    fd = 0;
  } else {
    fstate = CloseFailed;
  }
#endif
}


int fs_seek(ULONGLONG offset) {
  assert(fstate == Opened);

#ifdef _ARDUINO
#endif

#ifdef _WIN32_WINNT
  return fseek(fd, offset, SEEK_SET);
#endif
}


int fs_read(pByte buf, int len) {
  assert(fstate == Opened);

#ifdef _ARDUINO
#endif

#ifdef _WIN32_WINNT
  return fread(buf, 1, len, fd);
#endif
}


int fs_read_string(pByte buf, int max) {
  assert(fstate == Opened);
  char c;
  int rlen = 0;
  --max;

#ifdef _ARDUINO
#endif

#ifdef _WIN32_WINNT`
  while (rlen < max) {
    c = fgetc(fd);
    if (c > 0) {
      buf[rlen] = c;
      ++rlen;
    } else {
      break;
    }
  }
#endif

  buf[rlen] = 0;
  return rlen;
}


int fs_size() {
  assert(fstate == Opened);

#ifdef _ARDUINO
#endif

#ifdef _WIN32_WINNT
  fseek(fd, 0, SEEK_END);
  ULONGLONG pos = 0;
  fgetpos(fd, &pos);
  return pos;
#endif
}