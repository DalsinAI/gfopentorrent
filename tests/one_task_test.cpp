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
#include <cstdlib>
#include <exception>
#ifdef __linux__
#include <dirent.h>
#endif
#include <fstream>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <vector>

#if !defined TORRENT_AMIGA || !defined BOOST_ASIO_DISABLE_THREADS
#error "build with -DTORRENT_AMIGA_HOST_TEST -DBOOST_ASIO_DISABLE_THREADS (the Amiga build has both)"
#endif

namespace lt = libtorrent;
namespace asio = boost::asio;

// Lab runs: stdout to a file is fully buffered, and libnix's abort() drops
// what waits there, so an abort used to show nothing. Before main()
// (init_priority 101, ahead of libtorrent's and Asio's statics): stdout
// unbuffered, and a terminate handler that names the exception on stderr.
// STEP lines on stderr say how far a run got.
static void on_terminate()
{
    if (std::exception_ptr e = std::current_exception()) {
        try { std::rethrow_exception(e); }
        catch (std::exception const &x) { std::fprintf(stderr, "TERMINATE: %s\n", x.what()); }
        catch (...) { std::fprintf(stderr, "TERMINATE: an exception that is not a std::exception\n"); }
    } else {
        std::fprintf(stderr, "TERMINATE: no exception (std::terminate called)\n");
    }
    std::fflush(stderr);
    std::abort();
}
struct early_setup {
    early_setup()
    {
        std::setvbuf(stdout, nullptr, _IONBF, 0);
        std::set_terminate(on_terminate);
        std::fputs("STEP static setup\n", stderr);
        std::fflush(stderr);
    }
};
static early_setup early_setup_ __attribute__((init_priority(101)));
static void step_line(const std::string &what)
{
    std::fprintf(stderr, "STEP %s\n", what.c_str());
    std::fflush(stderr);
}

#if !defined __amigaos__ && !defined __AMIGA__
// The Amiga's names are IPv4 only: GFOpenBoost's inet_pton() answers
// EAFNOSUPPORT for AF_INET6 (compat/src/amiga_net.c), so an IPv6 literal
// libtorrent parses becomes a throw. Here this inet_pton() stands in for the
// C library's, as GFOpenBoost's, so the host meets the same (lsd.cpp's
// IPv6 group parsed at start-up aborted OneTask before main on the Amiga).
#include <arpa/inet.h>
#include <cerrno>
extern "C" int inet_pton(int af, const char *src, void *dst)
{
    if (af != AF_INET) { errno = EAFNOSUPPORT; return -1; }
    unsigned v[4];
    char tail;
    if (std::sscanf(src, "%u.%u.%u.%u%c", &v[0], &v[1], &v[2], &v[3], &tail) != 4) return 0;
    unsigned char *b = static_cast<unsigned char *>(dst);
    for (int i = 0; i < 4; i++) {
        if (v[i] > 255) return 0;
        b[i] = static_cast<unsigned char>(v[i]);
    }
    return 1;
}
#endif

static int failed = 0;
static void check(const char *what, bool ok, const std::string &detail = std::string())
{
    std::printf("%s %s%s%s\n", ok ? "PASS" : "FAIL", what, detail.empty() ? "" : ": ", detail.c_str());
    std::fflush(stdout);
    if (!ok) failed++;
}

// Linux: the process's threads. The Amiga build is the lab run of this same
// file (tools/stove/build-amiga.sh); there a second task would show as
// a bsdsocket.library failure, so it counts 1.
static int threads_now()
{
#ifdef __linux__
    int n = 0;
    if (DIR *d = opendir("/proc/self/task")) {
        while (dirent *e = readdir(d))
            if (e->d_name[0] != '.') n++;
        closedir(d);
    }
    return n;
#else
    return 1;
#endif
}

