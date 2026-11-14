// GFOpenTorrent: PicoTorrent's HttpClient on the toolkit's wxWebRequest
// (libcurl), in place of WinHTTP. Copied over src/picotorrent/http/
// httpclient.cpp by tools/stove/build-picotorrent.sh; the class is
// declared in http/httpclient.hpp (patch 0006).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "httpclient.hpp"

#include <wx/app.h>
#include <wx/sstream.h>
#include <wx/stream.h>
#include <wx/webrequest.h>

#include "gfot_http.hpp"

using pt::Http::HttpClient;

struct HttpClient::Pending
{
    wxWebRequest request;
    std::function<void(int, std::string const&)> callback;
};

static gfot::http::state to_state(wxWebRequest::State s)
{
    switch (s)
    {
    case wxWebRequest::State_Idle: return gfot::http::state::idle;
    case wxWebRequest::State_Unauthorized: return gfot::http::state::unauthorized;
    case wxWebRequest::State_Active: return gfot::http::state::active;
    case wxWebRequest::State_Completed: return gfot::http::state::completed;
    case wxWebRequest::State_Failed: return gfot::http::state::failed;
    case wxWebRequest::State_Cancelled: return gfot::http::state::cancelled;
    }
    return gfot::http::state::failed;
}

// the body's bytes as they came, not decoded to text
static std::string body_of(wxWebResponse const& response)
{
    std::string body;
    wxInputStream* in = response.GetStream();
    if (!in)
    {
        return body;
    }
    char buf[4096];
    while (in->CanRead())
    {
        in->Read(buf, sizeof(buf));
        size_t n = in->LastRead();
        if (n == 0)
        {
            break;
        }
        body.append(buf, n);
    }
    return body;
}

HttpClient::HttpClient()
{
    // one handler for every request, keyed by its id
    this->Bind(wxEVT_WEBREQUEST_STATE, &HttpClient::OnRequestState, this);
}

HttpClient::~HttpClient()
{
    for (auto& p : m_pending)
    {
        p.second->request.Cancel();
    }
}

void HttpClient::Get(wxString const& url, std::function<void(int, std::string const&)> const& callback)
{
    int id = m_nextId++;
    auto pending = std::make_shared<Pending>();
    pending->callback = callback;
    pending->request = wxWebSession::GetDefault().CreateRequest(this, url, id);

    if (!pending->request.IsOk())
    {
        // as a failed request: status 0, no body, after Get returns
        auto cb = callback;
        wxTheApp->CallAfter([cb]() { cb(0, std::string()); });
        return;
    }

    m_pending[id] = pending;
    pending->request.Start();
}

void HttpClient::OnRequestState(wxWebRequestEvent& evt)
{
    auto it = m_pending.find(evt.GetId());
    if (it == m_pending.end())
    {
        return;
    }
    wxWebResponse const& response = evt.GetResponse();
    int status = response.IsOk() ? response.GetStatus() : 0;
    gfot::http::outcome o = gfot::http::on_state(to_state(evt.GetState()), status);
    if (!o.done)
    {
        return;
    }
    std::string body = o.body && response.IsOk() ? body_of(response) : std::string();
    auto cb = it->second->callback;
    m_pending.erase(it);
    // the callback can hold the last reference to this client
    // (libpico_http_get): call it after this event is done
    int st = o.status;
    wxTheApp->CallAfter([cb, st, body]() { cb(st, body); });
}
