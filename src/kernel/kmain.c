#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stdio.h>

#include <arch/i686/gdt.h>
#include <arch/i686/idt.h>

void kernel_main(void) {
  terminal_clear_screen(); // also initializes terminal
  i686_init_gdt();         // initialize GDT
  i686_init_idt();         // initialize IDT

  terminal_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
  printf("----- Zircon OS v0.0.1 -----\n");
  terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

  for (;;)
    __asm__ volatile("hlt");
}