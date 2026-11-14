// GFOpenTorrent: PicoTorrent's "Add torrent" file picker on AmigaOS. The
// same class as src/picotorrent/ui/win32/openfiledialog.hpp, so
// mainframe.cpp is unchanged; behind it is wx's own wxFileDialog in place
// of Windows' IFileOpenDialog. The header's one member, an
// IFileOpenDialog pointer, holds the settings here.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "picotorrent/ui/win32/openfiledialog.hpp"

#include <wx/filedlg.h>

#include "gfot_filedlg.hpp"

struct IFileOpenDialog
{
    std::vector<std::tuple<std::wstring, std::wstring>> types;
    std::wstring title;
    bool multi = false;
    std::vector<std::wstring> files;
};

using pt::UI::Win32::OpenFileDialog;

OpenFileDialog::OpenFileDialog() : m_wrappedDialog(new IFileOpenDialog) {}

OpenFileDialog::~OpenFileDialog() { delete m_wrappedDialog; }

void OpenFileDialog::GetFiles(std::vector<std::wstring>& files) { files = m_wrappedDialog->files; }

void OpenFileDialog::SetFileTypes(std::vector<std::tuple<std::wstring, std::wstring>> const& types)
{
    m_wrappedDialog->types = types;
}

void OpenFileDialog::SetOption(Option opt)
{
    if (opt == Option::Multi) m_wrappedDialog->multi = true;
}

void OpenFileDialog::SetTitle(std::wstring const& title) { m_wrappedDialog->title = title; }

void OpenFileDialog::Show(wxWindow* parent)
{
    IFileOpenDialog& d = *m_wrappedDialog;
    d.files.clear();
    long style = wxFD_OPEN | wxFD_FILE_MUST_EXIST | (d.multi ? wxFD_MULTIPLE : 0);
    wxFileDialog dlg(parent, wxString(d.title), wxEmptyString, wxEmptyString,
        wxString(gfot::filedlg::wildcard(d.types)), style);
    if (dlg.ShowModal() != wxID_OK) return;
    wxArrayString paths;
    if (d.multi) dlg.GetPaths(paths); else paths.Add(dlg.GetPath());
    for (auto const& p : paths) d.files.push_back(p.ToStdWstring());
}
