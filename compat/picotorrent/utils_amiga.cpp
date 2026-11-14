// GFOpenTorrent: PicoTorrent's core/utils.cpp for AmigaOS, in place of the
// Windows one (explorer.exe, StrFormatByteSize64, WideCharToMultiByte).
// "Open in Explorer" opens the drawer that holds the torrent's data in
// Workbench (workbench.library 44's OpenWorkbenchObjectA); sizes read as on
// Windows; the string conversions are UTF-8 both ways (gfot_env.cpp).
// Copyright (c) 2026 AmiCradle Team. MIT licence (see LICENSE).
#include "picotorrent/core/utils.hpp"

#include <boost/log/trivial.hpp>
#include <string>

#include "gfot_env.hpp"

#if defined __amigaos__ || defined __AMIGA__
#include <proto/exec.h>
#include <proto/wb.h>
#include <utility/tagitem.h>
#endif

namespace fs = std::filesystem;
using pt::Utils;

void Utils::openAndSelect(fs::path path)
{
    std::string drawer = gfot::env::parent(path.string());
    if (drawer.empty()) drawer = "PROGDIR:";
#if defined __amigaos__ || defined __AMIGA__
    bool opened = false;
    if (struct Library *WorkbenchBase = OpenLibrary((CONST_STRPTR) "workbench.library", 44)) {
        opened = OpenWorkbenchObjectA((STRPTR)drawer.c_str(), nullptr) != FALSE;
        CloseLibrary(WorkbenchBase);
    }
    if (!opened) BOOST_LOG_TRIVIAL(warning) << "Workbench did not open " << drawer;
#else
    // host tests: say which drawer would open
    BOOST_LOG_TRIVIAL(info) << "open drawer " << drawer;
#endif
}

std::wstring Utils::toHumanFileSize(int64_t bytes)
{
    return gfot::env::utf8_to_wide(gfot::env::human_size((long long)bytes));
}

std::string Utils::toStdString(std::wstring const &input)
{
    return gfot::env::wide_to_utf8(input);
}

std::wstring Utils::toStdWString(std::string const &input)
{
    return gfot::env::utf8_to_wide(input);
}
