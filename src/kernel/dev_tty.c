#include "kernel/dev_tty.h"
#include "arch/i686/io.h"
#include "kernel/fd.h"
#include <kernel/tty.h>
#include <stddef.h>
#include <stdint.h>

static long tty_write(void *priv, const void *buf, size_t n) {
  (void)priv;
  const char *p = (const char *)buf;
  for (size_t i = 0; i < n; i++)
    _putc(p[i]);
  return (long)n;
}
static long tty_read(void *priv, void *buf, size_t n) {
  // TODO: implement non-blocking read
  //   (void)priv;
  //   char *out = (char *)buf;
  //   size_t got = 0;
  //   while (got < n) {
  //     int c = kbd_getchar();
  //     if (c < 0)
  //       break; // make read() non-blocking for now
  //     out[got++] = (char)c;
  //   }
  //   return (long)got;
  return -1;
}
static int tty_close(void *priv) {
  (void)priv;
  return 0;
}

static const file_ops_t TTY_IN_OPS = {
    .read = tty_read, .write = NULL, .seek = NULL, .close = tty_close};
static const file_ops_t TTY_OUT_OPS = {
    .read = NULL, .write = tty_write, .seek = NULL, .close = tty_close};

int dev_tty_install_std(void) {
  static file_t tty_in = {
      .ops = &TTY_IN_OPS, .priv = NULL, .flags = 0, .pos = 0, .refcnt = 0};
  static file_t tty_out = {
      .ops = &TTY_OUT_OPS, .priv = NULL, .flags = 0, .pos = 0, .refcnt = 0};

  int fd0 = fd_install(&tty_in);
  int fd1 = fd_install(&tty_out);
  int fd2 = fd1; // stderr -> same as stdout for now

  if (fd0 != 0 || fd1 != 1) {
    printf("failed to install fds 0,1\n");
    i686_panic();
    return -1;
  }
  (void)fd2;
  return 0;
}
