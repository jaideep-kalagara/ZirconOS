#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <kernel/vga.h>
#include <stddef.h>
#include <stdint.h>

/* ----- Cursor ----- */

/* Set cursor position */
void set_cursor(size_t x, size_t y);
/* Enable cursor */
void enable_cursor(uint8_t start, uint8_t end);
/* Disable cursor */
void disable_cursor(void);

/* Draw single cell */
void putchr(size_t x, size_t y, char c);
/* Read single cell */
char getchr(size_t x, size_t y);
/* Draw single 16-bit cell */
void putcell(size_t x, size_t y, uint16_t cell);
/* Read single 16-bit cell */
uint16_t getcell(size_t x, size_t y);

/* Scroll back 'lines' lines */
void scrollback(int lines);

/* ----- Terminal ----- */

/* Set terminal color */
void terminal_set_color(enum vga_color fg, enum vga_color bg);
/* Clear terminal screen */
void terminal_clear_screen(void);

/* ----- Public API ----- */

/* Print character */
void _putc(const char c);
/* Print string */
void _puts(const char *s);

#endif // _KERNEL_TTY_H