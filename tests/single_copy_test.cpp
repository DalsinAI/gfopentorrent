// GFOpenTorrent: the single copy's ARexx commands (compat/gfot_rexx.cpp) and
// the tray settings (compat/gfot_settings.cpp), on the host. The commands
// are read back with a ReadArgs-style splitter, as arexx.class reads them.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_rexx.hpp"
#include "gfot_settings.hpp"

#include <cstdio>
#include <string>
#include <vector>

static int fails;
static void check(bool ok, const char* what)
{
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) fails++;
}

// ReadArgs' word splitting: blanks part words; "..." is one word, with *"
// a quote, ** a star, *N a new line and *E escape inside
static std::vector<std::string> words(std::string const& s)
{
    std::vector<std::string> out;
    size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && s[i] == ' ') i++;
        if (i == s.size()) break;
        std::string w;
        if (s[i] == '"') {
            for (i++; i < s.size() && s[i] != '"'; i++) {
                if (s[i] == '*' && i + 1 < s.size()) {
                    char c = s[++i];
                    w += c == 'N' || c == 'n' ? '\n' : c == 'E' || c == 'e' ? '\x1b' : c;
                } else {
                    w += s[i];
                }
            }
            i++;
        } else {
            while (i < s.size() && s[i] != ' ') w += s[i++];
        }
        out.push_back(w);
    }
    return out;
}

int main()
{
    using gfot::rexx::commands;
    using gfot::rexx::quote;
    using V = std::vector<std::string>;

    check(std::string(gfot::rexx::first_port) == std::string(gfot::rexx::port_base) + ".1",
        "the first copy's port is the base name in its first slot");
    check(quote("Work:a.torrent") == "\"Work:a.torrent\"", "a plain name is quoted");
    check(quote("a \"b\" *c\nd\x1b") == "\"a *\"b*\" **c*Nd*E\"", "quotes, stars, new lines and escape are escaped");

    std::string odd = "Work:My \"best\" *.torrent";
    check(words(quote(odd)) == V{ odd }, "an odd name reads back whole");

    auto c = commands({ "Work:a b.torrent", "RAM:c.torrent" }, {}, "", false);
    check(c.size() == 1 && words(c[0]) == V{ "ADD", "Work:a b.torrent", "RAM:c.torrent" }, "files: one ADD with every file");

    std::string m = "magnet:?xt=urn:btih:0123456789abcdef0123456789abcdef01234567&dn=a b";
    c = commands({}, { m }, "Work:Downloads", true);
    check(c.size() == 1 && words(c[0]) == V{ "MAGNET", m, "SAVEPATH", "Work:Downloads", "SILENT" },
        "a magnet link with SAVEPATH and SILENT");

    c = commands({ "a.torrent" }, { m }, "", false);
    check(c.size() == 2 && words(c[0])[0] == "ADD" && words(c[1])[0] == "MAGNET", "files and links: ADD, then MAGNET");

    c = commands({}, {}, "Work:x", true);
    check(c == V{ "ACTIVATE" }, "nothing given: ACTIVATE");

    check(!gfot::rexx::first_running() && !gfot::rexx::send("ACTIVATE"), "no port on the host");

    auto adjusted = [](const char* key, const char* value) {
        std::string v = value;
        gfot::settings::adjust(key, v);
        return v;
    };
    check(adjusted("show_in_notification_area", "true") == "false", "no tray icon");
    check(adjusted("minimize_to_notification_area", "true") == "false", "iconify never hides in the tray");
    check(adjusted("close_to_notification_area", "true") == "false", "close never hides in the tray");
    check(adjusted("start_position", "2") == "0", "a start hidden in the tray starts normal");
    check(adjusted("start_position", "3") == "3" && adjusted("start_position", "1") == "1", "maximized and minimized stay");
    check(adjusted("locale_name", "\"de-DE\"") == "\"de-DE\"", "other settings stay");

    std::printf("single_copy_test: %d failed\n", fails);
    return fails != 0;
}
