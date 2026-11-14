/* GFOpenTorrent host test: gfot_pread()/gfot_pwrite() (compat/gfot_pio.c),
 * the pread()/pwrite() the Amiga build gives libnix. One line a check,
 * "pio_test: N failed" at the end.
 * Copyright (c) 2026 AmiCradle Team. MIT licence. */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gfot_pio.h"

static int failed = 0;
static void check(const char *what, int ok)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    fflush(stdout);
    if (!ok) failed++;
}

int main(void)
{
    char path[] = "/tmp/gfotpio-XXXXXX", buf[16];
    int fd = mkstemp(path);
    check("temp file", fd >= 0);
    if (fd < 0) return 1;
    check("write 0123456789", write(fd, "0123456789", 10) == 10);
    lseek(fd, 3, SEEK_SET);
    memset(buf, 0, sizeof buf);
    check("pread 4 at 5", gfot_pread(fd, buf, 4, 5) == 4 && memcmp(buf, "5678", 4) == 0);
    check("pread leaves the offset", lseek(fd, 0, SEEK_CUR) == 3);
    check("pwrite 2 at 1", gfot_pwrite(fd, "AB", 2, 1) == 2);
    check("pwrite leaves the offset", lseek(fd, 0, SEEK_CUR) == 3);
    check("pread past the end gives 0", gfot_pread(fd, buf, 4, 100) == 0);
    memset(buf, 0, sizeof buf);
    check("file is 0AB3456789", gfot_pread(fd, buf, 10, 0) == 10 && memcmp(buf, "0AB3456789", 10) == 0);
    errno = 0;
    check("negative offset is EINVAL", gfot_pread(fd, buf, 1, -1) == -1 && errno == EINVAL);
    close(fd);
    errno = 0;
    check("closed descriptor fails with EBADF", gfot_pread(fd, buf, 1, 0) == -1 && errno == EBADF);
    unlink(path);
    printf("pio_test: %d failed\n", failed);
    return failed ? 1 : 0;
}
