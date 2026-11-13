#!/bin/sh
# GFOpenTorrent: libtorrent for AmigaOS on the stove, and the lab tests.
#   STOVE=.../stoves/os32-gcc16-open6 GFOPENBOOST_SDK=<sdk dir> OUT=out \
#       JOBS=2 tools/stove/build-amiga.sh
# GFOPENBOOST_SDK is an unpacked GFOpenBoost SDK (left as it is: the build
# uses a copy). Patch 0005 (gfopenboost #8, async_resolve on one task)
# must be in it. When its headers lack it (rc3), the copy gets the patch,
# pinned in torrent.lock, and GFOPENBOOST_ASIO_LIB must name a
# libboost_asio.a built with it (#8's tools/asio-check), which replaces
# the copy's. Builds, with no OpenSSL and no GnuTLS:
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
. "$HERE/torrent.lock"
# the SDK copy, with patch 0005 when it lacks it
SC="$WORK/gfopenboost-sdk"
rm -rf "$SC"
mkdir -p "$SC"
cp -R "$SDK/." "$SC/"
RTP=boost/asio/detail/impl/resolver_thread_pool.ipp
if ! grep -q 'BOOST_ASIO_DISABLE_THREADS' "$SC/include/$RTP"; then
    : "${GFOPENBOOST_ASIO_LIB:?$SDK lacks patch 0005: set GFOPENBOOST_ASIO_LIB to a libboost_asio.a built with it (gfopenboost #8, tools/asio-check)}"
    P="$WORK/gfopenboost-0005.patch"
    [ -f "$P" ] || curl -fsSL -o "$P" "$GFOPENBOOST_0005_URL"
    echo "$GFOPENBOOST_0005_SHA256  $P" | sha256sum -c - >/dev/null || { echo "patch 0005: checksum differs"; exit 1; }
    patch -s -d "$SC/include" -p1 --no-backup-if-mismatch < "$P"
    cp "$GFOPENBOOST_ASIO_LIB" "$SC/lib/libboost_asio.a"
    echo "SDK copy: patch 0005 applied, libboost_asio.a from $GFOPENBOOST_ASIO_LIB"
fi
SDK="$SC"

WORK="$WORK/src" sh "$HERE/scripts/fetch-sources.sh" libtorrent
LT="$WORK/src/libtorrent"
B="$WORK/amiga-build"
BOOST_CMAKE=$(ls -d "$SDK"/lib/cmake/Boost-* | head -1)
cmake -G "Unix Makefiles" -S "$LT" -B "$B" -DCMAKE_TOOLCHAIN_FILE="$HERE/cmake/toolchain-m68k-amigaos.cmake" \
    -DAMIGA_TOOLCHAIN="$TC" -DGFOPENBOOST_SDK="$SDK" -DBoost_DIR="$BOOST_CMAKE" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 -DBUILD_SHARED_LIBS=OFF \
    -Dencryption=OFF -Dtry-signal=OFF -Ddeprecated-functions=OFF -Dlogging=OFF \
    -DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_GnuTLS=TRUE \
    > "$B.configure.log" 2>&1 || { tail -40 "$B.configure.log"; exit 1; }
cmake --build "$B" --target torrent-rasterbar -j "$JOBS" > "$B.build.log" 2>&1 \
    || { grep -m5 ' error:' "$B.build.log"; exit 1; }
# nm's own failure must not pass as "no GnuTLS"
m68k-amigaos-nm -A "$B/libtorrent-rasterbar.a" > "$B.nm.txt" 2>"$B.nm.err" \
    || { echo "nm could not read libtorrent: $(head -1 "$B.nm.err")"; exit 1; }
[ -s "$B.nm.txt" ] || { echo "nm listed no symbols"; exit 1; }
if grep -qi gnutls "$B.nm.txt"; then
    echo "libtorrent links GnuTLS"; exit 1
fi
if grep -q '_ZN3sig' "$B.nm.txt"; then
    echo "libtorrent still has try_signal"; exit 1
fi
cp "$B/libtorrent-rasterbar.a" "$OUT/"

CF="-noixemul -m68040 -O2"
for c in gfot_ifq_amiga gfot_ifq_common; do
    m68k-amigaos-gcc $CF -std=gnu99 -Wall -I"$HERE/compat" -c "$HERE/compat/$c.c" -o "$B/$c.o"
done
rm -f "$OUT/libgfotcompat.a"
m68k-amigaos-ar rcs "$OUT/libgfotcompat.a" "$B/gfot_ifq_amiga.o" "$B/gfot_ifq_common.o"

LIBS="$OUT/libtorrent-rasterbar.a $OUT/libgfotcompat.a -L$SDK/lib -lboost_asio -lsocket -lpthread -latomic -lm"
# libtorrent's own compile definitions, so the test sees its structs
LTDEFS=$(sed -n 's/^CXX_DEFINES = //p' "$B/CMakeFiles/torrent-rasterbar.dir/flags.make" \
    | tr ' ' '\n' | grep -v '^-DTORRENT_BUILDING_LIBRARY$' | tr '\n' ' ')
[ -n "$LTDEFS" ] || { echo "no definitions in libtorrent's flags.make"; exit 1; }
m68k-amigaos-g++ $CF -std=gnu++17 -DBOOST_ASIO_DISABLE_THREADS -DBOOST_ASIO_SEPARATE_COMPILATION $LTDEFS \
    -I"$SDK/include" -I"$LT/include" -I"$HERE/compat" \
    -o "$OUT/OneTask" "$HERE/tests/one_task_test.cpp" $LIBS 2> "$B.link.log" \
    || { grep -E 'undefined|error' "$B.link.log" | sort -u | head -40; exit 1; }
m68k-amigaos-gcc $CF -std=gnu99 -Wall -I"$HERE/compat" -o "$OUT/IfqProbe" \
    "$HERE/tests/ifqprobe.c" "$OUT/libgfotcompat.a" -lsocket

E="$WORK/gfopenevent-$GFOPENEVENT_TAG.zip"
[ -f "$E" ] || curl -fsSL -o "$E" "$GFOPENEVENT_URL"
echo "$GFOPENEVENT_SHA256  $E" | sha256sum -c - >/dev/null || { echo "event.library archive checksum differs"; exit 1; }
python3 -c 'import sys,zipfile; open(sys.argv[2],"wb").write(zipfile.ZipFile(sys.argv[1]).read("GFOpenEvent/Libs/event.library"))' \
    "$E" "$OUT/event.library"
(cd "$OUT" && sha256sum libtorrent-rasterbar.a libgfotcompat.a OneTask IfqProbe event.library)
