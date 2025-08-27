#include "libk/stdio.h"
#include "libk/unistd.h"

int putchar(int c) {
    char ch = (char)c;
    return (int)write(STDOUT_FILENO, &ch, 1);
}