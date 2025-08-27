#include "kernel/fd.h"
#include "libk/unistd.h"

int dup2(int oldfd, int newfd) {
  file_t *f = fd_get(oldfd);
  if (!f || newfd < 0 || newfd >= MAX_FD)
    return -1;
  if (fd_get(newfd))
    fd_close(newfd);
  extern file_t *g_fds[];
  g_fds[newfd] = f;
  f->refcnt++;
  return newfd;
}