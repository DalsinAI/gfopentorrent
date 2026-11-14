// GFOpenTorrent: the query filter's message on AmigaOS (gfot_pql.hpp).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_pql.hpp"

std::string gfot::pql::not_built(std::string const& query)
{
    if (query.empty())
    {
        return std::string();
    }
    return "Query filters are not in this build of GFOpenTorrent, so \""
        + query + "\" was not applied. Every torrent is shown.";
}
