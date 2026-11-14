// GFOpenTorrent: the two SQL functions PicoTorrent's database migrations
// call, answered without Windows' shell. get_known_folder_path gives the
// download drawer (Environment's KnownFolder::UserDownloads: PROGDIR:
// Downloads/ or ENV:GFOpenTorrent/Downloads); get_user_default_ui_language
// gives a fixed language id, as the language now comes from the
// locale_name setting and locale.library, not from that id. And the
// migrations themselves, which Windows keeps as resources.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

#include <cstddef>
#include <string>

namespace gfot
{
namespace db
{
    // folder_id: the migration's argument ("FOLDERID_Downloads");
    // downloads: the download drawer. False (SQL NULL) for any other id
    // or an empty drawer name.
    bool known_folder_path(std::string const& folder_id, std::string const& downloads, std::string& out);

    // the language_id setting's default: 1033 (en-US); only a 2018
    // migration reads it, and the 2020 ones replace it with locale_name
    int user_default_ui_language();

    // the migrations, in resources.rc's order, in place of the Windows
    // resources Database::Migrate enumerates (tools/stove/gen-migrations.sh
    // writes the table)
    struct migration
    {
        const char* name;
        const char* sql;
    };
    extern const migration migrations[];
    extern const std::size_t migration_count;
}
}
