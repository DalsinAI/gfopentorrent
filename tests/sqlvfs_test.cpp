// GFOpenTorrent: SQLite's file names on AmigaOS (compat/gfot_sqlvfs.cpp,
// built with GFOT_SQLVFS_HOST_TEST: the name rule without SQLite).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_sqlvfs.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static int failed = 0;
static int passed = 0;

static void check(bool ok, char const* what)
{
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    (ok ? passed : failed)++;
}

int main(int argc, char** argv)
{
    using gfot::sqlvfs::full_name;
    check(full_name("PROGDIR:PicoTorrent.sqlite", "DH0:T/torrent") == "PROGDIR:PicoTorrent.sqlite",
        "an assign is kept as given (the lab failure)");
    check(full_name("DH0:T/x.sqlite", "Work:") == "DH0:T/x.sqlite", "a volume path is kept");
    check(full_name("x.sqlite", "DH0:T/torrent") == "DH0:T/torrent/x.sqlite", "relative: joined with /");
    check(full_name("x.sqlite", "DH0:") == "DH0:x.sqlite", "relative in a volume's root: no /");
    check(full_name("x.sqlite", "DH0:T/") == "DH0:T/x.sqlite", "a drawer ending in /: no second /");
    check(full_name("sub/x.sqlite", "RAM:") == "RAM:sub/x.sqlite", "relative with a drawer");
    check(full_name("x.sqlite", "") == "x.sqlite", "no current directory: as given");
    check(full_name("x.sqlite-journal", "PROGDIR:") == "PROGDIR:x.sqlite-journal", "journal beside it");

    if (argc > 1)
    {
        std::ifstream in(std::string(argv[1]) + "/tools/stove/build-picotorrent.sh");
        std::stringstream ss;
        ss << in.rdbuf();
        std::string sh = ss.str();
        check(sh.find(" gfot_sqlvfs") != std::string::npos, "the stove build compiles gfot_sqlvfs");
    }
    std::printf("%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
