#include <arch/i686/io.h>
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum { SCREEN_WIDTH = 80, SCREEN_HEIGHT = 25 };

static volatile uint16_t *const screen_buffer = (uint16_t *)0xB8000;
static uint8_t terminal_color;
static size_t screen_x = 0, screen_y = 0;

/* ----- Cursor ----- */

void set_cursor(size_t x, size_t y) {
  size_t pos = y * SCREEN_WIDTH + x;
  i686_outb(0x3D4, 0x0F);
  i686_outb(0x3D5, (uint8_t)(pos & 0xFF));
  i686_outb(0x3D4, 0x0E);
  i686_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}
void enable_cursor(uint8_t start, uint8_t end) {
  i686_outb(0x3D4, 0x0A);
  i686_outb(0x3D5, (i686_inb(0x3D5) & 0xC0) | start);
  i686_outb(0x3D4, 0x0B);
  i686_outb(0x3D5, (i686_inb(0x3D5) & 0xE0) | end);
}
void disable_cursor(void) {
  i686_outb(0x3D4, 0x0A);
  i686_outb(0x3D5, 0x20);
}

/* Draw/read single cell */

void putchr(size_t x, size_t y, char c) {
  screen_buffer[y * SCREEN_WIDTH + x] =
      vga_entry((unsigned char)c, terminal_color);
}
char getchr(size_t x, size_t y) {
  return (char)(screen_buffer[y * SCREEN_WIDTH + x] & 0xFF);
}
void putcell(size_t x, size_t y, uint16_t cell) {
  screen_buffer[y * SCREEN_WIDTH + x] = cell;
}
uint16_t getcell(size_t x, size_t y) {
  return screen_buffer[y * SCREEN_WIDTH + x];
}

/* ----- Scrolling ----- */

void scrollback(int lines) {
  if (lines <= 0)
    return;
  if (lines > SCREEN_HEIGHT)
    lines = SCREEN_HEIGHT;

  size_t rows_to_move = SCREEN_HEIGHT - (size_t)lines;
  size_t cells_to_move = rows_to_move * SCREEN_WIDTH;

  /* Move whole 16-bit cells to preserve attributes */
  memmove((void *)screen_buffer,
          (const void *)(screen_buffer + lines * SCREEN_WIDTH),
          cells_to_move * sizeof(uint16_t));

  /* Clear the bottom 'lines' rows */
  uint16_t blank = vga_entry(' ', terminal_color);
  for (size_t i = cells_to_move; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
    screen_buffer[i] = blank;
  }

  if (screen_y >= (size_t)lines)
    screen_y -= (size_t)lines;
  else
    screen_y = 0;
}

void terminal_set_color(enum vga_color fg, enum vga_color bg) {
  terminal_color = vga_entry_color(fg, bg);
}

void terminal_clear_screen(void) {
  terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  enable_cursor(0, 15);
  uint16_t blank = vga_entry(' ', terminal_color);
  for (size_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
    screen_buffer[i] = blank;
  }
  screen_x = 0;
  screen_y = 0;
  set_cursor(screen_x, screen_y);
}

void _putc(const char c) {
  switch (c) {
  case '\n':
    screen_x = 0;
    screen_y++;
    break;
  case '\r':
    screen_x = 0;
    break;
  case '\t': {
    int spaces = 4 - (int)(screen_x % 4);
    while (spaces-- > 0)
      _putc(' ');
    break;
  }
  default:
    if (screen_x >= SCREEN_WIDTH) {
      screen_x = 0;
      screen_y++;
    }
    if (screen_y >= SCREEN_HEIGHT)
      scrollback(1);
    putchr(screen_x, screen_y, c);
    screen_x++;
    break;
  }

  if (screen_x >= SCREEN_WIDTH) {
    screen_x = 0;
    screen_y++;
  }
  if (screen_y >= SCREEN_HEIGHT)
    scrollback(1);
  set_cursor(screen_x, screen_y);
}

void _puts(const char *s) {
  while (*s)
    _putc(*s++);
}