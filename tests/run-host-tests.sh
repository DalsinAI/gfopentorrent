#!/bin/sh
# GFOpenTorrent host tests: libtorrent, patched, built on a Linux host with
# TORRENT_AMIGA_HOST_TEST, which picks its Amiga platform block, and
# tests/amiga_host_test.cpp run against it. Then libtorrent again with
# Asio's threads off, as GFOpenBoost builds it for the Amiga, and
# tests/one_task_test.cpp. OpenSSL is kept out, as on the Amiga. Needs
# cmake, g++ and Boost 1.83's headers.
#   WORK=dir JOBS=n tests/run-host-tests.sh
# Copyright (c) 2026 AmiCradle Team. MIT licence.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
WORK=${WORK:-"$HOME/.cache/gfopentorrent"}
JOBS=${JOBS:-2}
export WORK="$WORK/src"
sh "$HERE/scripts/fetch-sources.sh" libtorrent
LT="$WORK/libtorrent"
B="$WORK/../host-build"
mkdir -p "$B"
cmake -S "$LT" -B "$B" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
    -Dencryption=OFF -Ddeprecated-functions=OFF -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=TRUE \
    -DCMAKE_CXX_FLAGS="-DTORRENT_AMIGA_HOST_TEST" > "$B/configure.log"
cmake --build "$B" --target torrent-rasterbar -j "$JOBS" > "$B/build.log" 2>&1 \
    || { grep -E 'error' "$B/build.log" | head -40; exit 1; }
for c in gfot_ifq_host gfot_ifq_common; do
    ${CC:-gcc} -std=gnu99 -O1 -Wall -c "$HERE/compat/$c.c" -o "$B/$c.o"
done
${CXX:-g++} -std=c++17 -O1 -DTORRENT_AMIGA_HOST_TEST -DTORRENT_ABI_VERSION=100 \
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
F=boost/asio/detail/impl/resolver_service_base.ipp
SYS=${BOOST_INCLUDE:-/usr/include}
mkdir -p "$OV/$(dirname "$F")"
awk '
  { print }
  /^void resolver_service_base::start_resolve_op\(resolve_op\* op\)$/ { want = 1; next }
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
' "$SYS/$F" > "$OV/$F" || { echo "the host's $F has no start_resolve_op to patch (Boost 1.83 expected)"; exit 1; }
B1="$WORK/../host-1task"
mkdir -p "$B1"
cmake -S "$LT" -B "$B1" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
    -Dencryption=OFF -Ddeprecated-functions=OFF -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=TRUE \
    -DCMAKE_CXX_FLAGS="-I$OV -DTORRENT_AMIGA_HOST_TEST -DBOOST_ASIO_DISABLE_THREADS" > "$B1/configure.log"
cmake --build "$B1" --target torrent-rasterbar -j "$JOBS" > "$B1/build.log" 2>&1 \
    || { grep -E 'error' "$B1/build.log" | head -40; exit 1; }
${CXX:-g++} -std=c++17 -O1 -I"$OV" -DTORRENT_AMIGA_HOST_TEST -DBOOST_ASIO_DISABLE_THREADS -DTORRENT_ABI_VERSION=100 \
    -I"$LT/include" -I"$HERE/compat" "$HERE/tests/one_task_test.cpp" "$B1/libtorrent-rasterbar.a" \
    "$B/gfot_ifq_host.o" "$B/gfot_ifq_common.o" \
    -pthread -o "$B1/one_task_test"
"$B1/one_task_test"