static lt::settings_pack pack(bool utp, bool lsd = false)
{
    lt::settings_pack p;
    p.set_str(lt::settings_pack::listen_interfaces, "127.0.0.1:0");
    p.set_bool(lt::settings_pack::enable_dht, false);
    // LSD on: GFOpenSocket refuses its multicast group, and the transfer
    // must still go
    p.set_bool(lt::settings_pack::enable_lsd, lsd);
    p.set_bool(lt::settings_pack::enable_upnp, false);
    p.set_bool(lt::settings_pack::enable_natpmp, false);
    p.set_bool(lt::settings_pack::enable_outgoing_utp, utp);
    p.set_bool(lt::settings_pack::enable_incoming_utp, utp);
    p.set_bool(lt::settings_pack::enable_outgoing_tcp, !utp);
    p.set_bool(lt::settings_pack::enable_incoming_tcp, !utp);
    p.set_bool(lt::settings_pack::allow_multiple_connections_per_ip, true);
    p.set_int(lt::settings_pack::alert_mask, lt::alert_category::error | lt::alert_category::status
        | lt::alert_category::storage | lt::alert_category::connect | lt::alert_category::peer);
    // hashing on the network thread, no hasher threads
    p.set_int(lt::settings_pack::hashing_threads, 0);
    p.set_int(lt::settings_pack::aio_threads, 0);
    return p;
}

// Print what the sessions report, so a lab run that moves no piece says
// why: the first 60 alerts of each kind of test, then only a count.
static void show_alerts(const std::string &kind, const char *who, std::vector<lt::alert *> const &as, int &shown)
{
    for (auto *al : as) {
        if (shown++ >= 60) continue;
        std::fprintf(stderr, "ALERT %s %s: %s\n", kind.c_str(), who, al->message().c_str());
    }
    std::fflush(stderr);
}

static const char *state_name(lt::torrent_status::state_t s)
{
    switch (s) {
    case lt::torrent_status::checking_files: return "checking_files";
    case lt::torrent_status::downloading_metadata: return "downloading_metadata";
    case lt::torrent_status::downloading: return "downloading";
    case lt::torrent_status::finished: return "finished";
    case lt::torrent_status::seeding: return "seeding";
    case lt::torrent_status::checking_resume_data: return "checking_resume_data";
    default: return "?";
    }
}

static std::string status_line(lt::torrent_handle const &h)
{
    lt::torrent_status st = h.status();
    std::string e = st.errc ? st.errc.message() : std::string("none");
    return std::string(state_name(st.state)) + ", " + std::to_string(st.num_pieces) + " pieces, "
        + std::to_string(st.num_peers) + " peers, paused " + (st.flags & lt::torrent_flags::paused ? "yes" : "no")
        + ", error " + e;
}

