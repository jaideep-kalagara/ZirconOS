#include "kernel/fd.h"

#include <stddef.h>

static file_t *g_fds[MAX_FD];

int fd_install(file_t *f) {
    for (int i = 0; i < MAX_FD; i++) {
        if (!g_fds[i]) {
            g_fds[i] = f;
            f->refcnt = 1;
            return i;
        }
    }
    return -1;
}
file_t *fd_get(int fd) {
    if (fd < 0 || fd >= MAX_FD)
        return NULL;
    return g_fds[fd];
}
int fd_close(int fd) {
    file_t *f = fd_get(fd);
    if (!f)
        return -1;
    if (f->refcnt > 0 && --f->refcnt == 0) {
        if (f->ops && f->ops->close)
            f->ops->close(f->priv);
    }
    g_fds[fd] = NULL;
    return 0;
}
