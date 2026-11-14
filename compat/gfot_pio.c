/* GFOpenTorrent: see gfot_pio.h.
 * Copyright (c) 2026 AmiCradle Team. MIT licence. */
#include <errno.h>
#include <unistd.h>
#include "gfot_pio.h"

static long at(int fd, long off, long (*op)(int, void *, unsigned long), void *buf, unsigned long len)
{
    long was = (long)lseek(fd, 0, SEEK_CUR), n;
    int err;
    if (was < 0) return -1;
    if (off < 0) { errno = EINVAL; return -1; }
    if (lseek(fd, off, SEEK_SET) < 0) return -1;
    n = op(fd, buf, len);
    err = errno;
    lseek(fd, was, SEEK_SET);
    errno = err;
    return n;
}

static long do_read(int fd, void *b, unsigned long n) { return (long)read(fd, b, n); }
static long do_write(int fd, void *b, unsigned long n) { return (long)write(fd, b, n); }

long gfot_pread(int fd, void *buf, unsigned long len, long off) { return at(fd, off, do_read, buf, len); }
long gfot_pwrite(int fd, const void *buf, unsigned long len, long off) { return at(fd, off, do_write, (void *)buf, len); }
