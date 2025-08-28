#include "kernel/kinit.h"

#include <arch/i686/cpu_brand.h>        // cpu_get_brand_string
#include <arch/i686/drivers/keyboard.h> // keyboard_init
#include <arch/i686/gdt.h>              // i686_init_gdt
#include <arch/i686/idt.h>              // i686_init_idt
#include <arch/i686/irq.h>              // i686_init_irq
#include <arch/i686/isr.h>              // i686_init_isr
#include <arch/i686/memory.h>           // KERNEL_START, i686_init_memory
#include <arch/i686/multiboot.h> // MB2_BOOTLOADER_MAGIC, mb2_info_fixed, mb2_mem_top_bytes
#include <kernel/dev_tty.h> // dev_tty_install_std
#include <kernel/kmalloc.h> // kmalloc_init
#include <kernel/sleep.h>   // init_sleep
#include <kernel/tty.h>     // terminal_*()
#include <kernel/vga.h>     // VGA_COLOR_*
#include <stdint.h>         // uint32_t, uintptr_t
#include <stdio.h>          // printf

#define PHYS_TO_VIRT(p) ((void *)((uintptr_t)(p) + KERNEL_START))
#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

void init(uint32_t magic, void *boot_info_phys, uint32_t *mem_high_bytes,
          uint32_t *physical_alloc_start) {
  dev_tty_install_std();
  terminal_clear_screen();
  terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
  printf("----- Zircon OS v0.0.1 (Kernel) -----\n");

  // Early CPU/IDT/IRQ init
  i686_init_gdt();
  i686_init_idt();
  i686_init_isr();
  i686_init_irq();
  keyboard_init();
  init_sleep();
  printf("Early CPU/IDT/IRQ init done.\n");

  // --- Work out top-of-RAM (bytes) and first free physical byte ---
  *mem_high_bytes = 0x00100000u; // safe default = 1 MiB

  // End of kernel (VIRTUAL) from linker script; convert to PHYSICAL then
  // page-align up
  extern char _kernel_end; // provided by your linker
  uint32_t kernel_phys_end = (uint32_t)((uintptr_t)&_kernel_end - KERNEL_START);
  *physical_alloc_start = (uint32_t)ALIGN_UP(kernel_phys_end, 0x1000u);

  if (magic == MB2_BOOTLOADER_MAGIC) {
    // Multiboot2: EBX (= boot_info_phys) is PHYSICAL; convert to VIRTUAL
    const struct mb2_info_fixed *info =
        (const struct mb2_info_fixed *)PHYS_TO_VIRT(boot_info_phys);
    *mem_high_bytes =
        mb2_mem_top_bytes(info); // prefer E820; fallback to basic mem
    printf("MB2: mem_top = 0x%08X bytes\n", *mem_high_bytes); // <-- deref fix
  } else {
    printf("Unknown boot protocol (magic=0x%08X). Assuming 1 MiB only.\n",
           magic);
  }

  // --- Bring up paging/PMM/KHEAP ---
  i686_init_memory(/*mem_high*/ *mem_high_bytes,
                   /*physical_alloc_start*/ *physical_alloc_start);

  kmalloc_init(16 * 1024);

  // CPU brand
  char brand[64];
  if (cpu_get_brand_string(brand, sizeof brand))
    printf("CPU: %s\n", brand);
  else
    printf("CPU brand string not supported.\n");

  printf("\nWelcome to Zircon OS!\n");
  printf("Type \"help\" for help.\n");
  printf("------------------------------------\n");
}
