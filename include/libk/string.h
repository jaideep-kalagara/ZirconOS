#ifndef _LIBK_STRING_H
#define _LIBK_STRING_H

#include <stddef.h>

// memory functions

void memmove(void *dest, const void *src, size_t n);
void memset(void *s, int c, size_t n);
void memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

// string functions

size_t strlen(const char *s);
#endif // _LIBK_STRING_H