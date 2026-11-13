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
- interfaces from `SIOCGIFCONF`, stepping 4.4BSD's variable-length entries;
- routes from the interfaces plus a connected UDP socket
  (`TORRENT_USE_UDP_ROUTE`), because bsdsocket.library has no route table
  call. The gateway isn't known, so NAT-PMP has no router to ask; UPnP
  finds its router by multicast.

Asio comes from GFOpenBoost and waits through event.library, so the
package puts `event.library` in `LIBS:`, as aMule's does. Without it every
socket call fails with ENOSYS.

## Tests

    JOBS=4 tests/run-host-tests.sh

builds the patched libtorrent on a Linux host with `TORRENT_AMIGA_HOST_TEST`,
which picks the Amiga platform block there, and runs
`tests/amiga_host_test.cpp`: the platform switches, interfaces and routes
through the Amiga paths, and a 300 KB transfer between two sessions over
loopback, once over uTP and once over TCP. CI runs it on every pull request.
UDP on the Amiga itself is checked by GFOpenBoost's `UDPLT`.
