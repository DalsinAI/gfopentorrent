/* GFOpenTorrent: the POSIX names libnix lacks, for libtorrent's
 * copy_file.cpp (C symbols, as libnix's <unistd.h> declares them).
 * Copyright (c) 2026 AmiCradle Team. MIT licence. */
#include <unistd.h>
#include "gfot_pio.h"

ssize_t pread(int fd, void *buf, size_t len, off_t off) { return (ssize_t)gfot_pread(fd, buf, len, (long)off); }
ssize_t pwrite(int fd, const void *buf, size_t len, off_t off) { return (ssize_t)gfot_pwrite(fd, buf, len, (long)off); }
