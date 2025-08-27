#include "kernel/fd.h"
#include "libk/unistd.h"

long write(int fd, const void *buf, size_t n) {
  file_t *f = fd_get(fd);
  if (!f || !f->ops || !f->ops->write)
    return -1;
  return f->ops->write(f->priv, buf, n);
}