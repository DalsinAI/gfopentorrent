// GFOpenTorrent: the one loop (see gfot_loop.hpp).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_loop.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <libtorrent/session_handle.hpp>
#include <wx/timer.h>

#include <proto/dos.h>

boost::asio::io_context& gfot::loop::io()
{
    static boost::asio::io_context ctx;
    return ctx;
}

int gfot::loop::run_inside(int (*body)(int&, char**), int argc, char** argv)
{
    int rc = 1;
    boost::asio::post(io(), [&]
        {
            rc = body(argc, argv);
            io().stop();
        });
    io().run();
    return rc;
}

namespace
{
    class Pump : public wxTimer
    {
    public:
        void Notify() override { gfot::loop::io().poll(); }
    };

    Pump* pump = nullptr;
}

void gfot::loop::start_pump(int ms)
{
    if (pump) return;
    pump = new Pump;
    pump->Start(ms, wxTIMER_CONTINUOUS);
}

void gfot::loop::stop_pump()
{
    if (!pump) return;
    pump->Stop();
    delete pump;
    pump = nullptr;
}

libtorrent::alert const* gfot::loop::wait_for_alert(libtorrent::session_handle& s, int ms)
{
    libtorrent::alert const* got = nullptr;
    poll_until(
        [] { io().poll(); },
        [&] { return (got = s.wait_for_alert(libtorrent::seconds(0))) != nullptr; },
        [] { Delay(1); },
        naps_for(ms));
    return got;
}
