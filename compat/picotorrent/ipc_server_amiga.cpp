// GFOpenTorrent: PicoTorrent's ipc/server.cpp for AmigaOS. The first copy's
// ARexx port (GFOPENTORRENT.1) takes what a second start was given (see
// compat/gfot_rexx.hpp), where Windows has a DDE server; a script can use
// the same commands. What a command gives goes to the main window as the
// Windows connection's JSON did (ipc/applicationoptionsconnection.cpp).
// Copyright (c) 2026 AmiCradle Team. MIT licence (see LICENSE).
#include "server.hpp"

#include <boost/log/trivial.hpp>
#include <wx/openlayout/arexx.h>

#include "../applicationoptions.hpp"
#include "../core/utils.hpp"
#include "../ui/mainframe.hpp"
#include "gfot_rexx.hpp"

using pt::IPC::Server;

namespace
{
    // ADD's and MAGNET's template: the list, then where to save, and no dialog
    const char* const add_template_files = "FILE/M/A,SAVEPATH/K,SILENT/S";
    const char* const add_template_links = "LINK/M/A,SAVEPATH/K,SILENT/S";
}

Server::Server(pt::UI::MainFrame* mainFrame)
    : m_frame(mainFrame),
    m_rexx(std::make_unique<wxARexxPort>(gfot::rexx::port_base, "GFOpenTorrent"))
{
    auto add = [this](bool magnets)
    {
        return [this, magnets](wxARexxCommand& cmd)
        {
            pt::CommandLineOptions options;
            for (auto const& s : cmd.Multi(0))
            {
                std::string arg = pt::Utils::toStdString(s.ToStdWstring());
                (magnets ? options.magnets : options.files).push_back(arg);
            }
            options.save_path = pt::Utils::toStdString(cmd.Str(1).ToStdWstring());
            options.silent = cmd.Switch(2);

            m_frame->GetEventHandler()->CallAfter([this, options]()
                {
                    if (m_frame->IsIconized())
                    {
                        m_frame->Restore();
                    }

                    m_frame->Raise();
                    m_frame->Show();
                    m_frame->HandleParams(options);
                });
        };
    };

    m_rexx->Add("ADD", add_template_files, add(false));
    m_rexx->Add("MAGNET", add_template_links, add(true));

    if (!m_rexx->Open())
    {
        BOOST_LOG_TRIVIAL(warning) << "No ARexx port: a second start can't hand its torrents over";
    }
    else
    {
        BOOST_LOG_TRIVIAL(info) << "ARexx port " << m_rexx->GetPortName().ToStdString();
    }
}

Server::~Server()
{
}
