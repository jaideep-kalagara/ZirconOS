#include "kernel/fd.h"
#include "libk/unistd.h"

long read(int fd, void *buf, size_t n) {
  file_t *f = fd_get(fd);
  if (!f || !f->ops || !f->ops->read)
    return -1;
  return f->ops->read(f->priv, buf, n);
}