#!/bin/sh
# GFOpenTorrent: libtorrent for AmigaOS on the stove, and the lab tests.
#   STOVE=.../stoves/os32-gcc16-open6 GFOPENBOOST_SDK=<sdk dir> OUT=out \
#       JOBS=2 tools/stove/build-amiga.sh
# GFOPENBOOST_SDK is an unpacked GFOpenBoost SDK with patch 0005 (#8):
# v1.89.0-rc4 or later, or rc3 with tools/asio-check's libboost_asio.a
# from #8 copied into its lib/. Builds, with no OpenSSL and no GnuTLS:
#   OUT/libtorrent-rasterbar.a, OUT/libgfotcompat.a
#   OUT/OneTask   tests/one_task_test.cpp: two sessions, uTP and TCP over
#                 loopback, and a name lookup, on one task
#   OUT/IfqProbe  tests/ifqprobe.c
#   OUT/event.library  for the lab's LIBS:
# Origimagic rule: JOBS at most 2.
# Copyright (c) 2026 AmiCradle Team. MIT licence.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open6"}
TC=${TOOLCHAIN:-"$STOVE/prefix"}
SDK=${GFOPENBOOST_SDK:?set GFOPENBOOST_SDK to an unpacked GFOpenBoost SDK}
OUT=${OUT:-out}
JOBS=${JOBS:-2}
[ "$JOBS" -le 2 ] || JOBS=2
WORK=${WORK:-"$HOME/.cache/gfopentorrent"}
mkdir -p "$OUT"
OUT=$(CDPATH= cd -- "$OUT" && pwd)
PATH="$TC/bin:$PATH"
command -v cmake >/dev/null || { echo "no cmake on this stove"; exit 1; }
[ -f "$SDK/lib/libboost_asio.a" ] || { echo "$SDK has no lib/libboost_asio.a"; exit 1; }
grep -q 'BOOST_ASIO_DISABLE_THREADS' "$SDK/include/boost/asio/detail/impl/resolver_thread_pool.ipp" \
    || { echo "$SDK lacks GFOpenBoost patch 0005 (async_resolve on one task)"; exit 1; }

WORK="$WORK/src" sh "$HERE/scripts/fetch-sources.sh" libtorrent
LT="$WORK/src/libtorrent"
B="$WORK/amiga-build"
BOOST_CMAKE=$(ls -d "$SDK"/lib/cmake/Boost-* | head -1)
cmake -S "$LT" -B "$B" -DCMAKE_TOOLCHAIN_FILE="$HERE/cmake/toolchain-m68k-amigaos.cmake" \
    -DAMIGA_TOOLCHAIN="$TC" -DGFOPENBOOST_SDK="$SDK" -DBoost_DIR="$BOOST_CMAKE" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 -DBUILD_SHARED_LIBS=OFF \
    -Dencryption=OFF -Ddeprecated-functions=OFF -Dlogging=OFF \
    -DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_GnuTLS=TRUE \
    > "$B.configure.log" 2>&1 || { tail -40 "$B.configure.log"; exit 1; }
cmake --build "$B" --target torrent-rasterbar -j "$JOBS" > "$B.build.log" 2>&1 \
    || { grep -E 'error' "$B.build.log" | sort -u | head -60; exit 1; }
if m68k-amigaos-nm -A "$B/libtorrent-rasterbar.a" 2>/dev/null | grep -qi gnutls; then
    echo "libtorrent links GnuTLS"; exit 1
fi
cp "$B/libtorrent-rasterbar.a" "$OUT/"

CF="-noixemul -m68040 -O2"
for c in gfot_ifq_amiga gfot_ifq_common; do
    m68k-amigaos-gcc $CF -std=gnu99 -Wall -I"$HERE/compat" -c "$HERE/compat/$c.c" -o "$B/$c.o"
done
rm -f "$OUT/libgfotcompat.a"
m68k-amigaos-ar rcs "$OUT/libgfotcompat.a" "$B/gfot_ifq_amiga.o" "$B/gfot_ifq_common.o"

LIBS="$OUT/libtorrent-rasterbar.a $OUT/libgfotcompat.a -L$SDK/lib -lboost_asio -lsocket -lpthread -latomic -lm"
m68k-amigaos-g++ $CF -std=gnu++17 -DBOOST_ASIO_DISABLE_THREADS -DBOOST_ASIO_SEPARATE_COMPILATION \
    -DTORRENT_ABI_VERSION=100 -DTORRENT_NO_DEPRECATE -DTORRENT_DISABLE_LOGGING \
    -I"$SDK/include" -I"$LT/include" -I"$HERE/compat" \
    -o "$OUT/OneTask" "$HERE/tests/one_task_test.cpp" $LIBS 2> "$B.link.log" \
    || { grep -E 'undefined|error' "$B.link.log" | sort -u | head -40; exit 1; }
m68k-amigaos-gcc $CF -std=gnu99 -Wall -I"$HERE/compat" -o "$OUT/IfqProbe" \
    "$HERE/tests/ifqprobe.c" "$OUT/libgfotcompat.a" -lsocket

. "$HERE/torrent.lock"
E="$WORK/gfopenevent-$GFOPENEVENT_TAG.zip"
[ -f "$E" ] || curl -fsSL -o "$E" "$GFOPENEVENT_URL"
echo "$GFOPENEVENT_SHA256  $E" | sha256sum -c - >/dev/null || { echo "event.library archive checksum differs"; exit 1; }
python3 -c 'import sys,zipfile; open(sys.argv[2],"wb").write(zipfile.ZipFile(sys.argv[1]).read("GFOpenEvent/Libs/event.library"))' \
    "$E" "$OUT/event.library"
(cd "$OUT" && sha256sum libtorrent-rasterbar.a libgfotcompat.a OneTask IfqProbe event.library)
