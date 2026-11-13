#!/bin/sh
# GFOpenTorrent host tests: libtorrent, patched, built on a Linux host with
# TORRENT_AMIGA_HOST_TEST, which picks its Amiga platform block, and
# tests/amiga_host_test.cpp run against it. Then libtorrent again with
# Asio's threads off, as GFOpenBoost builds it for the Amiga, and
# tests/one_task_test.cpp. OpenSSL is kept out, as on the Amiga. Needs
# cmake, g++ and Boost headers (1.83 or later).
#   WORK=dir JOBS=n tests/run-host-tests.sh
# Copyright (c) 2026 AmiCradle Team. MIT licence.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
# libtorrent's own compile definitions (encryption off, no deprecated API,
# Asio options), so a test sees the same structs as the library
lt_defs() {
    sed -n 's/^CXX_DEFINES = //p' "$1/CMakeFiles/torrent-rasterbar.dir/flags.make" \
        | tr ' ' '\n' | grep -v '^-DTORRENT_BUILDING_LIBRARY$' | tr '\n' ' '
}
WORK=${WORK:-"$HOME/.cache/gfopentorrent"}
JOBS=${JOBS:-2}
export WORK="$WORK/src"
sh "$HERE/scripts/fetch-sources.sh" libtorrent
LT="$WORK/libtorrent"
B="$WORK/../host-build"
mkdir -p "$B"
cmake -S "$LT" -B "$B" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
    -Dencryption=OFF -Dtry-signal=OFF -Ddeprecated-functions=OFF -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_GnuTLS=TRUE \
    -DCMAKE_CXX_FLAGS="-DTORRENT_AMIGA_HOST_TEST" > "$B/configure.log"
cmake --build "$B" --target torrent-rasterbar -j "$JOBS" > "$B/build.log" 2>&1 \
    || { grep -E 'error' "$B/build.log" | head -40; exit 1; }
# licence check: without OpenSSL, libtorrent's CMake looks for GnuTLS (LGPL),
# which must never come in
# nm's own failure must not pass as "no GnuTLS"
nm -A "$B/libtorrent-rasterbar.a" > "$B/nm.txt" 2>"$B/nm.err" \
    || { echo "FAIL nm could not read libtorrent: $(head -1 "$B/nm.err")"; exit 1; }
[ -s "$B/nm.txt" ] || { echo "FAIL nm listed no symbols"; exit 1; }
if grep -qi gnutls "$B/nm.txt"; then
    echo "FAIL libtorrent links GnuTLS"; exit 1
fi
echo "PASS no GnuTLS in libtorrent"
# no mmap on the Amiga, so no try_signal (libnix has no sigjmp_buf):
# patch 0003 builds without it, and nothing may still need it
if grep -q '_ZN3sig' "$B/nm.txt"; then
    echo "FAIL libtorrent still has try_signal"; exit 1
fi
echo "PASS libtorrent built without try_signal"
# patch 0001 repeats struct gfot_iface for enum_net.cpp: it must stay
# the same as compat/gfot_ifq.h's
gfot_struct() { sed -n '/^+\{0,1\}struct gfot_iface/,/};/p' "$1" | sed 's/^+//; s/\/\*.*\*\///' | tr -d ' \t\n'; }
a=$(gfot_struct "$HERE/compat/gfot_ifq.h"); b=$(gfot_struct "$HERE/patches/libtorrent/0001-amiga-platform.patch")
if [ -z "$a" ] || [ "$a" != "$b" ]; then
    echo "FAIL struct gfot_iface differs between compat/gfot_ifq.h and patch 0001"; exit 1
fi
echo "PASS struct gfot_iface the same in compat/gfot_ifq.h and patch 0001"
for c in gfot_ifq_host gfot_ifq_common; do
    ${CC:-gcc} -std=gnu99 -O1 -Wall -c "$HERE/compat/$c.c" -o "$B/$c.o"
done
${CXX:-g++} -std=c++17 -O1 -DTORRENT_AMIGA_HOST_TEST $(lt_defs "$B") \
    -I"$LT/include" -I"$HERE/compat" "$HERE/tests/amiga_host_test.cpp" "$B/libtorrent-rasterbar.a" \
    "$B/gfot_ifq_host.o" "$B/gfot_ifq_common.o" \
    -pthread -o "$B/amiga_host_test"
"$B/amiga_host_test"
# the lab probe, against the host's interfaces
${CC:-gcc} -std=gnu99 -O1 -Wall -I"$HERE/compat" -o "$B/ifqprobe" "$HERE/tests/ifqprobe.c" \
    "$B/gfot_ifq_host.o" "$B/gfot_ifq_common.o"
"$B/ifqprobe"

# ---- one task: Asio's threads off, and the host's Boost given GFOpenBoost's
# patch 0005 (async_resolve without a worker thread) in an overlay
OV="$WORK/../boost-overlay"
SYS=${BOOST_INCLUDE:-/usr/include}
# Boost 1.83 has the op in resolver_service_base.ipp, 1.87 and later in
# resolver_thread_pool.ipp (as GFOpenBoost's 1.89); "this" serves as an
# owner other than scheduler_ in both
F=""
for f in boost/asio/detail/impl/resolver_thread_pool.ipp boost/asio/detail/impl/resolver_service_base.ipp; do
    if grep -q '::start_resolve_op(resolve_op\* op)$' "$SYS/$f" 2>/dev/null; then F=$f; break; fi
done
[ -n "$F" ] || { echo "no start_resolve_op in the host's Boost under $SYS"; exit 1; }
mkdir -p "$OV/$(dirname "$F")"
awk '
  { print }
  /::start_resolve_op\(resolve_op\* op\)$/ { want = 1; next }
  want && /^\{$/ {
    print "#if defined(BOOST_ASIO_DISABLE_THREADS)"
    print "  // host stand-in for GFOpenBoost patches/headers/0005"
    print "  scheduler_.work_started();"
    print "  op->complete(this, boost::system::error_code(), 0);"
    print "  return;"
    print "#endif"
    want = 0; done = 1
  }
  END { if (!done) exit 1 }
' "$SYS/$F" > "$OV/$F" || { echo "could not patch the host's $F"; exit 1; }
echo "overlay: $F (Boost $(sed -n 's/^#define BOOST_LIB_VERSION "\(.*\)"/\1/p' "$SYS/boost/version.hpp"))"
B1="$WORK/../host-1task"
mkdir -p "$B1"
cmake -S "$LT" -B "$B1" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
    -Dencryption=OFF -Dtry-signal=OFF -Ddeprecated-functions=OFF -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_GnuTLS=TRUE \
    -DCMAKE_CXX_FLAGS="-I$OV -DTORRENT_AMIGA_HOST_TEST -DBOOST_ASIO_DISABLE_THREADS" > "$B1/configure.log"
cmake --build "$B1" --target torrent-rasterbar -j "$JOBS" > "$B1/build.log" 2>&1 \
    || { grep -E 'error' "$B1/build.log" | head -40; exit 1; }
${CXX:-g++} -std=c++17 -O1 -I"$OV" -DTORRENT_AMIGA_HOST_TEST -DBOOST_ASIO_DISABLE_THREADS $(lt_defs "$B1") \
    -I"$LT/include" -I"$HERE/compat" "$HERE/tests/one_task_test.cpp" "$B1/libtorrent-rasterbar.a" \
    "$B/gfot_ifq_host.o" "$B/gfot_ifq_common.o" \
    -pthread -o "$B1/one_task_test"
"$B1/one_task_test"
