// GFOpenTorrent: the listen-interface dialog's adapter list, from
// libtorrent's enum_net_interfaces (which reads the TCP/IP stack's
// interfaces on AmigaOS) in place of Windows' GetAdaptersAddresses. As on
// Windows: loopback and interfaces that are down are left out, an IPv4
// address shows as "a.b.c.d (name)", an IPv6 one as "[addr] (name)".
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

#include <string>
#include <vector>

namespace gfot
{
namespace netif
{
    // one address of an interface, as enum_net_interfaces gives it
    struct iface
    {
        std::string address;        // without brackets
        bool v6 = false;
        bool up = true;
        bool loopback = false;
        std::string name;           // the device name (e.g. "eth0")
        std::string friendly_name;  // shown in place of name when set
    };

    // one line of the dialog's list
    struct adapter
    {
        std::string address;   // what the setting stores ("[addr]" for IPv6)
        std::string name;      // what the list shows
    };

    std::vector<adapter> adapters(std::vector<iface> const& ifs);
}
}
