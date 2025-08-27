#include "libk/stdio.h"
#include "libk/unistd.h"
#include <stdarg.h>

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vfprintf_fd(STDOUT_FILENO, fmt, ap);
  va_end(ap);
  return r;
}