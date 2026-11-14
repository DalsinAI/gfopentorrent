// GFOpenTorrent: SQLite's file names on AmigaOS (see gfot_sqlvfs.hpp).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_sqlvfs.hpp"

#include <cstring>

#ifndef GFOT_SQLVFS_HOST_TEST
#include <sqlite3.h>
#include <unistd.h>
#endif

std::string gfot::sqlvfs::full_name(std::string const& name, std::string const& cwd)
{
    if (name.find(':') != std::string::npos || cwd.empty()) return name;
    char last = cwd.back();
    return cwd + ((last == ':' || last == '/') ? "" : "/") + name;
}

#ifndef GFOT_SQLVFS_HOST_TEST
namespace
{
    sqlite3_vfs amiga_vfs;

    int amiga_full_pathname(sqlite3_vfs*, const char* name, int n_out, char* out)
    {
        char buf[1024];
        std::string cwd = getcwd(buf, sizeof buf) ? buf : "";
        std::string full = gfot::sqlvfs::full_name(name ? name : "", cwd);
        if (static_cast<int>(full.size()) + 1 > n_out) return SQLITE_CANTOPEN;
        std::memcpy(out, full.c_str(), full.size() + 1);
        return SQLITE_OK;
    }

    // before PicoTorrent opens its database
    const bool installed = gfot::sqlvfs::install();
}

bool gfot::sqlvfs::install()
{
    static bool done = false;
    if (done) return true;
    // libnix has no POSIX file locks (fcntl F_SETLK), so SQLite's locking
    // layer gave a disk I/O error on the first read (lab, 14 Nov 2026).
    // Only one GFOpenTorrent runs at a time (its ARexx port), so SQLite's
    // no-locking layer is the base, the default one only if that is absent.
    sqlite3_vfs* base = sqlite3_vfs_find("unix-none");
    if (!base) base = sqlite3_vfs_find(nullptr);
    if (!base) return false;
    amiga_vfs = *base;          // its methods and their own data (pAppData)
    amiga_vfs.pNext = nullptr;
    amiga_vfs.zName = "gfot-amiga";
    amiga_vfs.xFullPathname = amiga_full_pathname;
    done = sqlite3_vfs_register(&amiga_vfs, 1) == SQLITE_OK;
    (void)installed;
    return done;
}
#endif
