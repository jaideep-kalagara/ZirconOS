#include <arch/i686/cpu_brand.h>
#include <arch/i686/drivers/keyboard.h>
#include <arch/i686/gdt.h>
#include <arch/i686/idt.h>
#include <arch/i686/io.h>
#include <arch/i686/irq.h>
#include <arch/i686/isr.h>
#include <arch/i686/memory.h>
#include <arch/i686/multiboot.h>
#include <kernel/dev_tty.h>
#include <kernel/kmalloc.h>
#include <kernel/sleep.h>
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PHYS_TO_VIRT(p) ((void *)((uintptr_t)(p) + KERNEL_START))
#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

// Forward decls
static void analyze_cmd(char *cmd);

static void analyze_cmd(char *cmd) {
  if (!cmd)
    return;

  char *command = strtok(cmd, " \t\r\n");
  char *arg = strtok(NULL, " \t\r\n");
  if (!command)
    return;

  printf("\n");
  if (strcmp(command, "color") == 0) {
    if (!arg) {
      printf("usage: color <light_green|red|white>\n");
      return;
    }
    if (strcmp(arg, "light_green") == 0)
      terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    else if (strcmp(arg, "red") == 0)
      terminal_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
    else if (strcmp(arg, "white") == 0)
      terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    else
      printf("Unknown color: %s\n", arg);
  } else if (strcmp(command, "clear") == 0) {
    terminal_clear_screen();
  } else if (strcmp(command, "sleep") == 0) {
    sleep(10000);
  } else if (strcmp(command, "dbg_dump_pm") == 0) {
    dump_physical_memory_bitmap();
  } else if (strcmp(command, "help") == 0) {
    printf("color <light_green|red|white>: set terminal color\n");
    printf("clear: clear the screen\n");
    printf("sleep: sleep for 10 seconds\n");
    printf("dbg_dump_pm: print physical memory bitmap\n");
    printf("kmalloc <size>: allocate <size> bytes of memory\n");
    printf("help: what you are reading\n");
  } else if (strcmp(command, "kmalloc") == 0) {
    // TODO: Implement kmalloc
  } else {
    printf("Unknown command: %s\n", command);
  }
}

void kernel_main(uint32_t magic, void *boot_info_phys) {
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
  uint32_t mem_high_bytes = 0x00100000u; // safe default = 1 MiB
  uint32_t physical_alloc_start;

  // End of kernel (VIRTUAL) from linker script; convert to PHYSICAL then
  // page-align up
  extern char _kernel_end; // provided by your linker
  uint32_t kernel_phys_end = (uint32_t)((uintptr_t)&_kernel_end - KERNEL_START);
  physical_alloc_start = (uint32_t)ALIGN_UP(kernel_phys_end, 0x1000u);

  if (magic == MB2_BOOTLOADER_MAGIC) {
    // Multiboot2: EBX (= boot_info_phys) points to tag list at a PHYSICAL
    // address
    const struct mb2_info_fixed *info =
        (const struct mb2_info_fixed *)PHYS_TO_VIRT(boot_info_phys);
    mem_high_bytes =
        mb2_mem_top_bytes(info); // prefer E820; fallback to basic mem
    printf("MB2: mem_top = 0x%08x bytes\n", mem_high_bytes);
  } else {
    printf("Unknown boot protocol (magic=0x%08x). Assuming 1 MiB only.\n",
           magic);
  }

  // --- Bring up paging/PMM/KHEAP ---
  i686_init_memory(/*mem_high*/ mem_high_bytes,
                   /*physical_alloc_start*/ physical_alloc_start);

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

  // Mini shell
  printf("> ");
  char input_buf[64] = {0};
  for (;;) {
    char ch;
    if (read(STDIN_FILENO, &ch, 1) > 0) {
      if (ch == '\n') {
        analyze_cmd(input_buf);
        memset(input_buf, 0, sizeof input_buf);
        printf("\n> ");
      } else if (ch == '\b') {
        size_t L = strlen(input_buf);
        if (L) {
          input_buf[L - 1] = '\0';
          printf("\b \b");
        }
      } else {
        size_t L = strlen(input_buf);
        if (L + 1 < sizeof input_buf) {
          input_buf[L] = ch;
          input_buf[L + 1] = '\0';
        }
        printf("%c", ch);
      }
    }
  }
}
