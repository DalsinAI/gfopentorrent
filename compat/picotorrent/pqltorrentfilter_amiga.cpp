// GFOpenTorrent: PicoTorrent's query filter (PQL) is not in the AmigaOS
// build: it needs the ANTLR4 C++ runtime, which is not in the tree
// (teams/team5/inbox QUESTION picotorrent-pql, option A). Copied over
// src/picotorrent/ui/filters/pqltorrentfilter.cpp by
// tools/stove/build-picotorrent.sh. The console that takes queries is
// hidden on AmigaOS (patch 0009); a saved query that reaches Create gets
// no filter and says why.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "pqltorrentfilter.hpp"

#include "gfot_pql.hpp"

using pt::BitTorrent::TorrentHandle;
using pt::BitTorrent::TorrentStatus;
using pt::UI::Filters::PqlTorrentFilter;
using pt::UI::Filters::TorrentFilter;

std::unique_ptr<TorrentFilter> PqlTorrentFilter::Create(std::string const& input, std::string* error)
{
    if (error)
    {
        *error = gfot::pql::not_built(input);
    }
    return nullptr;
}

PqlTorrentFilter::PqlTorrentFilter(std::function<bool(TorrentStatus const&)> const& filter)
    : m_filter(filter)
{
}

PqlTorrentFilter::~PqlTorrentFilter()
{
}

bool PqlTorrentFilter::Includes(TorrentHandle const&)
{
    // never made (Create gives none); every torrent stays in the list
    return true;
}
