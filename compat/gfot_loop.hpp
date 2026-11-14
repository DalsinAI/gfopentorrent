// GFOpenTorrent: the one loop. A bsdsocket.library base belongs to the task
// that opened it, so libtorrent's sockets, timers, disk work and the UI all
// run on one task and one Boost.Asio io_context (README, "One task"):
// - main() runs wxEntry inside io().run(), so a session call from the UI is
//   made inside the loop and libtorrent answers it there and then;
// - while the UI waits for Intuition, a wx timer polls the io_context
//   (nested poll() is allowed), so sockets and timers keep going;
// - nothing waits for libtorrent: wait_for_alert() polls instead.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

#include <libtorrent/fwd.hpp>

#include "gfot_poll.hpp"

namespace boost::asio
{
    class io_context;
}

namespace gfot::loop
{
    boost::asio::io_context& io();

    // body(argc, argv) inside io().run(); the loop ends when body returns
    int run_inside(int (*body)(int&, char**), int argc, char** argv);

    // the wx timer that polls io() every ms milliseconds (once; again is a
    // no-op), and its end
    void start_pump(int ms = 20);
    void stop_pump();

    // the session's next alert, polling io() up to ms milliseconds
    libtorrent::alert const* wait_for_alert(libtorrent::session_handle& s, int ms);
}
