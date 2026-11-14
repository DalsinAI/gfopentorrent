// GFOpenTorrent: wait for something by polling the one loop (gfot_loop.hpp).
// On one task, a blocking wait stops the loop that would bring what is
// waited for, so instead: poll, look, nap, up to a limit.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

namespace gfot::loop
{
    // poll(), then ready(); nap() between tries; at most tries polls.
    // true as soon as ready() is.
    template <class Poll, class Ready, class Nap>
    bool poll_until(Poll poll, Ready ready, Nap nap, int tries)
    {
        for (int i = 0; i < tries; i++)
        {
            poll();
            if (ready()) return true;
            if (i + 1 < tries) nap();
        }
        return false;
    }

    // How many 20 ms naps (one AmigaDOS Delay(1)) make ms; at least 1.
    constexpr int naps_for(int ms) { return ms <= 20 ? 1 : (ms + 19) / 20; }
}
