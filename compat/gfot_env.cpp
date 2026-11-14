// GFOpenTorrent: see gfot_env.hpp.
// Copyright (c) 2026 AmiCradle Team. MIT licence (see LICENSE).
#include "gfot_env.hpp"
#include <cctype>
#include <cstdio>
#include <cstring>

namespace gfot { namespace env {
std::string join(const std::string &dir, const std::string &name)
{
    if (dir.empty()) return name;
    char last = dir[dir.size() - 1];
    if (last == ':' || last == '/') return dir + name;
    return dir + "/" + name;
}

std::string log_file_name(std::time_t t)
{
    std::tm tm = *std::localtime(&t);
    char b[64];
    std::snprintf(b, sizeof b, "PicoTorrent.%04d%02d%02d%02d%02d%02d.log", tm.tm_year + 1900, tm.tm_mon + 1,
                  tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return b;
}

std::string language_code(const std::string &amiga_language)
{
    // the name without ".language", lower case (ASCII only: the Latin-1 and
    // UTF-8 bytes of "français" and the rest are compared as they are)
    std::string n = amiga_language;
    const std::string ext = ".language";
    if (n.size() > ext.size() && n.compare(n.size() - ext.size(), ext.size(), ext) == 0)
        n.erase(n.size() - ext.size());
    for (auto &c : n) c = char(std::tolower((unsigned char)c));
    static const struct { const char *name; const char *code; } table[] = {
        {"english", "en"}, {"english_british", "en"}, {"american", "en"},
        {"deutsch", "de"}, {"dansk", "da"}, {"nederlands", "nl"}, {"svenska", "sv"},
        {"norsk", "nb"}, {"suomi", "fi"}, {"italiano", "it"}, {"polski", "pl"},
        {"magyar", "hu"}, {"czech", "cs"}, {"\xc4\x8d" "esky", "cs"}, {"greek", "el"},
        {"russian", "ru"}, {"turkish", "tr"}, {"t\xfcrk\xe7" "e", "tr"}, {"t\xc3\xbcrk\xc3\xa7" "e", "tr"},
        {"fran\xe7" "ais", "fr"}, {"fran\xc3\xa7" "ais", "fr"}, {"francais", "fr"},
        {"espa\xf1" "ol", "es"}, {"espa\xc3\xb1" "ol", "es"}, {"espanol", "es"},
        {"portugu\xea" "s", "pt"}, {"portugu\xc3\xaa" "s", "pt"}, {"portugues", "pt"},
    };
    for (auto const &e : table)
        if (n == e.name) return e.code;
    return "en";
}
}}
