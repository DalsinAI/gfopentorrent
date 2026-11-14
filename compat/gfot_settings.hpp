// GFOpenTorrent: PicoTorrent settings that mean nothing on AmigaOS, read as
// what does. Workbench has no notification area, so the tray settings read
// false (closing or iconifying never hides the window where no icon can
// bring it back), and a start "hidden" in the tray starts normal.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

#include <string>

namespace gfot
{
namespace settings
{
    // value: the setting's JSON text as the database gave it; changed in
    // place for the settings above
    void adjust(std::string const& key, std::string& value);
}
}
