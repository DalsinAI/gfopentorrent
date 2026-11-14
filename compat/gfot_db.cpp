// GFOpenTorrent: the database migrations' SQL functions (gfot_db.hpp).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_db.hpp"

bool gfot::db::known_folder_path(std::string const& folder_id, std::string const& downloads, std::string& out)
{
    out.clear();
    if (folder_id != "FOLDERID_Downloads" || downloads.empty())
    {
        return false;
    }
    out = downloads;
    return true;
}

int gfot::db::user_default_ui_language()
{
    return 1033;
}
