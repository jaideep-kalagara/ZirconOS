#include <arch/i686/cpu_brand.h>
#include <arch/i686/drivers/keyboard.h>
#include <arch/i686/gdt.h>
#include <arch/i686/idt.h>
#include <arch/i686/io.h>
#include <arch/i686/irq.h>
#include <arch/i686/isr.h>
#include <arch/i686/mm/memory.h>
#include <arch/i686/multiboot.h>

#include <kernel/dev_tty.h>
#include <kernel/sleep.h>
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stdint.h>
#include <stdio.h>

void kernel_main(uint32_t magic, struct multiboot_info *boot_info) {
  dev_tty_install_std();
  terminal_clear_screen(); // init screen

  terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
  printf("----- Zircon OS v0.0.1 (Kernel) -----\n");

  i686_init_gdt(); // lgdt + reload segments (no sti)
  i686_init_idt(); // lidt
  i686_init_isr(); // idt
  i686_init_irq(); // idt
  keyboard_init();
  init_sleep();

  uint32_t mod1 = *(uint32_t *)(boot_info->mods_addr + 4);
  uint32_t physical_alloc_start = (mod1 + 0xFFF) & ~0xFFF;
  i686_init_memory(boot_info->mem_upper * 1024, physical_alloc_start);
  char brand[64];
  if (cpu_get_brand_string(brand, sizeof brand))
    printf("CPU: %s\n", brand);
  else
    printf("CPU brand string not supported.\n");
  terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

  for (;;) {
    __asm__ volatile("hlt");
  }
}
