// GFOpenTorrent: how a finished HTTP request reaches PicoTorrent's
// HttpClient callback (compat/picotorrent/httpclient_amiga.cpp, on the
// toolkit's wxWebRequest). The WinHTTP client gives the server's status
// and body when the request completes, and status 0 with no body when it
// fails; this keeps that.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

#include <string>

namespace gfot
{
namespace http
{
    // wxWebRequest::State, as far as the callback cares
    enum class state
    {
        idle,
        unauthorized,
        active,
        completed,
        failed,
        cancelled
    };

    struct outcome
    {
        bool done;    // call the callback now, once
        int status;   // what the callback gets as its status
        bool body;    // whether it gets the response's body
    };

    outcome on_state(state s, int http_status);
}
}
