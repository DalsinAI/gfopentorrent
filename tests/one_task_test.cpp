// GFOpenTorrent host test: libtorrent on one task, as the Amiga build runs
// it. A bsdsocket.library base belongs to the task that opened it, so
// GFOpenBoost's Asio is built with BOOST_ASIO_DISABLE_THREADS and every
// socket call must come from one task. libtorrent built with that and with
// TORRENT_AMIGA_HOST_TEST, sessions made on our own io_context (so the
// session starts no network thread) and posix_disk_io (disk work on the
// network thread): a 300 KB transfer between two sessions, while a timer
// checks that the process never has a second thread. Every session call is
// made from inside io.run(), as the UI will have to.
// Copyright (c) 2026 AmiCradle Team. MIT licence (see LICENSE).
#include <libtorrent/config.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/posix_disk_io.hpp>
#include <libtorrent/sha1_hash.hpp>
#include <libtorrent/operations.hpp>
#include <functional>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <vector>

#if !defined TORRENT_AMIGA || !defined BOOST_ASIO_DISABLE_THREADS
#error "build with -DTORRENT_AMIGA_HOST_TEST -DBOOST_ASIO_DISABLE_THREADS"
#endif

namespace lt = libtorrent;
namespace asio = boost::asio;

static int failed = 0;
static void check(const char *what, bool ok, const std::string &detail = std::string())
{
    std::printf("%s %s%s%s\n", ok ? "PASS" : "FAIL", what, detail.empty() ? "" : ": ", detail.c_str());
    std::fflush(stdout);
    if (!ok) failed++;
}

static int threads_now()
{
    int n = 0;
    if (DIR *d = opendir("/proc/self/task")) {
        while (dirent *e = readdir(d))
            if (e->d_name[0] != '.') n++;
        closedir(d);
    }
    return n;
}

static lt::settings_pack pack(bool utp)
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
    // hashing on the network thread, no hasher threads
    p.set_int(lt::settings_pack::hashing_threads, 0);
    p.set_int(lt::settings_pack::aio_threads, 0);
    return p;
}

static void run_transfer(const std::string &dir, bool utp)
{
    const std::string kind = utp ? "uTP" : "TCP";
    std::string seed = dir + "/s" + kind, leech = dir + "/l" + kind;
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

    asio::io_context io;
    lt::session_params pa(pack(utp)), pb(pack(utp));
    pa.disk_io_constructor = lt::posix_disk_io_constructor;
    pb.disk_io_constructor = lt::posix_disk_io_constructor;
    lt::session a(std::move(pa), io);
    lt::session b(std::move(pb), io);

    int max_threads = threads_now(), port = 0, pieces = 0;
    bool done = false, timeout = false;
    lt::torrent_handle hb;
    auto t0 = std::chrono::steady_clock::now();
    long ms = 0;

    asio::steady_timer tick(io);
    std::function<void()> step = [&] {
        max_threads = std::max(max_threads, threads_now());
        std::vector<lt::alert *> as;
        if (!port) {
            a.pop_alerts(&as);
            for (auto *al : as)
                if (auto *l = lt::alert_cast<lt::listen_succeeded_alert>(al))
                    if ((l->socket_type == lt::socket_type_t::utp) == utp) port = l->port;
            if (port) {
                lt::add_torrent_params p;
                p.ti = ti;
                p.save_path = seed;
                a.async_add_torrent(p);
                lt::add_torrent_params q;
                q.ti = ti;
                q.save_path = leech;
                hb = b.add_torrent(q);
                hb.connect_peer(lt::tcp::endpoint(lt::make_address("127.0.0.1"), std::uint16_t(port)));
            }
        } else {
            a.pop_alerts(&as);
            b.pop_alerts(&as);
            lt::torrent_status st = hb.status();
            pieces = st.num_pieces;
            if (st.is_seeding) done = true;
        }
        ms = long(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count());
        if (ms > 60000) timeout = true;
        if (done || timeout) { io.stop(); return; }
        tick.expires_after(std::chrono::milliseconds(50));
        tick.async_wait([&](boost::system::error_code const &) { step(); });
    };
    asio::post(io, step);
    io.run();
    check((kind + ": seed listens").c_str(), port != 0, std::to_string(port));
    check((kind + ": 300 KB on one task, hashes checked").c_str(), done && pieces == ti->num_pieces(),
          std::to_string(pieces) + "/" + std::to_string(ti->num_pieces()) + " pieces, " + std::to_string(ms) + " ms"
          + (timeout ? ", timeout" : ""));
    check((kind + ": never a second thread").c_str(), max_threads == 1, "max " + std::to_string(max_threads));
    // the sessions' destructors need the io_context to finish their work
    io.restart();
}

// Names: libtorrent looks up trackers and web seeds with Asio's
// async_resolve (aux::resolver), which normally runs on a helper thread.
// With Asio's threads off it must answer on the one task.
static void lookup_on_one_task()
{
    asio::io_context io;
    asio::ip::tcp::resolver r(io);
    std::string got, err;
    r.async_resolve("localhost", "80", [&](boost::system::error_code const &ec,
                                           asio::ip::tcp::resolver::results_type res) {
        if (ec) err = ec.message();
        for (auto const &e : res)
            if (e.endpoint().address().is_v4()) got = e.endpoint().address().to_string();
    });
    int max_threads = threads_now();
    io.run();
    max_threads = std::max(max_threads, threads_now());
    check("async_resolve(localhost) on one task", got == "127.0.0.1", got + err);
    check("lookup: never a second thread", max_threads == 1, "max " + std::to_string(max_threads));
}

int main()
{
    check("one thread at start", threads_now() == 1, std::to_string(threads_now()));
    char tmpl[] = "/tmp/gfot1-XXXXXX";
    const char *dir = mkdtemp(tmpl);
    check("temp dir", dir != nullptr);
    if (dir) {
        run_transfer(dir, true);
        run_transfer(dir, false);
    }
    lookup_on_one_task();
    std::printf("one_task_test: %d failed\n", failed);
    return failed ? 1 : 0;
}
