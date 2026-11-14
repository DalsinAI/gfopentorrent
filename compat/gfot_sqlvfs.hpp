// GFOpenTorrent: SQLite's file names on AmigaOS. SQLite's Unix layer makes
// a "full" name by putting the current directory and a / in front of any
// name that does not start with /, so PROGDIR:PicoTorrent.sqlite became
// DH0:T/torrent/PROGDIR:PicoTorrent.sqlite and the database never opened
// (lab, 14 Nov 2026). full_name() keeps a name with a volume or assign
// (a colon) as it is and joins a relative one to the current directory the
// AmigaDOS way; install() puts it in front of SQLite's own file layer.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

#include <string>

namespace gfot::sqlvfs
{
    // "PROGDIR:x" or "DH0:T/x" as given; "x" in "DH0:" is "DH0:x", in
    // "DH0:T" is "DH0:T/x"; with no current directory, as given.
    std::string full_name(std::string const& name, std::string const& cwd);

    // Registers SQLite's own no-locking file layer ("unix-none": libnix has
    // no POSIX file locks) with full_name() as the default, once; true when
    // it is in place. Done before PicoTorrent's database is opened (a
    // static object in gfot_sqlvfs.cpp).
    bool install();
}
