// GFOpenTorrent: what PicoTorrent's query filter says on AmigaOS, where the
// query language (PQL, on the ANTLR4 runtime) is not built
// (compat/picotorrent/pqltorrentfilter_amiga.cpp).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

#include <string>

namespace gfot
{
namespace pql
{
    // the error Console shows for a query (empty for an empty query, which
    // only clears the filter and never reaches the parser)
    std::string not_built(std::string const& query);
}
}
