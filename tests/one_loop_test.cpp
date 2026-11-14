// GFOpenTorrent: the one loop (patch 0011). poll_until's counting
// (compat/gfot_poll.hpp), and, given the repository root as argv[1] and the
// patched src/picotorrent drawer as argv[2], that the program runs inside
// the loop, the session is made on it with no threads, and nothing in the
// Amiga build waits for libtorrent.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_poll.hpp"

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
    std::string s = ss.str(), out;
    for (char c : s) if (c != '\r') out += c;
    return out;
}

static bool has(std::string const& s, char const* what) { return s.find(what) != std::string::npos; }

// the text between #ifdef __WXOPENLAYOUT__ and its #else (the Amiga side)
static std::string amiga_side(std::string const& s, char const* near)
{
    size_t at = s.find(near);
    if (at == std::string::npos) return "";
    size_t start = s.rfind("#ifdef __WXOPENLAYOUT__", at);
    size_t end = s.find("#else", at);
    if (start == std::string::npos || end == std::string::npos) return "";
    return s.substr(start, end - start);
}

int main(int argc, char** argv)
{
    using gfot::loop::poll_until;
    int polls = 0, naps = 0;
    check(poll_until([&] { polls++; }, [&] { return polls == 3; }, [&] { naps++; }, 10) && polls == 3 && naps == 2,
        "ready on the third poll: 3 polls, 2 naps");
    polls = naps = 0;
    check(!poll_until([&] { polls++; }, [] { return false; }, [&] { naps++; }, 5) && polls == 5 && naps == 4,
        "never ready: the limit's polls, no nap after the last");
    polls = naps = 0;
    check(poll_until([&] { polls++; }, [] { return true; }, [&] { naps++; }, 5) && polls == 1 && naps == 0,
        "ready at once: one poll, no nap");
    polls = 0;
    check(!poll_until([&] { polls++; }, [] { return true; }, [] {}, 0) && polls == 0, "no tries: nothing polled");
    check(gfot::loop::naps_for(10000) == 500, "10 s is 500 naps of 20 ms");
    check(gfot::loop::naps_for(0) == 1 && gfot::loop::naps_for(21) == 2, "at least one nap; part naps round up");

    if (argc < 3)
    {
        std::printf("FAIL usage: one_loop_test REPO SRC/picotorrent\n");
        return 1;
    }
    std::string repo = argv[1], pt = argv[2];
    std::string main_cpp = slurp(pt + "/main.cpp");
    check(has(main_cpp, "wxIMPLEMENT_APP_NO_MAIN(pt::Application);")
        && has(main_cpp, "gfot::loop::run_inside(&wxEntry, argc, argv)"), "main() runs wxEntry inside the loop");
    std::string ses = slurp(pt + "/bittorrent/session.cpp");
    std::string mk = amiga_side(ses, "std::make_unique<lt::session>(sp, gfot::loop::io())");
    check(!mk.empty(), "the session is made on the program's io_context");
    check(has(mk, "hashing_threads, 0") && has(mk, "aio_threads, 0") && has(mk, "posix_disk_io_constructor"),
        "no hashing or disk threads: posix disk I/O on the loop");
    check(has(mk, "gfot::loop::start_pump()"), "the pump starts with the session");
    std::string wait = amiga_side(ses, "gfot::loop::wait_for_alert(*m_session, 10000)");
    check(!wait.empty() && !has(wait, "m_session->wait_for_alert(lt::seconds(10))"),
        "saving at the end polls for alerts instead of waiting");
    check(has(amiga_side(ses, "gfot::loop::stop_pump()"), "stop_pump"), "the pump stops with the session");
    std::string loop = slurp(repo + "/compat/gfot_loop.cpp");
    check(has(loop, "io().stop();") && has(loop, "io().run();"), "the loop ends when the program does");
    check(has(loop, "wait_for_alert(libtorrent::seconds(0))"), "an alert is looked for, never waited for");
    std::string sh = slurp(repo + "/tools/stove/build-picotorrent.sh");
    check(has(sh, " gfot_loop"), "the stove build compiles gfot_loop");

    std::printf("%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
