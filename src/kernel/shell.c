#include "kernel/shell.h"

#include <arch/i686/cpu_brand.h>   // cpu_get_brand_string (used by "info")
#include <arch/i686/drivers/ide.h> // ide_devices, ide_read_sectors
#include <arch/i686/io.h>          // inb/outb
#include <arch/i686/memory.h>      // dump_physical_memory_bitmap
#include <arch/i686/pmm_stats.h>   // pmm_get_stats
#include <kernel/kmalloc.h>        // kmalloc/kfree
#include <kernel/sleep.h>          // sleep
#include <kernel/tty.h>            // terminal_*()
#include <kernel/vga.h>            // VGA_COLOR_*
#include <stdint.h>                // uint32_t
#include <stdio.h>                 // printf
#include <stdlib.h>                // strtoul
#include <string.h>                // strtok, strcmp, memset, strlen
#include <unistd.h>                // read
#include <util/hex.h>              // hexdump

#ifndef KERNEL_DATA_SEL
#define KERNEL_DATA_SEL 0x10 // typical GDT data selector
#endif

// from IDE driver (used to retrieve last error)
extern unsigned char package[2];

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
    printf("dsk <cmd> <arg>                : disk commands (see dsk help)\n");
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
    printf("inb(0x%X) -> 0x%X\n", port, val);

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
    printf("outb(0x%X, 0x%X) -> OK\n", port, val);

  } else if (strcmp(command, "info") == 0) {
    pmm_stats_t stats = pmm_get_stats();
    printf("Kernel version: 0.0.1\n");
    char brand[64];
    if (cpu_get_brand_string(brand, sizeof brand))
      printf("CPU: %s\n", brand);
    else
      printf("CPU brand string not supported.\n");
    printf("Memory: %u/%u MiB\n", stats.used_bytes_overall / (1024 * 1024),
           stats.total_bytes_usable / (1024 * 1024));

  } else if (strcmp(command, "dsk") == 0) {
    if (!arg) {
      printf("usage: dsk <cmd> <arg>\n");
      return;
    }
    char *arg2 = strtok(NULL, " \t\r\n");

    if (strcmp(arg, "help") == 0) {
      printf("dsk list                         : list disk(s)\n");
      printf("dsk read <drive> <lba> <sectors> : read and hexdump sectors\n");

    } else if (strcmp(arg, "list") == 0) {
      for (int i = 0; i < 4; i++) {
        if (ide_devices[i].reserved) {
          // KiB = sectors * 512 / 1024 = sectors / 2
          printf("Drive %d: Type: %s Size: %u KiB - %s\n", i,
                 (const char *[]){"ATA", "ATAPI"}[ide_devices[i].type],
                 ide_devices[i].size / 2, ide_devices[i].model);
        }
      }

    } else if (strcmp(arg, "read") == 0) {
      // dsk read <drive> <lba> <sectors>
      char *drive_str = arg2;
      char *lba_str = strtok(NULL, " \t\r\n");
      char *sec_str = strtok(NULL, " \t\r\n");
      if (!drive_str || !lba_str || !sec_str) {
        printf("usage: dsk read <drive> <lba> <sectors>\n");
        return;
      }

      unsigned long drive_ul = strtoul(drive_str, NULL, 0);
      unsigned long lba_ul = strtoul(lba_str, NULL, 0);
      unsigned long nsec_ul = strtoul(sec_str, NULL, 0);

      if (drive_ul > 3) {
        printf("read: drive must be 0..3\n");
        return;
      }
      if (!ide_devices[drive_ul].reserved) {
        printf("read: drive %lu not present\n", drive_ul);
        return;
      }
      if (ide_devices[drive_ul].type != 0) {
        printf("read: drive %lu is ATAPI (not ATA PIO here)\n", drive_ul);
        return;
      }
      if (nsec_ul == 0) {
        printf("read: sectors must be >= 1\n");
        return;
      }
      if (lba_ul + nsec_ul > ide_devices[drive_ul].size) {
        printf("read: range exceeds drive size (max LBA %u)\n",
               ide_devices[drive_ul].size - 1);
        return;
      }

      // cap sectors for output sanity
      const unsigned long max_secs_to_read = 32;
      if (nsec_ul > max_secs_to_read) {
        printf("note: capping sectors to %lu for output\n", max_secs_to_read);
        nsec_ul = max_secs_to_read;
      }

      size_t bytes = (size_t)nsec_ul * 512u;
      void *buf = kmalloc((uint32_t)bytes);
      if (!buf) {
        printf("read: OOM\n");
        return;
      }

      package[0] = 0; // clear previous error
      ide_read_sectors((unsigned char)drive_ul, (unsigned char)nsec_ul,
                       (unsigned int)lba_ul, KERNEL_DATA_SEL,
                       (unsigned int)buf);

      if (package[0] != 0) {
        printf("read: IDE error %u\n", package[0]);
        kfree(buf);
        return;
      }

      size_t show = bytes;
      if (show > 4096) {
        printf("showing first 4096 bytes only\n");
        show = 4096;
      }
      // If your hexdump has (data,len,base): use lba*512 as base; otherwise
      // ignore it.
      hexdump(buf, show, (uint32_t)(lba_ul * 512u));

      kfree(buf);
    } else {
      printf("dsk: unknown subcommand: %s\n", arg);
    }

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
