#include "libk/stdio.h"
#include <stdarg.h>

int puts(const char *s) { return fprintf(STDOUT_FILENO, "%s\n", s); }