#pragma once

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

/* ----- Cell I/O (text mode) ----- */

/* Draw single character cell (uses current color) */
void putchr(size_t x, size_t y, char c);
/* Read character from a cell */
char getchr(size_t x, size_t y);
/* Draw raw 16-bit VGA cell (char | attribute) */
void putcell(size_t x, size_t y, uint16_t cell);
/* Read raw 16-bit VGA cell */
uint16_t getcell(size_t x, size_t y);

/* ----- Terminal state ----- */

/* Set terminal color */
void terminal_set_color(enum vga_color fg, enum vga_color bg);
/* Clear terminal screen and reset scrollback view to bottom */
void terminal_clear_screen(void);

/* ----- Output ----- */

/* Print single character */
void _putc(const char c);
/* Print null-terminated string */
void _puts(const char *s);

/* ----- Scrollback controls (user navigation) ----- */
/* Scroll by N lines: +N = up, -N = down */
void terminal_scroll_lines(int delta);
/* Page up/down by one screen */
void terminal_scroll_page_up(void);
void terminal_scroll_page_down(void);
/* Jump to oldest (top) / newest (bottom) content */
void terminal_scroll_home(void);
void terminal_scroll_end(void);
