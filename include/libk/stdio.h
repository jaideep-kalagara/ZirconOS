#ifndef _LIBK_STDIO_H
#define _LIBK_STDIO_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

void putc(const char c);
void puts(const char *s);
void printf(const char *fmt, ...);

#endif