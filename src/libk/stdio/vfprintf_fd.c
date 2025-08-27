#include "libk/unistd.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ---------- tiny buffered sink ---------- */

#define OUTBUF_CAP 512

typedef struct {
  int fd;
  char buf[OUTBUF_CAP];
  size_t len;
  int total; // chars successfully flushed
  int err;   // sticky error
} outbuf_t;

static inline void ob_init(outbuf_t *ob, int fd) {
  ob->fd = fd;
  ob->len = 0;
  ob->total = 0;
  ob->err = 0;
}

static inline void ob_flush(outbuf_t *ob) {
  if (ob->err || ob->len == 0)
    return;
  long w = write(ob->fd, ob->buf, ob->len);
  if (w < 0) {
    ob->err = 1;
    return;
  }
  ob->total += (int)w;
  ob->len = 0;
}

static inline void ob_putc(outbuf_t *ob, char c) {
  if (ob->len == OUTBUF_CAP)
    ob_flush(ob);
  ob->buf[ob->len++] = c;
}

static inline void ob_write(outbuf_t *ob, const char *s, size_t n) {
  while (n) {
    size_t space = OUTBUF_CAP - ob->len;
    if (space == 0) {
      ob_flush(ob);
      space = OUTBUF_CAP;
    }
    size_t chunk = (n < space) ? n : space;
    memcpy(ob->buf + ob->len, s, chunk);
    ob->len += chunk;
    n -= chunk;
  }
}

static inline void ob_puts(outbuf_t *ob, const char *s) {
  ob_write(ob, s, strlen(s));
}

/* ---------- number formatting (no width/flags yet) ---------- */

static const char HEX_LOWER[] = "0123456789abcdef";
static const char HEX_UPPER[] = "0123456789ABCDEF";

static void ob_put_u64(outbuf_t *ob, unsigned long long v, int radix,
                       bool upper) {
  char tmp[32];
  const char *hex = upper ? HEX_UPPER : HEX_LOWER;
  int i = 0;
  do {
    unsigned long long q = v / (unsigned)radix;
    unsigned rem = (unsigned)(v - q * (unsigned)radix);
    tmp[i++] = hex[rem];
    v = q;
  } while (v && i < (int)sizeof(tmp));
  while (i--)
    ob_putc(ob, tmp[i]);
}

static void ob_put_s64(outbuf_t *ob, long long v, int radix, bool upper) {
  if (v < 0) {
    ob_putc(ob, '-');
    ob_put_u64(ob, (unsigned long long)(-v), radix, upper);
  } else
    ob_put_u64(ob, (unsigned long long)v, radix, upper);
}

/* ---------- buffered vfprintf_fd ---------- */

#define PRINTF_STATE_NORMAL 0
#define PRINTF_STATE_LENGTH 1
#define PRINTF_STATE_LENGTH_SHORT 2
#define PRINTF_STATE_LENGTH_LONG 3
#define PRINTF_STATE_SPEC 4

#define LEN_DEF 0
#define LEN_HH 1
#define LEN_H 2
#define LEN_L 3
#define LEN_LL 4

int vfprintf_fd(int fd, const char *fmt, va_list ap) {
  outbuf_t ob;
  ob_init(&ob, fd);

  int state = PRINTF_STATE_NORMAL, len = LEN_DEF, radix = 10;
  bool sign = false, number = false, upper = false;

  while (*fmt) {
    switch (state) {
    case PRINTF_STATE_NORMAL:
      if (*fmt == '%')
        state = PRINTF_STATE_LENGTH;
      else
        ob_putc(&ob, *fmt);
      break;

    case PRINTF_STATE_LENGTH:
      if (*fmt == 'h') {
        len = LEN_H;
        state = PRINTF_STATE_LENGTH_SHORT;
      } else if (*fmt == 'l') {
        len = LEN_L;
        state = PRINTF_STATE_LENGTH_LONG;
      } else
        goto SPEC;

      break;

    case PRINTF_STATE_LENGTH_SHORT:
      if (*fmt == 'h') {
        len = LEN_HH;
        state = PRINTF_STATE_SPEC;
      } else
        goto SPEC;
      break;

    case PRINTF_STATE_LENGTH_LONG:
      if (*fmt == 'l') {
        len = LEN_LL;
        state = PRINTF_STATE_SPEC;
      } else
        goto SPEC;
      break;

    case PRINTF_STATE_SPEC:
    SPEC:
      switch (*fmt) {
      case 'c': {
        int c = va_arg(ap, int);
        ob_putc(&ob, (char)c);
      } break;

      case 's': {
        const char *s = va_arg(ap, const char *);
        if (!s)
          s = "(null)";
        ob_puts(&ob, s);
      } break;

      case '%':
        ob_putc(&ob, '%');
        break;

      case 'd':
      case 'i':
        radix = 10;
        sign = true;
        number = true;
        upper = false;
        break;

      case 'u':
        radix = 10;
        sign = false;
        number = true;
        upper = false;
        break;

      case 'x':
        radix = 16;
        sign = false;
        number = true;
        upper = false;
        break;

      case 'X':
        radix = 16;
        sign = false;
        number = true;
        upper = true;
        break;

      case 'o':
        radix = 8;
        sign = false;
        number = true;
        upper = false;
        break;

      case 'p': {
        uintptr_t p = (uintptr_t)va_arg(ap, void *);
        ob_puts(&ob, "0x");
        ob_put_u64(&ob, (unsigned long long)p, 16, false);
      } break;

      default:
        /* ignore unknown specifier */
        break;
      }

      if (number) {
        if (sign) {
          switch (len) {
          case LEN_LL:
            ob_put_s64(&ob, va_arg(ap, long long), radix, upper);
            break;
          case LEN_L:
            ob_put_s64(&ob, va_arg(ap, long), radix, upper);
            break;
          default:
            ob_put_s64(&ob, va_arg(ap, int), radix, upper);
            break;
          }
        } else {
          switch (len) {
          case LEN_LL:
            ob_put_u64(&ob, va_arg(ap, unsigned long long), radix, upper);
            break;
          case LEN_L:
            ob_put_u64(&ob, va_arg(ap, unsigned long), radix, upper);
            break;
          default:
            ob_put_u64(&ob, va_arg(ap, unsigned int), radix, upper);
            break;
          }
        }
      }

      /* reset state */
      state = PRINTF_STATE_NORMAL;
      len = LEN_DEF;
      radix = 10;
      sign = false;
      number = false;
      upper = false;
      break;
    }
    fmt++;
  }

  ob_flush(&ob);
  return ob.err ? -1 : ob.total;
}