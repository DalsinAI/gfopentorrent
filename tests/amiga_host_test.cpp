// GFOpenTorrent host test: libtorrent built with TORRENT_AMIGA_HOST_TEST,
// which picks libtorrent's Amiga platform block on a Linux host. Checks the
// Amiga paths: interfaces from gfot_interfaces() (Roadshow's interface
// calls on the Amiga, getifaddrs() here), routes from the UDP fallback
// (TORRENT_USE_UDP_ROUTE), and a whole transfer over loopback between two
// sessions with posix_disk_io (no mmap on the Amiga), once over uTP only and
// once over TCP only. One line a check, "amiga_host_test: N failed" at the
// end, return code 0 only when all passed.
// Copyright (c) 2026 AmiCradle Team. MIT licence (see LICENSE).
#include <libtorrent/config.hpp>
#include <libtorrent/enum_net.hpp>
#include <libtorrent/platform_util.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/alert_types.hpp>
#include "gfot_ifq.h"
#include <libtorrent/aux_/file_pointer.hpp>
#include <cerrno>
#include <libtorrent/torrent_status.hpp>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

#ifndef TORRENT_AMIGA
#error "build libtorrent and this test with -DTORRENT_AMIGA_HOST_TEST"
#endif

namespace lt = libtorrent;

static int failed = 0;
static void check(const char *what, bool ok, const std::string &detail = std::string())
{
    std::printf("%s %s%s%s\n", ok ? "PASS" : "FAIL", what, detail.empty() ? "" : ": ", detail.c_str());
    std::fflush(stdout);
    if (!ok) failed++;
}

static void config_checks()
{
    check("TORRENT_AMIGA block picked", true);
    check("no mmap (posix_disk_io)", TORRENT_HAVE_MMAP == 0);
    check("interfaces through gfot_interfaces()", TORRENT_USE_AMIGA_IFQ == 1 && TORRENT_USE_IFCONF == 0
          && TORRENT_USE_NETLINK == 0 && TORRENT_USE_IFADDRS == 0);
    check("routes through the UDP fallback", TORRENT_USE_UDP_ROUTE == 1 && TORRENT_USE_SYSCTL == 0);
    check("no I2P", TORRENT_USE_I2P == 0);
    // patch 0004: libnix has no getrlimit()
    check("no getrlimit, a fixed max_open_files", TORRENT_USE_RLIMIT == 0 && lt::max_open_files() == 10000,
          std::to_string(lt::max_open_files()));
}

// GFOpenSocket lists one interface and no loopback: gfot_add_loopback()
// adds lo0 then, and only then
static void loopback_checks()
{
    gfot_iface rows[3] = {};
    std::strcpy(rows[0].name, "gfopensocket0");
    rows[0].addr[0] = 192; rows[0].addr[1] = 168; rows[0].addr[3] = 195;
    rows[0].mask[0] = rows[0].mask[1] = rows[0].mask[2] = 255;
    rows[0].up = 1;
    int n = gfot_add_loopback(rows, 1, 3);
    check("lo0 added after the stack's one interface", n == 2 && std::string(rows[1].name) == "lo0"
          && rows[1].addr[0] == 127 && rows[1].addr[3] == 1 && rows[1].mask[0] == 255 && rows[1].mask[1] == 0);
    check("lo0 not added twice", gfot_add_loopback(rows, 2, 3) == 2);
    check("no room, nothing added", gfot_add_loopback(rows, 1, 1) == 1);
    // the guard before ObtainInterfaceList: version 4 and a jump table
    // that reaches QueryInterfaceTagList (-444)
    check("Roadshow-style stack has the interface calls", gfot_stack_has_ifq(4, 444) == 1 && gfot_stack_has_ifq(4, 600) == 1);
    check("version 4 table too short (AmiTCP 4.x): not called", gfot_stack_has_ifq(4, 438) == 0);
    check("version 3 stack: not called", gfot_stack_has_ifq(3, 600) == 0);
}

static void net_checks()
{
    boost::asio::io_context io;
    lt::error_code ec;
    auto ifs = lt::enum_net_interfaces(io, ec);
    std::string names;
    bool lo = false;
    for (auto const &i : ifs) {
        names += std::string(" ") + i.name + "=" + i.interface_address.to_string() + "/" + i.netmask.to_string();
        if (i.interface_address.is_loopback()) lo = true;
    }
    check("enum_net_interfaces", !ec && !ifs.empty(), ec ? ec.message() : names);
    check("loopback interface listed with its netmask", lo);

    auto routes = lt::enum_routes(io, ec);
    std::string rs;
    bool lo_net = false, deflt = false, named = true;
    for (auto const &r : routes) {
        rs += std::string(" ") + r.destination.to_string() + "/" + r.netmask.to_string() + "@" + r.name;
        if (r.destination == lt::make_address("127.0.0.0") && r.netmask == lt::make_address("255.0.0.0")) lo_net = true;
        if (r.destination.is_unspecified()) deflt = true;
        if (r.name[0] == 0) named = false;
    }
    check("enum_routes", !ec && !routes.empty(), ec ? ec.message() : rs);
    check("route to 127.0.0.0/8 from the interface", lo_net, rs);
    check("every route names its interface", named);
    // a host with no network has no default route; say so, don't fail
    std::printf("INFO default route: %s\n", deflt ? "yes" : "no (no network)");
    check("has_any_internet_route matches the default route", lt::has_any_internet_route(routes) == deflt);
}

