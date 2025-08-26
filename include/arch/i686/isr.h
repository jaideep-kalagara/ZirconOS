#pragma once

#include <stdint.h>

typedef struct {
  uint32_t ds;                                         // data segment
  uint32_t edi, esi, ebp, useless, ebx, edx, ecx, eax; // pusha
  uint32_t interrupt, error;
  uint32_t eip, cs, eflags, esp, ss; // pushed automatically by CPU
} __attribute__((packed)) registers;

typedef void (*isr_handler_t)(registers *regs);

void i686_init_isr();
void i686_isr_register_handler(int interrupt, isr_handler_t handler);