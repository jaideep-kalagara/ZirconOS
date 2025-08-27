#include "libk/unistd.h"

int open(const char *path, int flags) {
  (void)path;
  (void)flags;
  // TODO: implement after adding VFS
  return -1;
}