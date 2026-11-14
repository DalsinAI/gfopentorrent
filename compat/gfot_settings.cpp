// GFOpenTorrent: settings read as AmigaOS can honour them (gfot_settings.hpp).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_settings.hpp"

void gfot::settings::adjust(std::string const& key, std::string& value)
{
    if (key == "show_in_notification_area"
        || key == "minimize_to_notification_area"
        || key == "close_to_notification_area")
    {
        value = "false";
    }
    // Configuration::WindowState: Hidden (2) needs the tray; Normal (0)
    else if (key == "start_position" && value == "2")
    {
        value = "0";
    }
}
