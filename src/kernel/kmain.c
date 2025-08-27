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
#include <kernel/sleep.h>
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void analyze_cmd(char *cmd) {
    char *command = strtok(cmd, " ");
    char *arg = strtok(NULL, " ");
    if (strcmp(command, "color") == 0) {
        terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        printf("\nTerminal color set to light green.\n");
    } else
        printf("Unknown command: %s\n", cmd);
}

void kernel_main(uint32_t magic, struct multiboot_info *boot_info) {
    dev_tty_install_std();
    terminal_clear_screen();  // init screen

    terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf("----- Zircon OS v0.0.1 (Kernel) -----\n");

    i686_init_gdt();  // lgdt + reload segments (no sti)
    i686_init_idt();  // lidt
    i686_init_isr();  // idt
    i686_init_irq();  // idt
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

    printf("> ");
    // shell EXTREMLY SIMPLE
    for (;;) {
        char buf[64];
        char command_buf[64];
        if (read(STDIN_FILENO, buf, sizeof buf) > 0) {
            if (buf[0] == '\n') {
                analyze_cmd(command_buf);
                memset(command_buf, 0, sizeof command_buf);
                printf("\n>");
                continue;
            }
            if (buf[0] == '\b') {
                if (strlen(command_buf) > 0) {
                    command_buf[strlen(command_buf) - 1] = '\0';
                    printf("\b \b");
                }
                continue;
            }
            strcat(command_buf, buf);
            printf("%c", buf[0]);
        }
    }
}
