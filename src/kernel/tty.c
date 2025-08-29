#include <arch/i686/io.h>
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum { SCREEN_WIDTH = 80, SCREEN_HEIGHT = 25 };

// ----- Physical VGA memory (mapped at 0xC00B8000) -----
static volatile uint16_t *const screen_buffer = (uint16_t *)0xC00B8000;

// ----- Colors / cursor & logical caret position -----
static uint8_t terminal_color;
static size_t screen_x = 0,
              screen_y = 0; // caret (column,row) in the *viewport* end

// ===== Scrollback state =====
// You can tune this depth. 1024 lines ≈ 1024*80*2
#define SCROLLBACK_LINES 1024

static uint16_t
    scrollbuf[SCROLLBACK_LINES * SCREEN_WIDTH]; // ring buffer of lines
static size_t tail_line = 0; // monotonic logical index of the *current* line
static size_t sb_count =
    0; // number of valid lines stored (<= SCROLLBACK_LINES)
static size_t view_offset = 0; // 0=bottom; N lines above bottom when scrolled

/* ---------- Low-level cursor ---------- */
static void hw_set_cursor_pos(size_t pos) {
  i686_outb(0x3D4, 0x0F);
  i686_outb(0x3D5, (uint8_t)(pos & 0xFF));
  i686_outb(0x3D4, 0x0E);
  i686_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}
void set_cursor(size_t x, size_t y) { hw_set_cursor_pos(y * SCREEN_WIDTH + x); }

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

/* ---------- Scrollback helpers ---------- */
static inline uint16_t *sb_line_ptr(size_t line_number) {
  // map monotonic line number to circular buffer row
  return &scrollbuf[(line_number % SCROLLBACK_LINES) * SCREEN_WIDTH];
}

static void sb_clear_line(size_t line_number) {
  uint16_t blank = vga_entry(' ', terminal_color);
  uint16_t *row = sb_line_ptr(line_number);
  for (size_t x = 0; x < SCREEN_WIDTH; ++x)
    row[x] = blank;
}

/* Compute how many lines we can scroll up at most */
static size_t max_view_offset(void) {
  if (sb_count <= SCREEN_HEIGHT)
    return 0;
  return sb_count - SCREEN_HEIGHT;
}

/* Redraw current viewport from scrollback into VGA memory */
static void blit_view(void) {
  // End line shown: tail_line - view_offset
  // Start line shown: end - (SCREEN_HEIGHT-1)
  size_t end_line =
      (view_offset <= (size_t)INT32_MAX) ? (tail_line - view_offset) : 0;
  size_t start_line =
      (end_line >= (SCREEN_HEIGHT - 1)) ? (end_line - (SCREEN_HEIGHT - 1)) : 0;

  // Oldest stored line number (monotonic)
  size_t oldest = (sb_count > 0) ? (tail_line + 1 - sb_count) : 0;

  // For each row of the screen, copy from scrollback if available; otherwise
  // blanks
  uint16_t blank = vga_entry(' ', terminal_color);
  for (size_t row = 0; row < SCREEN_HEIGHT; ++row) {
    size_t src_line = start_line + row;
    volatile uint16_t *dst = &screen_buffer[row * SCREEN_WIDTH];

    if (sb_count == 0 || src_line < oldest || src_line > tail_line) {
      for (size_t x = 0; x < SCREEN_WIDTH; ++x)
        dst[x] = blank;
    } else {
      uint16_t *src = sb_line_ptr(src_line);
      // copy whole line
      for (size_t x = 0; x < SCREEN_WIDTH; ++x)
        dst[x] = src[x];
    }
  }

  // Cursor: show only when following the bottom
  if (view_offset == 0) {
    // caret is on the end line (tail_line)
    size_t cursor_row =
        (tail_line >= start_line) ? (tail_line - start_line) : 0;
    if (cursor_row >= SCREEN_HEIGHT)
      cursor_row = SCREEN_HEIGHT - 1;
    set_cursor(screen_x, cursor_row);
    enable_cursor(0, 15);
  } else {
    disable_cursor();
  }
}

/* Jump back to bottom if user had scrolled up and new output comes */
static void follow_bottom_if_scrolled(void) {
  if (view_offset != 0) {
    view_offset = 0;
    blit_view();
  }
}

/* Advance N logical lines (like printing N newlines) */
static void advance_lines(size_t lines) {
  while (lines--) {
    tail_line++;
    if (sb_count < SCROLLBACK_LINES)
      sb_count++;
    // clear the new tail line before writing into it
    sb_clear_line(tail_line);
  }
}

/* Current logical line number corresponding to a viewport y (when
 * view_offset==0) */
static size_t line_for_view_y(size_t y) {
  // bottom row (y=SCREEN_HEIGHT-1) == tail_line
  // so: line = tail_line - (SCREEN_HEIGHT-1 - y)
  if (SCREEN_HEIGHT - 1 >= y) {
    return tail_line - ((SCREEN_HEIGHT - 1) - y);
  } else {
    return tail_line; // shouldn't happen
  }
}

