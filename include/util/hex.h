#pragma once

#include <stdint.h>
#include <stdio.h>

static inline void hexdump(const void *data, size_t len, uint32_t base) {
  const uint8_t *p = (const uint8_t *)data;
  for (size_t i = 0; i < len; i += 16) {
    printf("%X  ", base + (uint32_t)i);
    // hex
    for (size_t j = 0; j < 16; ++j) {
      if (i + j < len)
        printf("%X ", p[i + j]);
      else
        printf("   ");
      if (j == 7)
        printf(" ");
    }
    printf(" ");
    // ascii
    for (size_t j = 0; j < 16 && i + j < len; ++j) {
      uint8_t c = p[i + j];
      printf("%c", (c >= 32 && c < 127) ? c : '.');
    }
    printf("\n");
  }
}