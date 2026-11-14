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

namespace gfot { namespace env {
std::string parent(const std::string &path)
{
    std::string p = path;
    while (!p.empty() && p.back() == '/') p.pop_back();
    if (!p.empty() && p.back() == ':') return p;
    std::string::size_type cut = p.find_last_of("/:");
    if (cut == std::string::npos) return std::string();
    if (p[cut] == ':') return p.substr(0, cut + 1);
    return p.substr(0, cut);
}

std::string human_size(long long bytes)
{
    char b[32];
    if (bytes < 0) bytes = 0;
    if (bytes < 1024) {
        std::snprintf(b, sizeof b, "%lld bytes", bytes);
        return b;
    }
    static const char *unit[] = {"KB", "MB", "GB", "TB", "PB", "EB"};
    int u = 0;
    // hundredths of the unit, cut: whole numbers only, no float rounding
    unsigned long long v = (unsigned long long)bytes, div = 1024;
    while (u < 5 && v / div >= 1024) { div *= 1024; u++; }
    unsigned long long whole = v / div;
    unsigned long long hund = (v % div) * 100 / div;
    if (whole >= 100) std::snprintf(b, sizeof b, "%llu %s", whole, unit[u]);
    else if (whole >= 10) std::snprintf(b, sizeof b, "%llu.%llu %s", whole, hund / 10, unit[u]);
    else std::snprintf(b, sizeof b, "%llu.%02llu %s", whole, hund, unit[u]);
    return b;
}

std::wstring utf8_to_wide(const std::string &s)
{
    std::wstring w;
    const unsigned char *p = (const unsigned char *)s.data(), *e = p + s.size();
    while (p < e) {
        unsigned long c = *p++;
        int more = 0;
        unsigned long min = 0;
        if (c >= 0xf0 && c < 0xf5) { c &= 0x07; more = 3; min = 0x10000; }
        else if (c >= 0xe0 && c < 0xf0) { c &= 0x0f; more = 2; min = 0x800; }
        else if (c >= 0xc2 && c < 0xe0) { c &= 0x1f; more = 1; min = 0x80; }
        else if (c >= 0x80) { w += wchar_t(0xfffd); continue; }
        int got = 0;
        while (got < more && p < e && (*p & 0xc0) == 0x80) { c = (c << 6) | (*p++ & 0x3f); got++; }
        if (got < more || c < min || c > 0x10ffff || (c >= 0xd800 && c < 0xe000)) { w += wchar_t(0xfffd); continue; }
        if (sizeof(wchar_t) == 2 && c >= 0x10000) {
            c -= 0x10000;
            w += wchar_t(0xd800 + (c >> 10));
            w += wchar_t(0xdc00 + (c & 0x3ff));
        } else {
            w += wchar_t(c);
        }
    }
    return w;
}

std::string wide_to_utf8(const std::wstring &w)
{
    std::string s;
    for (std::wstring::size_type i = 0; i < w.size(); i++) {
        unsigned long c = (unsigned long)w[i];
        if (sizeof(wchar_t) == 2) c &= 0xffff;
        if (sizeof(wchar_t) == 2 && c >= 0xd800 && c < 0xdc00 && i + 1 < w.size()
            && ((unsigned long)w[i + 1] & 0xffff) >= 0xdc00 && ((unsigned long)w[i + 1] & 0xffff) < 0xe000) {
            c = 0x10000 + ((c - 0xd800) << 10) + (((unsigned long)w[++i] & 0xffff) - 0xdc00);
        } else if ((c >= 0xd800 && c < 0xe000) || c > 0x10ffff) {
            c = 0xfffd;
        }
        if (c < 0x80) s += char(c);
        else if (c < 0x800) { s += char(0xc0 | (c >> 6)); s += char(0x80 | (c & 0x3f)); }
        else if (c < 0x10000) { s += char(0xe0 | (c >> 12)); s += char(0x80 | ((c >> 6) & 0x3f)); s += char(0x80 | (c & 0x3f)); }
        else { s += char(0xf0 | (c >> 18)); s += char(0x80 | ((c >> 12) & 0x3f)); s += char(0x80 | ((c >> 6) & 0x3f)); s += char(0x80 | (c & 0x3f)); }
    }
    return s;
}
}}