/* ---------- Draw/read single cell (mirrored into scrollback) ---------- */
void putcell(size_t x, size_t y, uint16_t cell) {
  // write to scrollback line that backs this screen row
  size_t line = line_for_view_y(y);
  if (sb_count == 0) {
    sb_count = 1;
    sb_clear_line(tail_line);
  }
  uint16_t *row = sb_line_ptr(line);
  row[x] = cell;

  // write to visible VGA memory too
  screen_buffer[y * SCREEN_WIDTH + x] = cell;
}
uint16_t getcell(size_t x, size_t y) {
  return screen_buffer[y * SCREEN_WIDTH + x];
}

void putchr(size_t x, size_t y, char c) {
  i686_outb(0xe9, c);
  putcell(x, y, vga_entry((unsigned char)c, terminal_color));
}
char getchr(size_t x, size_t y) { return (char)(getcell(x, y) & 0xFF); }

/* ---------- Public color/clear ---------- */
void terminal_set_color(enum vga_color fg, enum vga_color bg) {
  terminal_color = vga_entry_color(fg, bg);
}

void terminal_clear_screen(void) {
  // reset scrollback
  tail_line = 0;
  sb_count = 1;
  view_offset = 0;
  sb_clear_line(0);

  // fill visible VRAM
  uint16_t blank = vga_entry(' ', terminal_color);
  for (size_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
    screen_buffer[i] = blank;
  }
  screen_x = 0;
  screen_y = 0;
  enable_cursor(0, 15);
  set_cursor(screen_x, screen_y);
}

/* ---------- Output ---------- */
void _putc(const char c) {
  // If user scrolled up, snap back to bottom on new output
  follow_bottom_if_scrolled();

  switch (c) {
  case '\n':
    i686_outb(0xe9, '\n');
    screen_x = 0;
    // move caret to next logical line
    if (screen_y < SCREEN_HEIGHT - 1) {
      // move caret within viewport; also ensure backing line exists
      screen_y++;
      // make sure the line exists in scrollback view: it does because tail_line
      // is bottom
    } else {
      // we need to scroll forward by 1 line (produce a new line at the bottom)
      advance_lines(1);
      blit_view();
    }
    break;

  case '\r':
    i686_outb(0xe9, '\r');
    screen_x = 0;
    break;

  case '\t': {
    i686_outb(0xe9, '\t');
    int spaces = 4 - (int)(screen_x % 4);
    while (spaces-- > 0)
      _putc(' ');
    break;
  }

  case '\b':
    i686_outb(0xe9, '\b');
    if (screen_x > 0) {
      screen_x--;
      putchr(screen_x, screen_y, ' ');
    } else if (screen_y > 0) {
      screen_y--;
      screen_x = SCREEN_WIDTH - 1;
      putchr(screen_x, screen_y, ' ');
    }
    set_cursor(screen_x, screen_y);
    break;

  default:
    if (screen_x >= SCREEN_WIDTH) {
      screen_x = 0;
      // move to next line
      if (screen_y < SCREEN_HEIGHT - 1) {
        screen_y++;
      } else {
        advance_lines(1);
        blit_view();
      }
    }
    // Ensure the backing line exists (it does by construction)
    putchr(screen_x, screen_y, c);
    screen_x++;
    break;
  }

  // post-fixup: wrap/scroll if needed
  if (screen_x >= SCREEN_WIDTH) {
    screen_x = 0;
    if (screen_y < SCREEN_HEIGHT - 1) {
      screen_y++;
    } else {
      advance_lines(1);
      blit_view();
    }
  }
  set_cursor(screen_x, screen_y);
}

void _puts(const char *s) {
  while (*s)
    _putc(*s++);
}

/* ---------- User scroll control (call from your keyboard handler) ----------
 */
static void scroll_apply_delta(int delta) {
  if (delta == 0)
    return;
  int new_off = (int)view_offset + delta;

  int max_off = (int)max_view_offset();
  if (new_off < 0)
    new_off = 0;
  if (new_off > max_off)
    new_off = max_off;

  if ((size_t)new_off != view_offset) {
    view_offset = (size_t)new_off;
    blit_view();
  }
}

// Scroll by lines (positive = up, negative = down)
void terminal_scroll_lines(int delta) { scroll_apply_delta(delta); }

// Page up/down by a screenful
void terminal_scroll_page_up(void) { scroll_apply_delta((int)SCREEN_HEIGHT); }
void terminal_scroll_page_down(void) {
  scroll_apply_delta(-(int)SCREEN_HEIGHT);
}

// Jump to very top/bottom
void terminal_scroll_home(void) {
  view_offset = max_view_offset();
  blit_view();
}
void terminal_scroll_end(void) {
  view_offset = 0;
  blit_view();
}
