#include "libk/stdio.h"
#include "libk/unistd.h"

int fputs(const char *s, int fd) { return fprintf(fd, "%s\n", s); }