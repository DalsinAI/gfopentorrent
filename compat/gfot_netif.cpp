// GFOpenTorrent: the listen-interface dialog's adapter list (gfot_netif.hpp).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_netif.hpp"

std::vector<gfot::netif::adapter> gfot::netif::adapters(std::vector<iface> const& ifs)
{
    std::vector<adapter> out;
    for (iface const& i : ifs)
    {
        if (i.loopback || !i.up || i.address.empty())
        {
            continue;
        }
        std::string shown = i.friendly_name.empty() ? i.name : i.friendly_name;
        adapter a;
        a.address = i.v6 ? "[" + i.address + "]" : i.address;
        a.name = a.address + " (" + shown + ")";
        out.push_back(a);
    }
    return out;
}
