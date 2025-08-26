#ifndef _I686_IO_H
#define _I686_IO_H

#include <stdint.h>

static inline void i686_outb(uint16_t port, uint8_t value) {
  __asm__ volatile("outb %0, %1" ::"a"(value), "Nd"(port));
}

/* Read an 8-bit value from an I/O port */
static inline uint8_t i686_inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

#endif // _I686_IO_H