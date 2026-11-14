/* GFOpenTorrent: fchown and fchmod, which SQLite calls and libnix lacks.
 * An Amiga file has no Unix owner, and SQLite's mode change is only for a
 * new journal to match its database's mode, so both report success and
 * change nothing.
 * Copyright (c) 2026 AmiCradle Team. MIT licence. */
#include <sys/types.h>

int fchown(int fd, uid_t owner, gid_t group)
{
    (void)fd; (void)owner; (void)group;
    return 0;
}

int fchmod(int fd, mode_t mode)
{
    (void)fd; (void)mode;
    return 0;
}
