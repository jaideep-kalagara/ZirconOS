#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stdio.h>

void kernel_main(void) {
  terminal_clear_screen(); // also initializes terminal

  // TODO:initialize gdt

  terminal_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
  printf("----- Zircon OS v0.0.1 -----\n");
  terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

  for (;;)
    __asm__ volatile("hlt");
}