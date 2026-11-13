#!/bin/sh
# GFOpenTorrent host tests: libtorrent, patched, built on a Linux host with
# TORRENT_AMIGA_HOST_TEST, which picks its Amiga platform block, and
# tests/amiga_host_test.cpp run against it. OpenSSL is kept out, as on the
# Amiga. Needs cmake, g++ and Boost headers (1.83 or later).
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