static lt::settings_pack base_pack(bool utp)
{
    lt::settings_pack p;
    p.set_str(lt::settings_pack::listen_interfaces, "127.0.0.1:0");
    p.set_bool(lt::settings_pack::enable_dht, false);
    p.set_bool(lt::settings_pack::enable_lsd, false);
    p.set_bool(lt::settings_pack::enable_upnp, false);
    p.set_bool(lt::settings_pack::enable_natpmp, false);
    p.set_bool(lt::settings_pack::enable_outgoing_utp, utp);
    p.set_bool(lt::settings_pack::enable_incoming_utp, utp);
    p.set_bool(lt::settings_pack::enable_outgoing_tcp, !utp);
    p.set_bool(lt::settings_pack::enable_incoming_tcp, !utp);
    p.set_bool(lt::settings_pack::allow_multiple_connections_per_ip, true);
    p.set_int(lt::settings_pack::alert_mask, lt::alert_category::error | lt::alert_category::status);
    return p;
}

static int listen_port(lt::session &s, bool utp, int seconds)
{
    auto end = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    while (std::chrono::steady_clock::now() < end) {
        s.wait_for_alert(std::chrono::milliseconds(100));
        std::vector<lt::alert *> as;
        s.pop_alerts(&as);
        for (auto *a : as)
            if (auto *l = lt::alert_cast<lt::listen_succeeded_alert>(a))
                if ((l->socket_type == lt::socket_type_t::utp) == utp)
                    return l->port;
    }
    return 0;
}

static void transfer(const std::string &dir, bool utp)
{
    const char *kind = utp ? "uTP" : "TCP";
    std::string seed = dir + (utp ? "/seed-u" : "/seed-t"), leech = dir + (utp ? "/leech-u" : "/leech-t");
    mkdir(seed.c_str(), 0755);
    mkdir(leech.c_str(), 0755);
    {
        std::ofstream f(seed + "/payload.bin", std::ios::binary);
        for (int i = 0; i < 300 * 1024; i++) f.put(char((i * 131) ^ (i >> 7)));
    }
    lt::file_storage fs;
    lt::add_files(fs, seed + "/payload.bin");
    lt::create_torrent ct(fs, 16 * 1024);
    lt::set_piece_hashes(ct, seed);
    std::vector<char> buf;
    lt::bencode(std::back_inserter(buf), ct.generate());
    auto ti = std::make_shared<lt::torrent_info>(buf, lt::from_span);

    lt::session a(lt::session_params(base_pack(utp)));
    lt::session b(lt::session_params(base_pack(utp)));
    int port = listen_port(a, utp, 5);
    check((std::string(kind) + ": seed listens on 127.0.0.1").c_str(), port != 0, std::to_string(port));
    if (!port) return;

    lt::add_torrent_params pa;
    pa.ti = ti;
    pa.save_path = seed;
    lt::torrent_handle ha = a.add_torrent(pa);
    lt::add_torrent_params pb;
    pb.ti = ti;
    pb.save_path = leech;
    lt::torrent_handle hb = b.add_torrent(pb);
    lt::tcp::endpoint const seed_ep(lt::make_address("127.0.0.1"), std::uint16_t(port));

    // Connect once the seed has checked its file (a peer that arrives while
    // it checks is turned away, and libtorrent waits a minute before trying
    // that peer again), and again each second while the leech has no peer.
    auto t0 = std::chrono::steady_clock::now(), last = t0 - std::chrono::seconds(2);
    bool done = false;
    while (std::chrono::steady_clock::now() - t0 < std::chrono::seconds(60)) {
        lt::torrent_status st = hb.status();
        if (st.is_seeding) { done = true; break; }
        auto now = std::chrono::steady_clock::now();
        if (ha.status().is_seeding && st.num_peers == 0 && now - last >= std::chrono::seconds(1)) {
            hb.connect_peer(seed_ep);
            last = now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    long ms = long(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count());
    lt::torrent_status st = hb.status();
    check((std::string(kind) + ": 300 KB over loopback, hashes checked").c_str(), done && st.num_pieces == ti->num_pieces(),
          std::to_string(st.num_pieces) + "/" + std::to_string(ti->num_pieces()) + " pieces, " + std::to_string(ms) + " ms");
}

// patch 0002: libnix's fseek() takes a 32-bit long, so an offset past
// 2 GiB must fail with EFBIG, not seek somewhere else
static void seek_checks()
{
    FILE *f = std::tmpfile();
    if (!f) { check("tmpfile for the seek checks", false); return; }
    errno = 0;
    int r = lt::aux::portable_fseeko(f, std::int64_t(3) << 30, SEEK_SET);
    check("seek past 2 GiB refused with EFBIG", r == -1 && errno == EFBIG, std::to_string(r) + " errno " + std::to_string(errno));
    r = lt::aux::portable_fseeko(f, 1000, SEEK_SET);
    check("seek within 2 GiB", r == 0 && std::ftell(f) == 1000, std::to_string(r));
    std::fclose(f);
}

int main()
{
    config_checks();
    seek_checks();
    loopback_checks();
    net_checks();
    char tmpl[] = "/tmp/gfot-XXXXXX";
    const char *dir = mkdtemp(tmpl);
    check("temp dir", dir != nullptr);
    if (dir) {
        transfer(dir, true);
        transfer(dir, false);
    }
    std::printf("amiga_host_test: %d failed\n", failed);
    return failed ? 1 : 0;
}
