// GFOpenTorrent: a language's own name for PicoTorrent's language list,
// in place of Windows' GetLocaleInfoEx(LOCALE_SLOCALIZEDLANGUAGENAME).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

#include <string>

namespace gfot
{
    // The language's name in its own language, UTF-8, for a PicoTorrent
    // locale such as "de-DE" or "de". Unknown: the locale itself.
    std::string language_name(std::string const& locale);
}
