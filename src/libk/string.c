#include <string.h>

void memmove(void *dest, const void *src, size_t n) {
  for (size_t i = 0; i < n; i++)
    ((char *)dest)[i] = ((char *)src)[i];
}

void memset(void *s, int c, size_t n) {
  for (size_t i = 0; i < n; i++)
    ((char *)s)[i] = c;
}

void memcpy(void *dest, const void *src, size_t n) {
  for (size_t i = 0; i < n; i++)
    ((char *)dest)[i] = ((char *)src)[i];
}

int memcmp(const void *s1, const void *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (((char *)s1)[i] != ((char *)s2)[i])
      return ((char *)s1)[i] - ((char *)s2)[i];
  }
  return 0;
}

size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len])
    len++;
  return len;
}