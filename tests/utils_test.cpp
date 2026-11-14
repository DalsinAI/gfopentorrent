// GFOpenTorrent host test: PicoTorrent's Utils for AmigaOS
// (compat/picotorrent/utils_amiga.cpp) and the AmigaDOS path rules in
// gfot_env.cpp that it and patches/picotorrent/0001 use. One line a check,
// "utils_test: N failed" at the end.
// Copyright (c) 2026 AmiCradle Team. MIT licence (see LICENSE).
#include "picotorrent/core/utils.hpp"
#include "gfot_env.hpp"
#include <cstdio>
#include <string>

using pt::Utils;
static int failed = 0;
static void check(const char *what, bool ok, const std::string &detail = std::string())
{
    std::printf("%s %s%s%s\n", ok ? "PASS" : "FAIL", what, detail.empty() ? "" : ": ", detail.c_str());
    std::fflush(stdout);
    if (!ok) failed++;
}

int main()
{
    using namespace gfot::env;
    // the joins patch 0001 makes: a download drawer at a volume's root
    check("save path at a volume root", join("Work:", "Ubuntu.iso") == "Work:Ubuntu.iso", join("Work:", "Ubuntu.iso"));
    check("save path in a drawer", join("Work:Downloads", "Ubuntu") == "Work:Downloads/Ubuntu");

    check("parent in a drawer", parent("Work:Downloads/Ubuntu.iso") == "Work:Downloads", parent("Work:Downloads/Ubuntu.iso"));
    check("parent at the root", parent("Work:Ubuntu.iso") == "Work:", parent("Work:Ubuntu.iso"));
    check("parent of a volume is itself", parent("Work:") == "Work:");
    check("parent past a trailing slash", parent("Work:a/b/") == "Work:a", parent("Work:a/b/"));
    check("parent of a bare name is the current drawer", parent("x") == "");
    check("parent of a relative path", parent("a/b") == "a");

    check("0 bytes", human_size(0) == "0 bytes", human_size(0));
    check("532 bytes", human_size(532) == "532 bytes");
    check("1023 bytes", human_size(1023) == "1023 bytes");
    check("1024 is 1.00 KB", human_size(1024) == "1.00 KB", human_size(1024));
    check("1490 is 1.45 KB (cut, not 1.46)", human_size(1490) == "1.45 KB", human_size(1490));
    check("14.5 MB", human_size(15204352) == "14.5 MB", human_size(15204352));
    check("145 GB", human_size(155692564480LL) == "145 GB", human_size(155692564480LL));
    check("1023 KB stays KB", human_size(1048575) == "1023 KB", human_size(1048575));
    check("2 TB", human_size(2199023255552LL) == "2.00 TB", human_size(2199023255552LL));
    check("negative is 0 bytes", human_size(-5) == "0 bytes");
    check("toHumanFileSize is the same text", Utils::toHumanFileSize(1490) == L"1.45 KB");

    const std::string u = "fran\xc3\xa7" "ais \xe2\x82\xac \xf0\x9f\x8e\xb5";
    std::wstring w = Utils::toStdWString(u);
    check("UTF-8 to wide: no trailing NUL", !w.empty() && w.back() != L'\0');
    check("UTF-8 to wide: c cedilla, euro, emoji", w.size() == (sizeof(wchar_t) == 2 ? 13u : 12u) && w[4] == wchar_t(0xe7) && w[9] == wchar_t(0x20ac));
    check("wide to UTF-8 and back", Utils::toStdString(w) == u);
    check("a bad byte is U+FFFD", Utils::toStdWString("a\xff" "b") == std::wstring(L"a\xfffd" L"b"));
    check("a cut sequence is U+FFFD", Utils::toStdWString("a\xe2\x82") == std::wstring(L"a\xfffd"));
    check("an overlong sequence is U+FFFD", Utils::toStdWString("\xc0\xaf") == std::wstring(L"\xfffd\xfffd"));
    check("a surrogate in UTF-8 is U+FFFD", Utils::toStdWString("\xed\xa0\x80") == std::wstring(L"\xfffd"));
    check("empty both ways", Utils::toStdWString("").empty() && Utils::toStdString(L"").empty());

    Utils::openAndSelect(std::filesystem::path("Work:Downloads/Ubuntu.iso"));   // host: logs the drawer

    std::printf("utils_test: %d failed\n", failed);
    return failed ? 1 : 0;
}
