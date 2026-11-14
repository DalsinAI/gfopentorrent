// GFOpenTorrent: what a plugin's HTTP request gives back (compat/gfot_http.cpp)
// and that libpico on AmigaOS keeps Windows' DLL code to Windows (patch 0006,
// libpico.cpp given as argv[1]).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_http.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using gfot::http::on_state;
using gfot::http::state;

static int failed = 0;
static int passed = 0;

static void check(bool ok, char const* what)
{
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    (ok ? passed : failed)++;
}

int main(int argc, char** argv)
{
    auto c = on_state(state::completed, 200);
    check(c.done && c.status == 200 && c.body, "a completed request gives its status and body");
    auto nf = on_state(state::completed, 404);
    check(nf.done && nf.status == 404 && nf.body, "a 404 is still an answer: status 404 and its body");
    auto f = on_state(state::failed, 0);
    check(f.done && f.status == 0 && !f.body, "a failed request gives status 0, no body (as WinHTTP's client)");
    auto x = on_state(state::cancelled, 200);
    check(x.done && x.status == 0 && !x.body, "a cancelled request gives status 0");
    auto u = on_state(state::unauthorized, 401);
    check(u.done && u.status == 0 && !u.body, "an unauthorized request ends, status 0 (no credentials to try)");
    check(!on_state(state::active, 0).done, "an active request waits");
    check(!on_state(state::idle, 0).done, "an idle request waits");

    if (argc > 1)
    {
        std::ifstream in(argv[1], std::ios::binary);
        std::stringstream ss;
        ss << in.rdbuf();
        std::string s = ss.str();
        auto amiga = s.find("#ifdef __WXOPENLAYOUT__");
        auto win = s.find("#include <Windows.h>");
        auto load = s.find("LoadLibrary(");
        check(amiga != std::string::npos && win != std::string::npos && amiga < win, "libpico.cpp: Windows.h only past the AmigaOS branch");
        check(load != std::string::npos && s.rfind("#else", load) != std::string::npos
            && s.rfind("return nullptr;", load) != std::string::npos && s.rfind("return nullptr;", load) > s.rfind("IPlugin* IPlugin::Load", load),
            "libpico.cpp: IPlugin::Load gives no plugin on AmigaOS");
    }

    std::printf("http_plugin_test: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
