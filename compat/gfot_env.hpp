// GFOpenTorrent: the AmigaOS rules behind PicoTorrent's Environment
// (compat/picotorrent/environment_amiga.cpp), kept apart so the host tests
// can check them.
// Copyright (c) 2026 AmiCradle Team. MIT licence (see LICENSE).
#ifndef GFOT_ENV_HPP
#define GFOT_ENV_HPP
#include <ctime>
#include <string>

namespace gfot { namespace env {
// AmigaDOS joins: "PROGDIR:" + "logs" is "PROGDIR:logs", "Work:a" + "b" is
// "Work:a/b". (std::filesystem's "/" would give "PROGDIR:/logs", and a "/"
// after the colon means the parent directory.)
std::string join(const std::string &dir, const std::string &name);
// the drawer holding a path: "Work:a/b" is "Work:a", "Work:a" is "Work:",
// "Work:" stays "Work:", "a" is "" (the current drawer)
std::string parent(const std::string &path);
// a size as Windows' StrFormatByteSize64 writes it, which PicoTorrent shows:
// "532 bytes", "1.45 KB", "14.5 MB", "145 GB" (three figures, cut, not rounded)
std::string human_size(long long bytes);
// UTF-8 and wide strings, for PicoTorrent's Utils (a bad UTF-8 byte is U+FFFD)
std::wstring utf8_to_wide(const std::string &s);
std::string wide_to_utf8(const std::wstring &w);
// "PicoTorrent.20261113215500.log", as the Windows build names it
std::string log_file_name(std::time_t t);
// locale.library's language name ("deutsch", "français.language", ...) to
// the code PicoTorrent's translations use ("de"); "en" when unknown
std::string language_code(const std::string &amiga_language);
}}
#endif
