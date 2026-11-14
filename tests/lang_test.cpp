// GFOpenTorrent: compat/gfot_lang.cpp, the language list's names, on the
// host. Every lang/*.json PicoTorrent has must have its own name.
//   lang_test <PicoTorrent's lang/ directory>
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_lang.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

static int fails;
static void check(bool ok, std::string const& what)
{
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what.c_str());
    if (!ok) fails++;
}

// well-formed UTF-8, no overlong forms or surrogates
static bool utf8(std::string const& s)
{
    for (size_t i = 0; i < s.size();) {
        unsigned char c = s[i];
        int n = c < 0x80 ? 0 : (c & 0xe0) == 0xc0 ? 1 : (c & 0xf0) == 0xe0 ? 2 : (c & 0xf8) == 0xf0 ? 3 : -1;
        if (n < 0 || i + n >= s.size() + (n ? 0 : 1)) return false;
        unsigned long cp = n == 0 ? c : n == 1 ? (c & 0x1f) : n == 2 ? (c & 0x0f) : (c & 0x07);
        for (int k = 1; k <= n; k++) {
            unsigned char d = s[i + k];
            if ((d & 0xc0) != 0x80) return false;
            cp = (cp << 6) | (d & 0x3f);
        }
        static const unsigned long min[] = { 0, 0x80, 0x800, 0x10000 };
        if (cp < min[n] || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) return false;
        i += n + 1;
    }
    return true;
}

int main(int argc, char** argv)
{
    if (argc != 2) { std::printf("usage: lang_test <lang dir>\n"); return 2; }
    int files = 0;
    for (auto const& e : std::filesystem::directory_iterator(argv[1])) {
        if (e.path().extension() != ".json") continue;
        files++;
        std::string loc = e.path().stem().string();
        std::string name = gfot::language_name(loc);
        if (name == loc || name.empty() || !utf8(name))
            check(false, loc + " has its own name, in UTF-8 (got \"" + name + "\")");
    }
    check(files >= 40, "lang/ has PicoTorrent's 40 files (" + std::to_string(files) + ")");
    check(fails == 0, "every lang/*.json has its own name, in UTF-8");
    check(gfot::language_name("de-DE") == "Deutsch", "de-DE is Deutsch");
    check(gfot::language_name("sv-SE") == "Svenska", "sv-SE is Svenska");
    check(gfot::language_name("fr-FR") == "Fran\xc3\xa7" "ais", "fr-FR is Fran\xc3\xa7" "ais");
    check(gfot::language_name("de") == "Deutsch", "de (no country) finds de-DE");
    check(gfot::language_name("de-AT") == "Deutsch", "de-AT finds de-DE");
    check(gfot::language_name("pt-PT") == "Portugu\xc3\xaas", "pt-PT is its own row, not pt-BR's");
    check(gfot::language_name("xx-YY") == "xx-YY", "an unknown locale is given back");
    check(gfot::language_name("") == "", "an empty locale is given back");
    check(gfot::language_name("e") == "e", "a part of a language does not match (e is not en)");
    check(!utf8("\xc0\xaf") && !utf8("\xed\xa0\x80") && !utf8("\xe2\x82"), "the UTF-8 check refuses bad input");
    std::printf("lang_test: %d failed\n", fails);
    return fails != 0;
}
