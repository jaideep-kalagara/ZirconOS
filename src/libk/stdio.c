#include <kernel/tty.h>
#include <kernel/vga.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <stdio.h>

/* ----- Public API ----- */

void putc(const char c) { _putc(c); }
void puts(const char *s) { _puts(s); }

/* ----- Minimal printf ----- */
#define PRINTF_STATE_NORMAL 0
#define PRINTF_STATE_LENGTH 1
#define PRINTF_STATE_LENGTH_SHORT 2
#define PRINTF_STATE_LENGTH_LONG 3
#define PRINTF_STATE_SPEC 4
#define PRINTF_LENGTH_DEFAULT 0
#define PRINTF_LENGTH_SHORT_SHORT 1
#define PRINTF_LENGTH_SHORT 2
#define PRINTF_LENGTH_LONG 3
#define PRINTF_LENGTH_LONG_LONG 4

static const char hex_chars[] = "0123456789abcdef";

static void printf_unsigned_ull(unsigned long long number, int radix) {
  char buf[32];
  int pos = 0;
  do {
    unsigned long long rem = number % (unsigned)radix;
    number /= (unsigned)radix;
    buf[pos++] = hex_chars[(unsigned)rem];
  } while (number);
  while (pos--)
    putc(buf[pos]);
}
static void printf_signed_ll(long long n, int radix) {
  if (n < 0) {
    putc('-');
    printf_unsigned_ull((unsigned long long)(-n), radix);
  } else
    printf_unsigned_ull((unsigned long long)n, radix);
}

void printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int state = PRINTF_STATE_NORMAL, length = PRINTF_LENGTH_DEFAULT, radix = 10;
  bool sign = false, number = false;

  while (*fmt) {
    switch (state) {
    case PRINTF_STATE_NORMAL:
      if (*fmt == '%')
        state = PRINTF_STATE_LENGTH;
      else
        putc(*fmt);
      break;

    case PRINTF_STATE_LENGTH:
      if (*fmt == 'h') {
        length = PRINTF_LENGTH_SHORT;
        state = PRINTF_STATE_LENGTH_SHORT;
      } else if (*fmt == 'l') {
        length = PRINTF_LENGTH_LONG;
        state = PRINTF_STATE_LENGTH_LONG;
      } else
        goto PRINTF_STATE_SPEC_;

      break;

    case PRINTF_STATE_LENGTH_SHORT:
      if (*fmt == 'h') {
        length = PRINTF_LENGTH_SHORT_SHORT;
        state = PRINTF_STATE_SPEC;
      } else
        goto PRINTF_STATE_SPEC_;
      break;

    case PRINTF_STATE_LENGTH_LONG:
      if (*fmt == 'l') {
        length = PRINTF_LENGTH_LONG_LONG;
        state = PRINTF_STATE_SPEC;
      } else
        goto PRINTF_STATE_SPEC_;
      break;

    case PRINTF_STATE_SPEC:
    PRINTF_STATE_SPEC_:
      switch (*fmt) {
      case 'c':
        putc((char)va_arg(args, int));
        break;
      case 's':
        puts(va_arg(args, const char *));
        break;
      case '%':
        putc('%');
        break;
      case 'd':
      case 'i':
        radix = 10;
        sign = true;
        number = true;
        break;
      case 'u':
        radix = 10;
        sign = false;
        number = true;
        break;
      case 'X':
      case 'x':
      case 'p':
        radix = 16;
        sign = false;
        number = true;
        break;
      case 'o':
        radix = 8;
        sign = false;
        number = true;
        break;
      default: /* ignore unknown */
        break;
      }

      if (number) {
        if (sign) {
          switch (length) {
          case PRINTF_LENGTH_LONG_LONG:
            printf_signed_ll(va_arg(args, long long), radix);
            break;
          case PRINTF_LENGTH_LONG:
            printf_signed_ll(va_arg(args, long), radix);
            break;
          default:
            printf_signed_ll(va_arg(args, int), radix);
            break;
          }
        } else {
          switch (length) {
          case PRINTF_LENGTH_LONG_LONG:
            printf_unsigned_ull(va_arg(args, unsigned long long), radix);
            break;
          case PRINTF_LENGTH_LONG:
            printf_unsigned_ull(va_arg(args, unsigned long), radix);
            break;
          default:
            printf_unsigned_ull(va_arg(args, unsigned int), radix);
            break;
          }
        }
      }

      state = PRINTF_STATE_NORMAL;
      length = PRINTF_LENGTH_DEFAULT;
      radix = 10;
      sign = false;
      number = false;
      break;
    }
    fmt++;
  }
  va_end(args);
}
