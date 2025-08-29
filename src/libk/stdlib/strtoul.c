#include "libk/stdio.h"
#include "libk/stdlib.h"
#include <limits.h>

static int is_space(int c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
         c == '\f';
}

static int digit_val(int c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'z')
    return 10 + (c - 'a');
  if (c >= 'A' && c <= 'Z')
    return 10 + (c - 'A');
  return -1;
}

unsigned long strtoul(const char *nptr, char **endptr, int base) {
  const char *s = nptr;
  unsigned long acc = 0;
  int neg = 0, any = 0, d;

  // Skip leading whitespace
  while (is_space((unsigned char)*s))
    s++;

  // Optional sign
  if (*s == '+' || *s == '-') {
    neg = (*s == '-');
    s++;
  }

  // Base resolution
  if (base == 0) {
    if (s[0] == '0') {
      if (s[1] == 'x' || s[1] == 'X') {
        base = 16;
        s += 2;
      } else
        base = 8;
    } else {
      base = 10;
    }
  } else if (base == 16) {
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
      s += 2;
  }
  if (base < 2 || base > 36) { // invalid base
    if (endptr)
      *endptr = (char *)nptr;
    return 0;
  }

  // Overflow guard thresholds
  unsigned long cutoff = ULONG_MAX / (unsigned long)base;
  unsigned long cutlim = ULONG_MAX % (unsigned long)base;
  int overflow = 0;

  // Accumulate
  for (; (d = digit_val((unsigned char)*s)) >= 0; s++) {
    if (d >= base)
      break;
    if (acc > cutoff || (acc == cutoff && (unsigned long)d > cutlim)) {
      overflow = 1;
    } else {
      acc = acc * (unsigned long)base + (unsigned long)d;
      any = 1;
    }
  }

  // No digits consumed?
  if (!any) {
    if (endptr)
      *endptr = (char *)nptr;
    return 0;
  }

  // Apply sign (C standard: result is negated modulo ULONG_MAX+1)
  if (neg)
    acc = (unsigned long)(-(long)acc);

  // 8) Handle overflow
  if (overflow) {
    acc = ULONG_MAX;
    fprintf(STDERR_FILENO, "strtoul: value too large\n");
  }

  if (endptr)
    *endptr = (char *)s;
  return acc;
}