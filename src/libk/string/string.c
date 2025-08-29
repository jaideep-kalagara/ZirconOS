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
