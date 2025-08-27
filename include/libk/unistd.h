#pragma once
#include <stddef.h>
#include <stdint.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

int open(const char *path, int flags);  // minimal stub for now
int close(int fd);
long read(int fd, void *buf, size_t n);
long write(int fd, const void *buf, size_t n);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
