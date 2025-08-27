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
    if (!cmd) return;

    // First token: command; second token: arg
    char *command = strtok(cmd, " \t\r\n");
    char *arg = strtok(NULL, " \t\r\n");

    if (!command) return;

    printf("\n");
    if (strcmp(command, "color") == 0) {
        if (!arg) {
            printf("usage: color <light_green|red>\n");
            return;
        }

        if (strcmp(arg, "light_green") == 0) {
            terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        } else if (strcmp(arg, "red") == 0) {
            terminal_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
        } else if (strcmp(arg, "white") == 0) {
            terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        } else {
            printf("Unknown color: %s\n", arg);
        }
    } else if (strcmp(command, "clear") == 0) {
        terminal_clear_screen();
    } else if (strcmp(command, "sleep") == 0) {
        sleep(10000);
    } else if (strcmp(command, "help") == 0) {
        printf("color <light_green|red|white>: set terminal color\n");
        printf("clear: clear the screen\n");
        printf("sleep: sleep for 10 seconds\n");
        printf("help: what you are reading\n");
    } else {
        // Print the command token
        printf("Unknown command: %s\n", command);
    }
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

    printf("\nWelcome to Zircon OS!\n");
    printf("Type \"help\" for help.\n");
    printf("------------------------------------\n");

    printf("> ");
    // shell EXTREMLY SIMPLE
    for (;;) {
        char buf[64];
        char command_buf[64];
        if (read(STDIN_FILENO, buf, sizeof buf) > 0) {
            if (buf[0] == '\n') {
                analyze_cmd(command_buf);
                memset(command_buf, 0, sizeof command_buf);
                printf("\n> ");
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
