#include <arch/i686/cpu_brand.h>
#include <arch/i686/gdt.h>
#include <arch/i686/io.h>

#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stdint.h>
#include <stdio.h>

void kernel_main(void) {
  terminal_clear_screen();
  terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

  i686_init_gdt(); // lgdt + reload segments (no sti)

  terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
  printf("----- Zircon OS v0.0.1 (Kernel) -----\n");
  terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  char brand[64];
  if (cpu_get_brand_string(brand, sizeof brand))
    printf("CPU: %s\n", brand);
  else
    printf("CPU brand string not supported.\n");

  for (;;) {
    __asm__ volatile("hlt");
  }
}
