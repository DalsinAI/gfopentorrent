// GFOpenTorrent: a finished HTTP request, as PicoTorrent's callback sees it
// (gfot_http.hpp).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_http.hpp"

gfot::http::outcome gfot::http::on_state(state s, int http_status)
{
    switch (s)
    {
    case state::completed:
        // any status the server sent, 404 too: the plugin decides
        return { true, http_status, true };
    case state::failed:
    case state::cancelled:
    case state::unauthorized:
        // no answer to give: as WinHTTP's client, status 0 and no body
        // (an unauthorized request is not retried with credentials)
        return { true, 0, false };
    case state::idle:
    case state::active:
    default:
        return { false, 0, false };
    }
}
