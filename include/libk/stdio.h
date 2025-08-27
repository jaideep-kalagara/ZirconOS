#pragma once

#include "libk/unistd.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static inline int out_write(int fd, const char *s, size_t n) {
  long w = write(fd, s, n);
  return (w < 0) ? -1 : (int)w;
}

int vfprintf_fd(int fd, const char *fmt, va_list ap);
int printf(const char *fmt, ...);
int fprintf(int fd, const char *fmt, ...);
int puts(const char *s);
int fputs(const char *s, int fd);
int putchar(int c);