static void run_transfer(const std::string &dir, bool utp, bool lsd = false)
{
    const std::string kind = std::string(utp ? "uTP" : "TCP") + (lsd ? "+LSD" : "");
    std::string seed = dir + "/s" + kind, leech = dir + "/l" + kind;
    mkdir(seed.c_str(), 0755);
    mkdir(leech.c_str(), 0755);
    step_line(kind + ": drawers " + seed + " and " + leech);
    {
        std::ofstream f(seed + "/payload.bin", std::ios::binary);
        for (int i = 0; i < 300 * 1024; i++) f.put(char((i * 131) ^ (i >> 7)));
    }
    step_line(kind + ": payload written");
    lt::file_storage fs;
    lt::add_files(fs, seed + "/payload.bin");
    step_line(kind + ": add_files");
    lt::create_torrent ct(fs, 16 * 1024);
    lt::set_piece_hashes(ct, seed);
    step_line(kind + ": set_piece_hashes");
    std::vector<char> buf;
    lt::bencode(std::back_inserter(buf), ct.generate());
    step_line(kind + ": bencode");
    auto ti = std::make_shared<lt::torrent_info>(buf, lt::from_span);
    step_line(kind + ": torrent made");

    asio::io_context io;
    lt::session_params pa(pack(utp, lsd)), pb(pack(utp, lsd));
    pa.disk_io_constructor = lt::posix_disk_io_constructor;
    pb.disk_io_constructor = lt::posix_disk_io_constructor;
    auto a = std::make_unique<lt::session>(std::move(pa), io);
    auto b = std::make_unique<lt::session>(std::move(pb), io);
    step_line(kind + ": sessions made");

    int max_threads = threads_now(), port = 0, pieces = 0;
    bool done = false, timeout = false, seed_ready = false;
    int shown = 0, connects = 0;
    std::string seed_end = "not added", leech_end = "not added";
    lt::torrent_handle ha, hb;
    auto t0 = std::chrono::steady_clock::now(), last = t0 - std::chrono::seconds(2);
    long ms = 0;

    asio::steady_timer tick(io);
    std::function<void()> step = [&] {
        max_threads = std::max(max_threads, threads_now());
        std::vector<lt::alert *> as;
        if (!port) {
            a->pop_alerts(&as);
            show_alerts(kind, "seed", as, shown);
            for (auto *al : as)
                if (auto *l = lt::alert_cast<lt::listen_succeeded_alert>(al))
                    if ((l->socket_type == lt::socket_type_t::utp) == utp) port = l->port;
            if (port) {
                step_line(kind + ": seed listens on " + std::to_string(port));
                lt::add_torrent_params p;
                p.ti = ti;
                p.save_path = seed;
                ha = a->add_torrent(p);
                lt::add_torrent_params q;
                q.ti = ti;
                q.save_path = leech;
                hb = b->add_torrent(q);
            }
        } else {
            a->pop_alerts(&as);
            show_alerts(kind, "seed", as, shown);
            as.clear();
            b->pop_alerts(&as);
            show_alerts(kind, "leech", as, shown);
            lt::torrent_status st = hb.status();
            if (!seed_ready && ha.status().is_seeding) {
                seed_ready = true;
                step_line(kind + ": seed has checked its file");
            }
            pieces = st.num_pieces;
            if (st.is_seeding) done = true;
            // connect once the seed has checked its file, and again each
            // second while the leech has no peer (a peer turned away while
            // the seed checks is otherwise tried again only after a minute)
            auto now = std::chrono::steady_clock::now();
            if (!done && ha.status().is_seeding && st.num_peers == 0 && now - last >= std::chrono::seconds(1)) {
                hb.connect_peer(lt::tcp::endpoint(lt::make_address("127.0.0.1"), std::uint16_t(port)));
                last = now;
                if (connects++ < 3) step_line(kind + ": leech connects to 127.0.0.1:" + std::to_string(port));
            }
        }
        ms = long(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count());
        if (ms > 60000) timeout = true;
        if (done || timeout) {
            // status() is a call into the session: make it here, on the
            // loop, as one outside io.run() would wait for ever
            if (ha.is_valid()) seed_end = status_line(ha);
            if (hb.is_valid()) leech_end = status_line(hb);
            io.stop();
            return;
        }
        tick.expires_after(std::chrono::milliseconds(50));
        tick.async_wait([&](boost::system::error_code const &) { step(); });
    };
    asio::post(io, step);
    io.run();
    step_line(kind + ": loop ended after " + std::to_string(shown) + " alerts, " + std::to_string(connects) + " connects");
    step_line(kind + ": seed " + seed_end);
    step_line(kind + ": leech " + leech_end);
    check((kind + ": seed listens").c_str(), port != 0, std::to_string(port));
    check((kind + ": 300 KB on one task, hashes checked").c_str(), done && pieces == ti->num_pieces(),
          std::to_string(pieces) + "/" + std::to_string(ti->num_pieces()) + " pieces, " + std::to_string(ms) + " ms"
          + (timeout ? ", timeout" : ""));
    check((kind + ": never a second thread").c_str(), max_threads == 1, "max " + std::to_string(max_threads));
    // A session's shutdown runs in the io_context. Take a session_proxy
    // (abort()), which keeps the session alive, run the loop until the
    // shutdown is done, and only then let the proxies and the io_context go.
    // (The test used to destroy the io_context with the shutdown still
    // queued, which freed the sessions' sockets after their services:
    // malloc_consolidate() aborted now and then.) The UI has to end the
    // same way.
    lt::session_proxy proxy_a = a->abort(), proxy_b = b->abort();
    a.reset();
    b.reset();
    io.restart();
    auto t1 = std::chrono::steady_clock::now();
    io.run();
    long down = long(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t1).count());
    check((kind + ": sessions shut down inside io.run()").c_str(), io.stopped() && down < 10000, std::to_string(down) + " ms");
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
    step_line("main");
    check("one thread at start", threads_now() == 1, std::to_string(threads_now()));
#if defined __amigaos__ || defined __AMIGA__
    const char *dir = "T:gfot1";
    mkdir(dir, 0755);
#else
    char tmpl[] = "/tmp/gfot1-XXXXXX";
    const char *dir = mkdtemp(tmpl);
#endif
    check("temp dir", dir != nullptr);
    if (dir) {
        run_transfer(dir, true);
        run_transfer(dir, false);
        run_transfer(dir, false, true);
    }
    step_line("lookup");
    lookup_on_one_task();
    std::printf("one_task_test: %d failed\n", failed);
    return failed ? 1 : 0;
}
