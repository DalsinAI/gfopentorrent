// GFOpenTorrent: the main window's first size (compat/gfot_window.hpp,
// patch 0013); given the patched src/picotorrent drawer as argv[1], that
// MainFrame uses it only when no saved size was restored.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_window.hpp"

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

int main(int argc, char** argv)
{
    using gfot::window::first_size;
    auto a = first_size(1920, 1056);
    check(a.w == 1152 && a.h == 633, "1920x1056 free: 1152x633");
    auto b = first_size(800, 580);
    check(b.w == 640 && b.h == 400, "a small screen: at least 640x400");
    auto c = first_size(640, 256);
    check(c.w == 640 && c.h == 256, "never more than the free area (PAL lores)");
    auto d = first_size(0, 0);
    check(d.w == 640 && d.h == 400, "no area known: 640x400");
    static_assert(gfot::window::fit(100, 640, 2000) == 640, "fit: the least");

    if (argc < 2)
    {
        std::printf("FAIL usage: window_size_test SRC/picotorrent\n");
        return 1;
    }
    std::ifstream in(std::string(argv[1]) + "/ui/mainframe.cpp", std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    std::string s = ss.str();
    size_t restore = s.find("if (!wxPersistenceManager::Get().RegisterAndRestore(this))");
    size_t first = s.find("gfot::window::first_size(area.width, area.height)");
    size_t windows = s.find("this->SetSize(FromDIP(wxSize(450, 400)));");
    check(restore != std::string::npos && first > restore && windows > first,
        "first size only when nothing was restored, Windows' size kept on its side");
    check(s.find("#include \"gfot_window.hpp\"") != std::string::npos, "mainframe includes gfot_window.hpp");

    std::printf("%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
