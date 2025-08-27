#include "kernel/fd.h"
#include "libk/unistd.h"

int dup(int oldfd) {
  file_t *f = fd_get(oldfd);
  if (!f)
    return -1;
  for (int i = 0; i < MAX_FD; i++)
    if (!fd_get(i)) {
      f->refcnt++;
      extern file_t *g_fds[];
      g_fds[i] = f;
      return i;
    }
  return -1;
}