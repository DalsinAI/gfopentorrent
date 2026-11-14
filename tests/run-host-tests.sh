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
    -Dencryption=OFF -Ddeprecated-functions=OFF -DCMAKE_CXX_STANDARD=17 \
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
    -Dencryption=OFF -Ddeprecated-functions=OFF -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_DISABLE_FIND_PACKAGE_OpenSSL=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_GnuTLS=TRUE \
    -DCMAKE_CXX_FLAGS="-I$OV -DTORRENT_AMIGA_HOST_TEST -DBOOST_ASIO_DISABLE_THREADS" > "$B1/configure.log"
cmake --build "$B1" --target torrent-rasterbar -j "$JOBS" > "$B1/build.log" 2>&1 \
    || { grep -E 'error' "$B1/build.log" | head -40; exit 1; }
${CXX:-g++} -std=c++17 -O1 -I"$OV" -DTORRENT_AMIGA_HOST_TEST -DBOOST_ASIO_DISABLE_THREADS $(lt_defs "$B1") \
    -I"$LT/include" -I"$HERE/compat" "$HERE/tests/one_task_test.cpp" "$B1/libtorrent-rasterbar.a" \
    "$B/gfot_ifq_host.o" "$B/gfot_ifq_common.o" \
    -pthread -o "$B1/one_task_test"
"$B1/one_task_test"

# ---- PicoTorrent's logging without Boost.Log (compat/boostlog)
sh "$HERE/scripts/fetch-sources.sh" picotorrent
PT="$WORK/picotorrent/src"
# every log line in PicoTorrent must be BOOST_LOG_TRIVIAL(<a level the shim
# has>) and include only trivial.hpp; core/environment.cpp's add_file_log
# setup (Windows-only) and crashpadinitializer.cpp (not built) are the
# exceptions, replaced in the Amiga port
odd=$(grep -rn 'BOOST_LOG\|boost/log\|boost::log' "$PT" \
    | grep -v 'core/environment.cpp:\|crashpadinitializer.cpp:' \
    | grep -v 'BOOST_LOG_TRIVIAL(\(trace\|debug\|info\|warning\|error\|fatal\))' \
    | grep -v '#include <boost/log/trivial.hpp>' || true)
if [ -n "$odd" ]; then
    echo "FAIL PicoTorrent uses Boost.Log beyond the shim:"; echo "$odd" | head -20; exit 1
fi
echo "PASS PicoTorrent uses only BOOST_LOG_TRIVIAL ($(grep -rc 'BOOST_LOG_TRIVIAL(' "$PT" | awk -F: '{s+=$2} END {print s}') lines)"
${CXX:-g++} -std=c++17 -O1 -Wall -Wextra -I"$HERE/compat/boostlog" "$HERE/tests/log_shim_test.cpp" \
    "$HERE/compat/gfot_log.cpp" -o "$B/log_shim_test"
"$B/log_shim_test"
# PicoTorrent's Environment on AmigaOS (host: GFOT_PROGDIR, GFOT_LANGUAGE)
${CXX:-g++} -std=c++17 -O1 -Wall -Wextra -I"$HERE/compat/boostlog" -I"$HERE/compat" -I"$PT" \
    "$HERE/tests/environment_test.cpp" "$HERE/compat/picotorrent/environment_amiga.cpp" \
    "$HERE/compat/gfot_env.cpp" "$HERE/compat/gfot_log.cpp" -o "$B/environment_test"
"$B/environment_test"
# PicoTorrent's path joins: with patches/picotorrent applied, no fs::path "/"
# join is left in the sources the build takes (core/environment.cpp and
# core/utils.cpp are replaced, crashpad, core/geoip/ and src/l10n/ are not
# built for the Amiga)
odd=$(grep -rnE ' / ' --include=*.cpp "$PT" | grep -E 'fs::path|Path\(\)|savePath|path\(\)' \
    | grep -v 'core/environment.cpp:\|crashpadinitializer.cpp:\|core/geoip/\|/l10n/' || true)
if [ -n "$odd" ]; then
    echo "FAIL PicoTorrent joins paths with fs::path's /:"; echo "$odd" | head -20; exit 1
fi
echo "PASS PicoTorrent joins no paths with fs::path's /"
# PicoTorrent's Utils on AmigaOS (host: openAndSelect logs the drawer)
${CXX:-g++} -std=c++17 -O1 -Wall -Wextra -I"$HERE/compat/boostlog" -I"$HERE/compat" -I"$PT" \
    "$HERE/tests/utils_test.cpp" "$HERE/compat/picotorrent/utils_amiga.cpp" \
    "$HERE/compat/gfot_env.cpp" "$HERE/compat/gfot_log.cpp" -o "$B/utils_test"
"$B/utils_test"
# The language list's names (compat/gfot_lang.cpp): one for each lang/*.json
${CXX:-g++} -std=c++17 -O1 -Wall -Wextra -I"$HERE/compat" "$HERE/tests/lang_test.cpp" \
    "$HERE/compat/gfot_lang.cpp" -o "$B/lang_test"
"$B/lang_test" "$WORK/picotorrent/lang"
# patch 0002 leaves no Windows header in translator.hpp outside _WIN32
if ! awk '/#include <Windows.h>/ && prev != "#ifdef _WIN32" { bad = 1 } { prev = $0 } END { exit bad }' \
    "$PT/picotorrent/ui/translator.hpp"; then
    echo "FAIL translator.hpp includes Windows.h outside _WIN32"; exit 1
fi
echo "PASS translator.hpp keeps Windows.h inside _WIN32"
# patch 0003: no std::string is made straight from a wxString getter (the
# toolkit has wx's implicit conversion off)
odd=$(grep -rnE 'std::string [A-Za-z_]+ = [^;]*\.(GetText|GetNextToken|GetValue|GetString)\(\);' --include=*.cpp "$PT" || true)
if [ -n "$odd" ]; then
    echo "FAIL a std::string is made from a wxString without ToStdString():"; echo "$odd" | head -20; exit 1
fi
echo "PASS every std::string from a wxString goes through ToStdString()"
# One copy at a time over ARexx, and the tray settings (patch 0004)
${CXX:-g++} -std=c++17 -O1 -Wall -Wextra -I"$HERE/compat" "$HERE/tests/single_copy_test.cpp" \
    "$HERE/compat/gfot_rexx.cpp" "$HERE/compat/gfot_settings.cpp" -o "$B/single_copy_test"
"$B/single_copy_test"
# free space on the save volume, in place of GetDiskFreeSpaceEx (patch 0004)
${CXX:-g++} -std=c++17 -O1 -Wall -Wextra -I"$HERE/compat" "$HERE/tests/disk_space_test.cpp" \
    "$HERE/compat/gfot_disk.cpp" -o "$B/disk_space_test"
"$B/disk_space_test" "$B"
# the stove scripts parse
for f in "$HERE"/tools/*/*.sh "$HERE"/scripts/*.sh; do sh -n "$f" || { echo "FAIL $f does not parse"; exit 1; }; done
echo "PASS the build scripts parse"
# the wxWidgets notice in the README and the About box (patch 0010)
${CXX:-g++} -std=c++17 -O1 -Wall -Wextra -I"$HERE/compat" "$HERE/tests/wx_notice_test.cpp" -o "$B/wx_notice_test"
"$B/wx_notice_test" "$HERE"
