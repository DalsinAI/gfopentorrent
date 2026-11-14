// GFOpenTorrent: what the Amiga link needs. The file picker's pattern list
// (compat/gfot_filedlg.hpp); and, given the repository root as argv[1] and
// PicoTorrent's src/picotorrent drawer as argv[2], that the Amiga file
// picker and crash stub define what PicoTorrent's headers declare, and that
// the stove script compiles with libtorrent's defines, builds the stubs and
// keeps one select().
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_filedlg.hpp"

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

static bool has(std::string const& s, char const* what) { return s.find(what) != std::string::npos; }

int main(int argc, char** argv)
{
    using gfot::filedlg::wildcard;
    using gfot::filedlg::pattern;
    check(wildcard({ { L"Torrent files", L"*.torrent" }, { L"All files (*.*)", L"*.*" } })
        == L"Torrent files|*.torrent|All files (*.*)|*", "PicoTorrent's two types, all files as *");
    check(pattern(L"*.torrent;*.*") == L"*.torrent;*", "*.* inside a list becomes *");
    check(pattern(L"") == L"*", "an empty pattern shows every file");
    check(pattern(L";;*.torrent;") == L"*.torrent", "empty parts dropped");
    check(wildcard({}) == L"*|*", "no types: every file");
    check(wildcard({ { L"a|b", L"*.x" } }) == L"a b|*.x", "a | in a name does not split it");

    if (argc < 3)
    {
        std::printf("FAIL usage: link_stage_test REPO SRC/picotorrent\n");
        return 1;
    }
    std::string repo = argv[1], pt = argv[2];
    std::string hdr = slurp(pt + "/ui/win32/openfiledialog.hpp");
    std::string ofd = slurp(repo + "/compat/picotorrent/openfiledialog_amiga.cpp");
    check(!hdr.empty() && has(hdr, "IFileOpenDialog* m_wrappedDialog;"), "PicoTorrent's dialog header has the one member");
    char const* members[] = { "OpenFileDialog::OpenFileDialog()", "OpenFileDialog::~OpenFileDialog()",
        "OpenFileDialog::GetFiles(", "OpenFileDialog::SetFileTypes(", "OpenFileDialog::SetOption(",
        "OpenFileDialog::SetTitle(", "OpenFileDialog::Show(" };
    bool all = true;
    for (auto m : members) all = all && has(ofd, m);
    check(all, "the Amiga file picker defines every member");
    check(has(ofd, "wxFileDialog") && has(ofd, "wxFD_MULTIPLE") && !has(ofd, "windows.h") && !has(ofd, "shobjidl"),
        "the Amiga file picker is wx's own dialog, no Windows headers");
    std::string crh = slurp(pt + "/crashpadinitializer.hpp");
    std::string crs = slurp(repo + "/compat/picotorrent/crashpadinitializer_amiga.cpp");
    check(has(crh, "static void Initialize(std::shared_ptr<Core::Environment> env);")
        && has(crs, "pt::CrashpadInitializer::Initialize(std::shared_ptr<Core::Environment>)"),
        "the crash stub matches PicoTorrent's declaration");
    std::string posix = slurp(repo + "/compat/gfot_posix.c");
    check(has(posix, "int fchown(") && has(posix, "int fchmod("), "fchown and fchmod for SQLite");

    std::string sh = slurp(repo + "/tools/stove/build-picotorrent.sh");
    check(has(sh, "torrent-rasterbar.dir/flags.make") && has(sh, "$LT_DEFS\""),
        "PicoTorrent compiled with libtorrent's own defines");
    check(has(sh, "grep -v '^-DTORRENT_BUILDING_LIBRARY$'"), "less TORRENT_BUILDING_LIBRARY");
    check(has(sh, "openfiledialog_amiga crashpadinitializer_amiga") && has(sh, "gfot_posix.c"),
        "the stubs are compiled");
    check(has(sh, "grep 'amiga_select'") && has(sh, "ar d \"$O/libboost_asio.a\""),
        "Asio's select left out of a copy, the toolkit's kept");
    check(has(sh, "libtorrent-rasterbar.a\" \"$LT_BUILD\"/gfot_*.o") && has(sh, "\"$SQLITE_LIB\" $(\"$WX/bin/wx-config\" --libs std,net)"),
        "linked with libtorrent, its Amiga objects, SQLite and the toolkit (with its net library)");
    check(has(sh, "exit 4"), "a failed link exits 4 with the missing symbols");
    check(has(sh, "stoves/os32-gcc16-open8\"}") && !has(sh, "open6"),
        "built on the open8 stove (open6 -O2 -m68040 byte-store fault)");

    std::string disk = slurp(repo + "/compat/gfot_disk.cpp");
    std::size_t off = disk.find("me->pr_WindowPtr = (APTR)-1;");
    std::size_t lk = disk.find("Lock((CONST_STRPTR)path.c_str()");
    std::size_t back = disk.find("me->pr_WindowPtr = window;");
    check(off != std::string::npos && lk != std::string::npos && back != std::string::npos
        && off < lk && lk < back && back < disk.find("if (!lock)"),
        "free space: no insert-volume requester around the Lock, window put back before any return");
    std::string rexx = slurp(repo + "/compat/gfot_rexx.cpp");
    check(has(rexx, "Nothing else in the program may") && has(rexx, "must\n// open its own base"),
        "RexxSysBase: nothing else may use it, said where it is defined");

    std::printf("%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
