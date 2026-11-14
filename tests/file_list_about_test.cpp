// GFOpenTorrent: the file list and the About box on AmigaOS (patch 0008),
// checked in the patched tree (its src/picotorrent/ui drawer as argv[1]):
// the shell's icons and OpenSSL stay on Windows, the toolkit's icons are
// asked for, and the About box lists no OpenSSL.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
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

// each place the text occurs is in the #else of an #ifdef __WXOPENLAYOUT__
static bool windows_only(std::string const& s, std::string const& text)
{
    for (auto h = s.find(text); h != std::string::npos; h = s.find(text, h + 1))
    {
        auto amiga = s.rfind("#ifdef __WXOPENLAYOUT__", h);
        auto els = s.rfind("#else", h);
        auto end = s.rfind("#endif", h);
        if (amiga == std::string::npos || els == std::string::npos || els < amiga
            || (end != std::string::npos && end > amiga))
        {
            return false;
        }
    }
    return true;
}

// the text sits in an #ifdef __WXOPENLAYOUT__ branch, before its #else
static bool amiga_only(std::string const& s, std::string const& text)
{
    auto h = s.find(text);
    if (h == std::string::npos)
    {
        return false;
    }
    auto amiga = s.rfind("#ifdef __WXOPENLAYOUT__", h);
    auto els = s.rfind("#else", h);
    return amiga != std::string::npos && (els == std::string::npos || els < amiga);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::printf("usage: file_list_about_test PICOTORRENT/src/picotorrent/ui\n");
        return 2;
    }
    std::string ui = argv[1];
    std::string fs = slurp(ui + "/models/filestoragemodel.cpp");
    std::string ab = slurp(ui + "/dialogs/aboutdialog.cpp");
    check(!fs.empty() && !ab.empty(), "filestoragemodel.cpp and aboutdialog.cpp are there");

    check(windows_only(fs, "<shellapi.h>"), "file list: shellapi.h only on Windows");
    check(windows_only(fs, "SHGetFileInfo("), "file list: every SHGetFileInfo call only on Windows");
    check(windows_only(fs, "SHGetStockIconInfo("), "file list: the stock icon only on Windows");
    check(windows_only(fs, "CreateFromHICON("), "file list: no HICON on AmigaOS");
    check(amiga_only(fs, "wxArtProvider::GetIcon(wxART_FOLDER"), "file list: the folder icon is the toolkit's");
    check(amiga_only(fs, "wxArtProvider::GetIcon(wxART_NORMAL_FILE"), "file list: the file icon is the toolkit's");
    check(fs.find("return UnknownIcon;") != std::string::npos, "file list: an extension with no icon falls back to the file icon");

    check(windows_only(ab, "<openssl/opensslv.h>"), "About: OpenSSL's header only on Windows");
    check(windows_only(ab, "OPENSSL_VERSION_TEXT"), "About: no OpenSSL row on AmigaOS");
    check(ab.find("LIBTORRENT_VERSION") != std::string::npos, "About: libtorrent's row stays");

    std::printf("file_list_about_test: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
