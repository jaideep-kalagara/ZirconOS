#include "arch/i686/drivers/keyboard.h"

#include <arch/i686/io.h>
#include <arch/i686/irq.h>
#include <stdint.h>
#include <stdio.h>

/* Public API */
void keyboard_init(void);
int keyboard_getchar(void);  // returns -1 if no char, else unsigned char in int

void keyboard_init() { i686_irq_register_handler(1, keyboard_handler); }

/* ===== US layout (set 1) ===== */
unsigned char kbdus[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-',
    0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

unsigned char kbdus_shifted[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-',
    0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* ===== Modifiers ===== */
static int shift_pressed = 0;
static int capslock_enabled = 0;

/* ===== Ring buffer ===== */
#define KBD_BUFFER_SIZE 256
static volatile unsigned char kbd_buf[KBD_BUFFER_SIZE];
static volatile unsigned int kbd_head = 0;
static volatile unsigned int kbd_tail = 0;

static inline int kbd_empty(void) {
    return kbd_head == kbd_tail;
}
static inline int kbd_full(void) {
    return ((kbd_head + 1) & (KBD_BUFFER_SIZE - 1)) == kbd_tail;
}
static inline void kbd_put(unsigned char c) {
    if (!kbd_full()) {
        kbd_buf[kbd_head] = c;
        kbd_head = (kbd_head + 1) & (KBD_BUFFER_SIZE - 1);
    }
}

/* Translate scancode -> ASCII (with modifiers). Returns 0 if unmapped. */
static inline unsigned char kbd_translate(uint8_t sc) {
    if (sc >= 128) return 0;
    unsigned char c = kbdus[sc];
    if (!c) return 0;

    int is_letter = (c >= 'a' && c <= 'z');

    /* Letters: Shift XOR CapsLock toggles case (like PCs) */
    if (is_letter) {
        if (shift_pressed ^ capslock_enabled)
            c = (unsigned char)(c - 'a' + 'A');
        return c;
    }

    /* Non-letters: only Shift changes to symbol table */
    if (shift_pressed) {
        unsigned char s = kbdus_shifted[sc];
        if (s) return s;
    }
    return c;
}

/* ===== IRQ handler ===== */
void keyboard_handler(registers *regs) {
    (void)regs;
    uint8_t scancode = i686_inb(0x60);

    /* Handle extended prefix 0xE0 by ignoring for now or extending your map */
    if (scancode == 0xE0) {
        /* Consume the next byte if you decide to handle arrows/etc. later */
        return;
    }

    if (scancode & 0x80) {
        /* Key release */
        scancode &= 0x7F;
        if (scancode == 0x2A || scancode == 0x36) { /* LShift/RShift */
            shift_pressed = 0;
        }
    } else {
        /* Key press */
        if (scancode == 0x2A || scancode == 0x36) { /* LShift/RShift */
            shift_pressed = 1;
            return;
        }
        if (scancode == 0x3A) { /* CapsLock toggle */
            capslock_enabled = !capslock_enabled;
            return;
        }

        unsigned char c = kbd_translate(scancode);
        if (c) kbd_put(c); /* enqueue only printable/non-zero */
    }
}

/* ===== Non-blocking getchar =====
   Returns -1 if no char; otherwise returns 0..255 (unsigned char in int). */
int keyboard_getchar(void) {
    if (kbd_empty()) return -1;
    unsigned char c = kbd_buf[kbd_tail];
    kbd_tail = (kbd_tail + 1) & (KBD_BUFFER_SIZE - 1);
    return (int)c;
}
