// GFOpenTorrent: the Preferences pages on AmigaOS (patch 0007): the listen
// interface list (compat/gfot_netif.cpp), and, given the patched dialogs
// drawer as argv[1], that the pages keep Windows' headers to Windows.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_netif.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using gfot::netif::iface;

static int failed = 0;
static int passed = 0;

static void check(bool ok, char const* what)
{
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    (ok ? passed : failed)++;
}

static std::string slurp(std::string const& path)
{
    std::ifstream in(path, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// every Windows header the file names comes after an #ifdef __WXOPENLAYOUT__
// and inside its #else
static bool windows_only(std::string const& s, char const* header)
{
    auto h = s.find(header);
    if (h == std::string::npos)
    {
        return true;
    }
    auto amiga = s.rfind("#ifdef __WXOPENLAYOUT__", h);
    auto els = s.rfind("#else", h);
    auto end = s.rfind("#endif", h);
    return amiga != std::string::npos && els != std::string::npos && els > amiga
        && (end == std::string::npos || end < amiga);
}

int main(int argc, char** argv)
{
    iface lo;  lo.address = "127.0.0.1"; lo.loopback = true; lo.name = "lo0";
    iface eth; eth.address = "192.168.1.20"; eth.name = "eth0";
    iface down; down.address = "10.0.0.5"; down.up = false; down.name = "ppp0";
    iface v6; v6.address = "fe80::1"; v6.v6 = true; v6.name = "eth0"; v6.friendly_name = "A1200 Ethernet";
    iface blank; blank.name = "eth1";

    auto a = gfot::netif::adapters({ lo, eth, down, v6, blank });
    check(a.size() == 2, "loopback, a down interface and one with no address are left out");
    check(a.size() > 0 && a[0].address == "192.168.1.20", "an IPv4 address is stored as it is");
    check(a.size() > 0 && a[0].name == "192.168.1.20 (eth0)", "and shown with its device name");
    check(a.size() > 1 && a[1].address == "[fe80::1]", "an IPv6 address is stored in brackets");
    check(a.size() > 1 && a[1].name == "[fe80::1] (A1200 Ethernet)", "and shown with its friendly name when it has one");
    check(gfot::netif::adapters({}).empty(), "no interfaces, no lines (the dialog still offers any address)");

    if (argc > 1)
    {
        std::string d = argv[1];
        std::string li = slurp(d + "/listeninterfacedialog.cpp");
        std::string pc = slurp(d + "/preferencesconnectionpage.cpp");
        std::string pd = slurp(d + "/preferencesdialog.cpp");
        std::string pg = slurp(d + "/preferencesgeneralpage.cpp");
        std::string pa = slurp(d + "/preferencesadvancedpage.cpp");
        check(!li.empty() && !pc.empty() && !pd.empty() && !pg.empty() && !pa.empty(), "the five dialog files are there");
        check(windows_only(li, "<WinSock2.h>") && windows_only(li, "<iphlpapi.h>"), "listeninterfacedialog: WinSock only on Windows");
        check(windows_only(pc, "<WinSock2.h>"), "preferencesconnectionpage: WinSock only on Windows");
        check(windows_only(pd, "<CommCtrl.h>"), "preferencesdialog: CommCtrl only on Windows");
        check(windows_only(pg, "<strsafe.h>"), "preferencesgeneralpage: strsafe only on Windows");
        check(pa.find("<##") == std::string::npos, "preferencesadvancedpage: no token pasted onto '<'");
        check(li.find("enum_net_interfaces") != std::string::npos, "listeninterfacedialog asks libtorrent for the interfaces");
    }

    std::printf("prefs_pages_test: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
