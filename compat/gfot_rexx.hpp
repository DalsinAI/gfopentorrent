// GFOpenTorrent: one copy of PicoTorrent at a time on AmigaOS. The first
// copy's ARexx port (GFOPENTORRENT.1, the GFOpenWX Toolkit's wxARexxPort)
// takes what a second start was given, in place of Windows' named mutex
// (wxSingleInstanceChecker) and DDE (wxServer/wxClient).
//   ADD FILE/M/A,SAVEPATH/K,SILENT/S      torrent files to add
//   MAGNET LINK/M/A,SAVEPATH/K,SILENT/S   magnet links to add
//   ACTIVATE                              (the toolkit's) the window to the front
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

#include <string>
#include <vector>

namespace gfot
{
namespace rexx
{
    // the port's name without its slot, as wxARexxPort takes it
    extern const char* const port_base;
    // the first copy's port
    extern const char* const first_port;

    // s as one ReadArgs argument: in double quotes, with ReadArgs' escapes
    // (*" for a quote, ** for a star, *N for a new line, *E for escape)
    std::string quote(std::string const& s);

    // the commands a second start sends the first copy, in order: ADD for
    // files, MAGNET for magnet links, ACTIVATE when it was given neither
    std::vector<std::string> commands(
        std::vector<std::string> const& files,
        std::vector<std::string> const& magnets,
        std::string const& save_path,
        bool silent);

    // AmigaOS only (false on the build machine): whether the first copy's
    // port is there; send one command to it and wait for the reply
    bool first_running();
    bool send(std::string const& command);
    // wait (up to ms) for the first copy's port to go, as Windows waits for
    // the old process with --wait-for-pid
    void wait_gone(long ms);
}
}
