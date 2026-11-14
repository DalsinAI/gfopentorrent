// GFOpenTorrent: what PicoTorrent's main window uses of Windows' tray icon
// and taskbar button, as nothing, for AmigaOS (the GFOpenWX Toolkit builds
// with wxUSE_TASKBARICON and wxUSE_TASKBARBUTTON 0). The tray settings read
// false there (gfot_settings.cpp), so none of this is ever asked to show.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

#include <wx/event.h>
#include <wx/icon.h>

class wxTaskBarIconEvent : public wxEvent
{
public:
    wxTaskBarIconEvent(wxEventType type = wxEVT_NULL) : wxEvent(wxID_ANY, type) { }
    wxEvent* Clone() const override { return new wxTaskBarIconEvent(*this); }
};
wxDECLARE_EVENT(wxEVT_TASKBAR_LEFT_DOWN, wxTaskBarIconEvent);

enum wxTaskBarButtonState
{
    wxTASKBAR_BUTTON_NO_PROGRESS = 0,
    wxTASKBAR_BUTTON_NORMAL = 2
};

class wxTaskBarButton
{
public:
    void Show(bool = true) { }
    void Hide() { }
    void SetProgressState(wxTaskBarButtonState) { }
    void SetProgressRange(int) { }
    void SetProgressValue(int) { }
};

namespace gfot
{
    // PicoTorrent's TaskBarIcon on a system with no tray: never installed
    class NoTaskBarIcon : public wxEvtHandler
    {
    public:
        explicit NoTaskBarIcon(wxWindow*) { }
        void Show() { }
        void Hide() { }
        bool IsIconInstalled() const { return false; }
        bool ShowBalloon(const wxString&, const wxString&, unsigned = 0, int = 0, const wxIcon& = wxNullIcon) { return false; }
    };
}
