// GFOpenTorrent: language names for PicoTorrent's lang/*.json locales.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_lang.hpp"

#include <cstring>

namespace
{
    struct Name { const char* locale; const char* name; };

    // One row for each of PicoTorrent v0.25.0's lang/*.json files, in the
    // order ls gives them; the names are the languages' own, UTF-8.
    const Name names[] = {
        { "af-ZA", "Afrikaans" },
        { "ar-SA", "\xd8\xa7\xd9\x84\xd8\xb9\xd8\xb1\xd8\xa8\xd9\x8a\xd8\xa9" },
        { "bg-BG", "\xd0\xb1\xd1\x8a\xd0\xbb\xd0\xb3\xd0\xb0\xd1\x80\xd1\x81\xd0\xba\xd0\xb8" },
        { "ca-ES", "Catal\xc3\xa0" },
        { "cs-CZ", "\xc4\x8c" "e\xc5\xa1tina" },
        { "da-DK", "Dansk" },
        { "de-DE", "Deutsch" },
        { "el-GR", "\xce\x95\xce\xbb\xce\xbb\xce\xb7\xce\xbd\xce\xb9\xce\xba\xce\xac" },
        { "en-US", "English" },
        { "es-ES", "Espa\xc3\xb1ol" },
        { "fi-FI", "Suomi" },
        { "fr-FR", "Fran\xc3\xa7" "ais" },
        { "he-IL", "\xd7\xa2\xd7\x91\xd7\xa8\xd7\x99\xd7\xaa" },
        { "hi-IN", "\xe0\xa4\xb9\xe0\xa4\xbf\xe0\xa4\xa8\xe0\xa5\x8d\xe0\xa4\xa6\xe0\xa5\x80" },
        { "hr-HR", "Hrvatski" },
        { "hu-HU", "Magyar" },
        { "hy-AM", "\xd5\x80\xd5\xa1\xd5\xb5\xd5\xa5\xd6\x80\xd5\xa5\xd5\xb6" },
        { "id-ID", "Bahasa Indonesia" },
        { "it-IT", "Italiano" },
        { "ja-JP", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e" },
        { "ka-GE", "\xe1\x83\xa5\xe1\x83\x90\xe1\x83\xa0\xe1\x83\x97\xe1\x83\xa3\xe1\x83\x9a\xe1\x83\x98" },
        { "ko-KR", "\xed\x95\x9c\xea\xb5\xad\xec\x96\xb4" },
        { "lt-LT", "Lietuvi\xc5\xb3" },
        { "lv-LV", "Latvie\xc5\xa1u" },
        { "nb-NO", "Norsk bokm\xc3\xa5l" },
        { "nl-NL", "Nederlands" },
        { "pl-PL", "Polski" },
        { "pt-BR", "Portugu\xc3\xaas (Brasil)" },
        { "pt-PT", "Portugu\xc3\xaas" },
        { "ro-RO", "Rom\xc3\xa2n\xc4\x83" },
        { "ru-RU", "\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9" },
        { "si-LK", "\xe0\xb7\x83\xe0\xb7\x92\xe0\xb6\x82\xe0\xb7\x84\xe0\xb6\xbd" },
        { "sk-SK", "Sloven\xc4\x8dina" },
        { "sr-SP", "\xd0\xa1\xd1\x80\xd0\xbf\xd1\x81\xd0\xba\xd0\xb8" },
        { "sv-SE", "Svenska" },
        { "tr-TR", "T\xc3\xbcrk\xc3\xa7" "e" },
        { "uk-UA", "\xd0\xa3\xd0\xba\xd1\x80\xd0\xb0\xd1\x97\xd0\xbd\xd1\x81\xd1\x8c\xd0\xba\xd0\xb0" },
        { "vi-VN", "Ti\xe1\xba\xbfng Vi\xe1\xbb\x87t" },
        { "zh-CN", "\xe7\xae\x80\xe4\xbd\x93\xe4\xb8\xad\xe6\x96\x87" },
        { "zh-TW", "\xe7\xb9\x81\xe9\xab\x94\xe4\xb8\xad\xe6\x96\x87" },
    };
}

std::string gfot::language_name(std::string const& locale)
{
    // an exact locale first, then the first row with the same language
    for (auto const& n : names)
        if (locale == n.locale) return n.name;
    std::string lang = locale.substr(0, locale.find('-'));
    if (!lang.empty())
        for (auto const& n : names)
            if (std::strncmp(n.locale, lang.c_str(), lang.size()) == 0 && n.locale[lang.size()] == '-')
                return n.name;
    return locale;
}
