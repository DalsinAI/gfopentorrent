// GFOpenTorrent: the database migrations' SQL functions (compat/gfot_db.cpp)
// and the migrations that call them (res/dbmigrations, given as argv[1]).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_db.hpp"

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

static std::string slurp(std::string const& path)
{
    std::ifstream in(path, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main(int argc, char** argv)
{
    std::string out = "x";

    check(gfot::db::known_folder_path("FOLDERID_Downloads", "Work:Torrents", out), "FOLDERID_Downloads answers");
    check(out == "Work:Torrents", "with the download drawer");
    check(gfot::db::known_folder_path("FOLDERID_Downloads", "PROGDIR:Downloads", out) && out == "PROGDIR:Downloads", "PROGDIR:Downloads passes as it is");
    check(!gfot::db::known_folder_path("FOLDERID_Documents", "Work:Torrents", out), "any other folder id is NULL");
    check(out.empty(), "and gives no text");
    check(!gfot::db::known_folder_path("FOLDERID_Downloads", "", out), "an empty drawer name is NULL");
    check(gfot::db::user_default_ui_language() == 1033, "the language id default is 1033 (en-US)");

    if (argc > 1)
    {
        // the migrations ask only for the ids gfot_db answers, and the
        // language comes from locale_name, not language_id
        std::string dir = argv[1];
        std::string m2018 = slurp(dir + "/20181208115732_insert_default_settings.sql");
        std::string m2020 = slurp(dir + "/20200912230012_enhance_setting_table.sql");
        std::string mloc = slurp(dir + "/20200916213321_insert_locale_name_setting.sql");
        check(!m2018.empty() && !m2020.empty() && !mloc.empty(), "the three migrations are there");
        check(m2018.find("get_known_folder_path('FOLDERID_Downloads')") != std::string::npos, "2018 asks for FOLDERID_Downloads");
        check(m2020.find("get_known_folder_path('FOLDERID_Downloads')") != std::string::npos, "2020 asks for FOLDERID_Downloads");
        std::string::size_type p = 0;
        int other = 0;
        for (std::string const* s : {&m2018, &m2020})
        {
            p = 0;
            while ((p = s->find("get_known_folder_path('", p)) != std::string::npos)
            {
                p += 23;
                if (s->compare(p, 18, "FOLDERID_Downloads") != 0)
                {
                    other++;
                }
            }
        }
        check(other == 0, "no migration asks for another known folder");
        check(mloc.find("'locale_name'") != std::string::npos, "locale_name is a setting of its own");
    }

    // the migrations table (tools/stove/gen-migrations.sh): every .sql,
    // in name order (the order Windows enumerates them), none empty
    check(gfot::db::migration_count >= 23, "the table holds the 23 migrations of PicoTorrent 0.25");
    check(std::string(gfot::db::migrations[0].name) == "20181208115043_create_setting_table", "the setting table comes first");
    bool ordered = true, filled = true;
    for (std::size_t i = 0; i < gfot::db::migration_count; i++)
    {
        if (i > 0 && std::string(gfot::db::migrations[i - 1].name) >= gfot::db::migrations[i].name)
        {
            ordered = false;
        }
        std::string sql = gfot::db::migrations[i].sql;
        if (sql.find_first_not_of(" \t\r\n") == std::string::npos || sql.find('\r') != std::string::npos)
        {
            filled = false;
        }
    }
    check(ordered, "the migrations run in name order, each once");
    check(filled, "each migration has its SQL, with plain line ends");
    if (argc > 1)
    {
        std::string first = slurp(std::string(argv[1]) + "/20181208115043_create_setting_table.sql");
        std::string::size_type e;
        while ((e = first.find('\r')) != std::string::npos) first.erase(e, 1);
        check(first == gfot::db::migrations[0].sql, "a migration's text is its .sql file's, byte for byte");
    }

    std::printf("db_functions_test: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
