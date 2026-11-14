// GFOpenTorrent: no Windows resources on AmigaOS (patch 0012). Given the
// patched src/picotorrent drawer as argv[1], no line the Amiga build
// compiles loads an icon or bitmap from resources.rc (wxICON(),
// *_RESOURCE); and amiga_lines() itself is checked on a small sample.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static int failed = 0;
static int passed = 0;

static void check(bool ok, std::string const& what)
{
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what.c_str());
    (ok ? passed : failed)++;
}

// The lines the Amiga build compiles: __WXOPENLAYOUT__ is defined, so the
// #else of an #ifdef __WXOPENLAYOUT__, and the body of an #ifndef, are out.
// Other conditionals are taken as compiled.
static std::vector<std::string> amiga_lines(std::string const& text)
{
    std::vector<std::string> out;
    std::vector<int> stack;   // 1 compiled, 0 not, per open #if
    std::istringstream in(text);
    std::string line;
    auto live = [&] { for (int v : stack) if (!v) return false; return true; };
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string t = line.substr(line.find_first_not_of(" \t") == std::string::npos ? line.size() : line.find_first_not_of(" \t"));
        if (t.rfind("#ifdef __WXOPENLAYOUT__", 0) == 0) { stack.push_back(1); continue; }
        if (t.rfind("#ifndef __WXOPENLAYOUT__", 0) == 0) { stack.push_back(0); continue; }
        if (t.rfind("#if", 0) == 0) { stack.push_back(2); continue; }
        if (t.rfind("#else", 0) == 0 && !stack.empty()) { if (stack.back() != 2) stack.back() ^= 1; continue; }
        if (t.rfind("#endif", 0) == 0 && !stack.empty()) { stack.pop_back(); continue; }
        if (live()) out.push_back(line);
    }
    return out;
}

static bool loads_resource(std::string const& l)
{
    return l.find("_RESOURCE") != std::string::npos || l.find("wxICON(") != std::string::npos;
}

int main(int argc, char** argv)
{
    auto a = amiga_lines("x\n#ifdef __WXOPENLAYOUT__\namiga\n#else\nwin\n#endif\n#ifndef __WXOPENLAYOUT__\nwin2\n#endif\n#if 1\nboth\n#endif\n");
    check(a == std::vector<std::string>{ "x", "amiga", "both" }, "amiga_lines keeps the Amiga side only");

    if (argc < 2)
    {
        std::printf("FAIL usage: no_resources_test SRC/picotorrent\n");
        return 1;
    }
    int files = 0;
    for (auto const& e : fs::recursive_directory_iterator(argv[1]))
    {
        if (e.path().extension() != ".cpp") continue;
        if (e.path().string().find("/win32/") != std::string::npos) continue;   // not built on AmigaOS
        std::ifstream in(e.path(), std::ios::binary);
        std::stringstream ss;
        ss << in.rdbuf();
        files++;
        for (auto const& l : amiga_lines(ss.str()))
            if (loads_resource(l))
                check(false, e.path().filename().string() + " loads a Windows resource: " + l);
    }
    check(files > 40, "looked at every source (" + std::to_string(files) + ")");
    check(failed == 0, "no Windows resource on the Amiga side");
    std::printf("%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
