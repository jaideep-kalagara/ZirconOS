#include "libk/string.h"
#include "libk/stdio.h"

#include <limits.h>
#include <stddef.h>

size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len])
    len++;
  return len;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

char *strcat(char *dest, const char *src) {
  while (*dest)
    dest++;
  while (*src) {
    *dest = *src;
    dest++;
    src++;
  }
  *dest = '\0';
  return dest;
}

char *strchr(const char *str, int ch) {
  while (*str && *str != ch)
    str++;
  return *str == ch ? (char *)str : NULL;
}

static char *s_strtok_save;

static int in_set(char c, const char *set) {
  while (*set)
    if (c == *set++)
      return 1;
  return 0;
}

char *strtok(char *str, const char *delim) {
  char *start, *end;

  if (str)
    s_strtok_save = str; // new string
  if (!s_strtok_save)
    return NULL; // nothing to scan

  // skip leading delimiters
  start = s_strtok_save;
  while (*start && in_set(*start, delim))
    start++;

  if (*start == '\0') { // only delimiters / end
    s_strtok_save = NULL;
    return NULL;
  }

  // find token end
  end = start;
  while (*end && !in_set(*end, delim))
    end++;

  if (*end) {                // hit a delimiter
    *end = '\0';             // terminate token
    s_strtok_save = end + 1; // advance past it
  } else {                   // hit end of string
    s_strtok_save = NULL;
  }

  return start;
}

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

  // 1) Skip leading whitespace
  while (is_space((unsigned char)*s))
    s++;

  // 2) Optional sign
  if (*s == '+' || *s == '-') {
    neg = (*s == '-');
    s++;
  }

  // 3) Base resolution
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

  // 4) Overflow guard thresholds
  unsigned long cutoff = ULONG_MAX / (unsigned long)base;
  unsigned long cutlim = ULONG_MAX % (unsigned long)base;
  int overflow = 0;

  // 5) Accumulate
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

  // 6) No digits consumed?
  if (!any) {
    if (endptr)
      *endptr = (char *)nptr;
    return 0;
  }

  // 7) Apply sign (C standard: result is negated modulo ULONG_MAX+1)
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