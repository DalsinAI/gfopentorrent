// GFOpenTorrent: no query filter (PQL) on AmigaOS (patch 0009): the message a
// query gets (compat/gfot_pql.cpp), and, given the repository root as
// argv[1] and the patched src/picotorrent drawer as argv[2], that the
// Amiga filter needs no ANTLR and the console is hidden.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_pql.hpp"

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

int main(int argc, char** argv)
{
    check(gfot::pql::not_built("").empty(), "an empty query gets no error (it only clears the filter)");
    std::string m = gfot::pql::not_built("status = \"downloading\"");
    check(!m.empty(), "a query gets an error, so Console shows why");
    check(m.find("status = \"downloading\"") != std::string::npos, "the error names the query");
    check(m.find("Every torrent is shown") != std::string::npos, "and says the list is not filtered");

    if (argc > 2)
    {
        std::string amiga = slurp(std::string(argv[1]) + "/compat/picotorrent/pqltorrentfilter_amiga.cpp");
        std::string mf = slurp(std::string(argv[2]) + "/ui/mainframe.cpp");
        check(!amiga.empty() && !mf.empty(), "pqltorrentfilter_amiga.cpp and mainframe.cpp are there");
        check(amiga.find("antlr") == std::string::npos && amiga.find("Query") == std::string::npos,
            "the AmigaOS filter needs no ANTLR runtime or generated parser");
        check(amiga.find("return nullptr;") != std::string::npos, "Create gives no filter");
        auto a = mf.find("    m_console->Hide();\n#else");
        check(a != std::string::npos && mf.rfind("#ifdef __WXOPENLAYOUT__", a) != std::string::npos,
            "mainframe: the console is always hidden on AmigaOS");
        check(mf.find("m_menuItemConsoleInput->Enable(false);") != std::string::npos, "mainframe: and its menu item is off");
    }

    std::printf("no_pql_test: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
