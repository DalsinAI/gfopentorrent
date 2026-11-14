# gfopentorrent

GFOpenTorrent: BitTorrent client for GFOS (PicoTorrent UI on libtorrent-rasterbar)

Part of GFOS (AmiCradle). Design: amicradle-teams `designs/` (GFOPENNEWS-20261113.md, GFOPENIRC-20261113.md, and the Team 2 answers of 13 Nov 2026).

Rules: commits are authored and committed by SacredTrees; draw through GFOpenLayout; third-party code keeps its own licence and goes in `third_party/` with its notice (BSD-3-Clause libtorrent is allowed; no GPL, LGPL or MPL code).

## Sources

`torrent.lock` pins the third-party sources; none is kept here.
`scripts/fetch-sources.sh` fetches each at its commit, checks it, and
applies `patches/<name>/`.

| Source | Licence | Pinned |
| --- | --- | --- |
| libtorrent-rasterbar | BSD-3-Clause | v2.0.15 |
| try_signal (libtorrent's submodule) | BSD-3-Clause | 105cce5 |
| PicoTorrent | MIT | v0.25.0 |

libtorrent 2.0 has no WebTorrent (2.1 adds it, with libdatachannel under
MPL-2.0). HTTPS trackers go through the gfopentls shim, never GnuTLS.
The BSD-3 notices go in the About box and the docs.

## libtorrent on AmigaOS

`patches/libtorrent/0001-amiga-platform.patch` fills in libtorrent's Amiga
platform block:

- no mmap, so `posix_disk_io` does the disk work;
- interfaces from `compat/gfot_ifq_amiga.c`, through Roadshow's
  `ObtainInterfaceList()` and `QueryInterfaceTagList()` (GFOpenSocket answers
  `SIOCGIFCONF` with EOPNOTSUPP), with `lo0` added when the stack doesn't
  list a loopback. Those calls are made only on a version 4 stack whose jump
  table reaches `QueryInterfaceTagList()` (-444); AmiTCP 4.x is version 4
  without them, and gets ENOSYS;
- routes from the interfaces plus a connected UDP socket
  (`TORRENT_USE_UDP_ROUTE`), because bsdsocket.library has no route table
  call. The gateway isn't known, so NAT-PMP has no router to ask; UPnP
  finds its router by multicast.

**One task.** A bsdsocket.library base belongs to the task that opened it,
so GFOpenBoost builds Asio with its threads off, and every socket call has
to come from one task. libtorrent does that when:

- the session is made on the program's own `io_context`
  (`lt::session(params, io)`), so it starts no network thread;
- `params.disk_io_constructor = lt::posix_disk_io_constructor`, with
  `hashing_threads` and `aio_threads` at 0, so disk work and hashing run on
  that task too;
- every call into the session is made from inside `io.run()` (a handler or
  `post()`), because a call from outside waits for the loop to answer it;
- at the end, `auto proxy = ses.abort();`, destroy the session, and run
  the loop until it returns before the proxy and the `io_context` go: the
  shutdown runs in the loop, and an `io_context` destroyed with it still
  queued frees the session's sockets out of order;
- GFOpenBoost has patch 0005 (#8), so `async_resolve` (tracker names) needs
  no worker thread. A lookup then blocks the task, and the UI with it, for
  as long as the stack's resolver takes; fine for trackers, but a name the
  user types should be looked up with that in mind.

The UI's loop is that `io_context`: GFOpenBoost's `exec_waiter` waits for
the window's MsgPort in the same wait as the sockets.

**Files up to 2 GiB.** libnix's `off_t` and `fseek()` are 32 bits, as
dos.library's `Seek()` is, so patch 0002 drops libtorrent's 64-bit asserts
and a seek or size past 2 GiB fails with EFBIG.

**No try_signal.** It catches SIGBUS from memory-mapped files; with no
mmap there's nothing to catch, and libnix has no `sigjmp_buf`. Patch 0003
adds a `try-signal` CMake option, and the Amiga build turns it off.

**What libnix lacks.** `compat/gfot_pio.c` gives libtorrent `pread()` and
`pwrite()` (seek, read or write, seek back; safe on one task), and patch
0004 turns off `getrlimit()` (`TORRENT_USE_RLIMIT 0`), so
`max_open_files()` gives a fixed number.

**No peer encryption yet.** libtorrent is built with `-Dencryption=OFF`
(no OpenSSL, no GnuTLS), which also turns off protocol encryption (PE/MSE).
Peers that insist on it won't connect. PE needs only RC4 and a
Diffie-Hellman exchange, which can come from GFOpenTLS later.

Asio comes from GFOpenBoost and waits through event.library, so the
package puts `event.library` in `LIBS:`, as aMule's does. Without it every
socket call fails with ENOSYS.

## Tests

    JOBS=4 tests/run-host-tests.sh

builds the patched libtorrent on a Linux host with `TORRENT_AMIGA_HOST_TEST`,
which picks the Amiga platform block there (with `compat/gfot_ifq_host.c`,
getifaddrs() in place of Roadshow's calls), and runs
`tests/amiga_host_test.cpp`: the platform switches, interfaces and routes
through the Amiga paths, and a 300 KB transfer between two sessions over
loopback, once over uTP and once over TCP. CI runs it on every pull request.
Local service discovery needs a multicast group GFOpenSocket doesn't open to
the guest (only mDNS's is), so it fails quietly there and DHT, trackers and
PEX find peers.
Then it builds libtorrent again with Asio's threads off (and the host's
Boost given the same change as GFOpenBoost's patch 0005) and runs
`tests/one_task_test.cpp`: the same transfers and a name lookup, all from
one `io.run()`, while a timer checks the process never has a second thread.
UDP on the Amiga itself is checked by GFOpenBoost's `UDPLT`.

`tools/lab-probe/build-amiga.sh` builds `IfqProbe` on the stove: what
`compat/gfot_ifq_amiga.c` gets from the stack, for a lab run (it needs
bsdsocket.library 4). The host tests run the same probe on the host's
interfaces.

## Building for AmigaOS

    STOVE=.../stoves/os32-gcc16-open6 GFOPENBOOST_SDK=<sdk> OUT=out JOBS=2 \
        tools/stove/build-amiga.sh

builds libtorrent with `cmake/toolchain-m68k-amigaos.cmake` (68040, Asio on
one task, no OpenSSL, no GnuTLS) and links the lab tests against it:
`OneTask` (`tests/one_task_test.cpp`, the host test's transfers and lookup,
run in `T:`) and `IfqProbe`. It also unpacks the pinned `event.library` for
the lab's `LIBS:`. The SDK needs GFOpenBoost's patch 0005 (#8); the script
stops if it is missing.
