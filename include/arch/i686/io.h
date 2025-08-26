#pragma once

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

static inline void i686_interrupts_disable(void) {
  __asm__ volatile("cli" ::: "memory");
}

static inline void i686_interrupts_enable(void) {
  __asm__ volatile("sti" ::: "memory");
}

static inline void i686_panic(void) {
  i686_interrupts_disable();
  for (;;) {
    __asm__ volatile("hlt");
  }
}

static inline void i686_iowait(void) { i686_outb(0x80, 0); }