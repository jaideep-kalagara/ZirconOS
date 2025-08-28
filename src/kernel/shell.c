#include "kernel/shell.h"

#include <arch/i686/cpu_brand.h> // cpu_get_brand_string (used by "info")
#include <arch/i686/io.h>        // inb/outb
#include <arch/i686/memory.h>    // dump_physical_memory_bitmap
#include <kernel/kmalloc.h>      // kmalloc/kfree
#include <kernel/sleep.h>        // sleep
#include <kernel/tty.h>          // terminal_*()
#include <kernel/vga.h>          // VGA_COLOR_*
#include <stdint.h>              // uint32_t
#include <stdio.h>               // printf
#include <string.h>              // strtok, strcmp, memset, strlen
#include <unistd.h>              // read

// If you don't have <stdlib.h>, keep this forward decl for your freestanding
// strtoul:
unsigned long strtoul(const char *nptr, char **endptr, int base);

void analyze_cmd(char *cmd, uint32_t mem_high_bytes) {
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
    printf("color <light_green|red|white>  : set terminal color\n");
    printf("clear                          : clear the screen\n");
    printf("sleep                          : sleep for 10 seconds\n");
    printf("dbg_dump_pm                    : print physical memory bitmap\n");
    printf("kmalloc <size>                 : allocate <size> bytes\n");
    printf("kfree <ptr>                    : free memory at <ptr>\n");
    printf("inb <port>                     : read byte from I/O port (hex/dec "
           "ok)\n");
    printf("outb <port> <value>            : write byte to I/O port\n");
    printf("info                           : print kernel/CPU/memory info\n");
    printf("help                           : this text\n");

  } else if (strcmp(command, "kmalloc") == 0) {
    if (!arg) {
      printf("usage: kmalloc <size>\n");
      return;
    }
    uint32_t n = (uint32_t)strtoul(arg, NULL, 0);
    void *p = kmalloc(n);
    if (!p)
      printf("kmalloc(%u) -> NULL\n", n);
    else
      printf("kmalloc(%u) -> %p\n", n, p);

  } else if (strcmp(command, "kfree") == 0) {
    if (!arg) {
      printf("usage: kfree <ptr>\n");
      return;
    }
    void *p = (void *)strtoul(arg, NULL, 0);
    if (!p)
      printf("kfree(NULL) -> error\n");
    else {
      kfree(p);
      printf("kfree(%p) -> OK\n", p);
    }

  } else if (strcmp(command, "inb") == 0) {
    if (!arg) {
      printf("usage: inb <port>\n");
      return;
    }
    unsigned long port_ul = strtoul(arg, NULL, 0);
    if (port_ul > 0xFFFFul) {
      printf("inb: port out of range (0..0xFFFF)\n");
      return;
    }
    uint16_t port = (uint16_t)port_ul;
    uint8_t val = i686_inb(port);
    printf("inb(0x%04X) -> 0x%02X\n", port, val);

  } else if (strcmp(command, "outb") == 0) {
    char *arg2 = strtok(NULL, " \t\r\n");
    if (!arg || !arg2) {
      printf("usage: outb <port> <value>\n");
      return;
    }
    unsigned long port_ul = strtoul(arg, NULL, 0);
    unsigned long val_ul = strtoul(arg2, NULL, 0);
    if (port_ul > 0xFFFFul) {
      printf("outb: port out of range (0..0xFFFF)\n");
      return;
    }
    if (val_ul > 0xFFul) {
      printf("outb: value out of range (0..0xFF)\n");
      return;
    }
    uint16_t port = (uint16_t)port_ul;
    uint8_t val = (uint8_t)val_ul;
    i686_outb(port, val);
    printf("outb(0x%04X, 0x%02X) -> OK\n", port, val);

  } else if (strcmp(command, "info") == 0) {
    printf("Kernel version: 0.0.1\n");
    char brand[64];
    if (cpu_get_brand_string(brand, sizeof brand))
      printf("CPU: %s\n", brand);
    else
      printf("CPU brand string not supported.\n");
    printf("Memory: %u MiB\n", mem_high_bytes >> 20u);

  } else {
    printf("Unknown command: %s\n", command);
  }
}

void loop(uint32_t mem_high_bytes) {
  printf("> ");
  char input_buf[64] = {0};
  for (;;) {
    char ch;
    if (read(STDIN_FILENO, &ch, 1) > 0) {
      if (ch == '\n') {
        analyze_cmd(input_buf, mem_high_bytes);
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
