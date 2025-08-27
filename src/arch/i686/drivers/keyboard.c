#include "arch/i686/drivers/keyboard.h"
#include <arch/i686/io.h>
#include <arch/i686/irq.h>
#include <stdio.h>

void keyboard_init() { i686_irq_register_handler(1, keyboard_handler); }

void keyboard_handler(registers *regs) {

  uint8_t scancode = i686_inb(0x60);
  i686_iowait();
  printf("scancode: %x\n", scancode);
}