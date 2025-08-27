#include "libk/stdio.h"
#include <stdarg.h>

int fprintf(int fd, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vfprintf_fd(fd, fmt, ap);
  va_end(ap);
  return r;
}