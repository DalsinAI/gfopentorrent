/* GFOpenTorrent: pread()/pwrite() for libnix, which has neither
 * (libtorrent's copy_file.cpp uses them). The program runs on one task, so
 * moving the descriptor's offset and putting it back is safe.
 * Copyright (c) 2026 AmiCradle Team. MIT licence. */
#ifndef GFOT_PIO_H
#define GFOT_PIO_H
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif
/* read or write at off; the descriptor's own offset is left where it was */
long gfot_pread(int fd, void *buf, unsigned long len, long off);
long gfot_pwrite(int fd, const void *buf, unsigned long len, long off);
#ifdef __cplusplus
}
#endif
#endif
