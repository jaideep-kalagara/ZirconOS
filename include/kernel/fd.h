#pragma once
#include <stddef.h>
#include <stdint.h>

typedef long (*fd_read_t)(void *priv, void *buf, size_t n);
typedef long (*fd_write_t)(void *priv, const void *buf, size_t n);
typedef long (*fd_seek_t)(void *priv, long off, int whence);
typedef int (*fd_close_t)(void *priv);

typedef struct file_ops {
  fd_read_t read;
  fd_write_t write;
  fd_seek_t seek;
  fd_close_t close;
} file_ops_t;

typedef struct file {
  const file_ops_t *ops;
  void *priv; // device/inode pointer (driver-owned)
  int flags;
  long pos;
  int refcnt;
} file_t;

#define MAX_FD 64
int fd_install(file_t *f); // returns fd >= 0
file_t *fd_get(int fd);    // NULL if invalid
int fd_close(int fd);
