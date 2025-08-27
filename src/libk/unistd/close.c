#include "kernel/fd.h"
#include "libk/unistd.h"

int close(int fd) { return fd_close(fd); }