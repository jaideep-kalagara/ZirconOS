#include "arch/i686/cpu_brand.h"
#include <cpuid.h>

static inline void append_reg(char **p, char *end, unsigned int v) {
  // CPUID returns chars little-endian in eax/ebx/ecx/edx
  for (int i = 0; i < 4 && *p <= end; i++) {
    *(*p)++ = (char)((v >> (i * 8)) & 0xFF);
  }
}

bool cpu_get_vendor(char vendor[13]) {
  unsigned int eax, ebx, ecx, edx;
  if (!__get_cpuid(0, &eax, &ebx, &ecx, &edx)) {
    vendor[0] = 0;
    return false;
  }
  char *p = vendor, *end = vendor + 12 - 1;
  append_reg(&p, end, ebx); // "Genu"
  append_reg(&p, end, edx); // "ineI"
  append_reg(&p, end, ecx); // "ntel" (or "AuthenticAMD")
  *p = '\0';
  return true;
}

bool cpu_get_brand_string(char *buf, size_t len) {
  if (!buf || len == 0)
    return false;
  buf[0] = 0;

  unsigned int max = __get_cpuid_max(0x80000000u, NULL);
  if (max < 0x80000004u)
    return false; // brand string unsupported

  char *p = buf;
  char *end = buf + len - 1;

  for (unsigned int leaf = 0x80000002u; leaf <= 0x80000004u; ++leaf) {
    unsigned int eax, ebx, ecx, edx;
    if (!__get_cpuid(leaf, &eax, &ebx, &ecx, &edx))
      break;
    append_reg(&p, end, eax);
    append_reg(&p, end, ebx);
    append_reg(&p, end, ecx);
    append_reg(&p, end, edx);
  }

  if (p > end)
    p = end;
  *p = '\0';

  // Trim leading/trailing spaces
  char *s = buf;
  while (*s == ' ')
    s++;
  char *q = s;
  while (*q)
    q++;
  while (q > s && q[-1] == ' ')
    --q;
  *q = '\0';
  if (s != buf) { // left-trim: memmove manually
    char *d = buf;
    while ((*d++ = *s++)) { /* copy including NUL */
    }
  }
  return buf[0] != 0;
}
