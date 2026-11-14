#!/bin/sh
# GFOpenTorrent: PicoTorrent's sources compiled for AmigaOS on the stove,
# one by one, against the GFOpenWX Toolkit SDK, with this repository's
# patches and its Amiga files in place of the Windows ones. Each source's
# object and errors go to WORK/picotorrent-obj; the report says which
# compile. When they all do, they are linked into WORK/GFOpenTorrent with
# the Amiga libtorrent, GFOpenBoost, SQLite and the toolkit's libraries.
#   STOVE=.../stoves/os32-gcc16-open8 WX=<toolkit sdk> GFOPENBOOST_SDK=<sdk> \
#       SQLITE_INCLUDE=<dir with sqlite3.h> [SQLITE_LIB=<libsqlite3.a>] \
#       [LT_BUILD=<Amiga libtorrent build dir>] tools/stove/build-picotorrent.sh
# WX is a GFOpenWX Toolkit sdk/ (bin/wx-config). LT_BUILD (default
# WORK/amiga-build) holds libtorrent-rasterbar.a, its gfot_*.o and its
# CMake flags; PicoTorrent is compiled with the same libtorrent defines, as
# they choose its ABI namespace. Exit 0 when it links, 3 when some sources
# do not compile yet (the report lists them), 4 when the link fails (the
# missing symbols are listed).
# Not built for the Amiga: crashpad (a stub that does nothing), geoip, l10n
# (a build-machine tool), resources.rc (Windows resources) and the Windows
# openfiledialog (the same class over wx's own file dialog).
# Copyright (c) 2026 AmiCradle Team. MIT licence.
set -eu
renice -n 19 $$ >/dev/null 2>&1 || true
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
. "$HERE/torrent.lock"
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open8"}
TC=${TOOLCHAIN:-"$STOVE/prefix"}
WX=${WX:?set WX to a GFOpenWX Toolkit sdk/ directory}
SDK=${GFOPENBOOST_SDK:?set GFOPENBOOST_SDK to an unpacked GFOpenBoost SDK}
SQLITE_INCLUDE=${SQLITE_INCLUDE:?set SQLITE_INCLUDE to a directory with sqlite3.h}
WORK=${WORK:-"$HOME/.cache/gfopentorrent"}
SQLITE_LIB=${SQLITE_LIB:-"$SQLITE_INCLUDE/../lib/libsqlite3.a"}
LT_BUILD=${LT_BUILD:-"$WORK/amiga-build"}
PATH="$TC/bin:$PATH"
[ -x "$WX/bin/wx-config" ] || { echo "$WX has no bin/wx-config"; exit 1; }
[ -f "$SQLITE_INCLUDE/sqlite3.h" ] || { echo "no sqlite3.h in $SQLITE_INCLUDE"; exit 1; }
[ -f "$SQLITE_LIB" ] || { echo "no $SQLITE_LIB (set SQLITE_LIB)"; exit 1; }
LT_FLAGS="$LT_BUILD/CMakeFiles/torrent-rasterbar.dir/flags.make"
[ -f "$LT_BUILD/libtorrent-rasterbar.a" ] && [ -f "$LT_FLAGS" ] \
    || { echo "no Amiga libtorrent build in $LT_BUILD (set LT_BUILD)"; exit 1; }
# libtorrent's own defines, less the one that says "building the library"
LT_DEFS=$(sed -n 's/^CXX_DEFINES = //p' "$LT_FLAGS" | tr ' ' '\n' | grep -v '^-DTORRENT_BUILDING_LIBRARY$' | tr '\n' ' ')
WORK="$WORK/src" sh "$HERE/scripts/fetch-sources.sh" libtorrent picotorrent fmt json
S="$WORK/src"; P="$S/picotorrent"
# the Amiga files in place of the Windows ones
cp "$HERE/compat/picotorrent/environment_amiga.cpp" "$P/src/picotorrent/core/environment.cpp"
cp "$HERE/compat/picotorrent/utils_amiga.cpp" "$P/src/picotorrent/core/utils.cpp"
cp "$HERE/compat/picotorrent/ipc_server_amiga.cpp" "$P/src/picotorrent/ipc/server.cpp"
cp "$HERE/compat/picotorrent/httpclient_amiga.cpp" "$P/src/picotorrent/http/httpclient.cpp"
cp "$HERE/compat/picotorrent/pqltorrentfilter_amiga.cpp" "$P/src/picotorrent/ui/filters/pqltorrentfilter.cpp"
sed -e "s/@GITVERSION_VAR_SHORTSHA@/$(echo "$PICOTORRENT_COMMIT" | cut -c1-8)/g" -e 's/@[A-Za-z_]*@/0/g' \
    "$P/src/picotorrent/buildinfo.cpp.in" > "$P/src/picotorrent/buildinfo.cpp"
CXXF="-noixemul -m68040 -O2 -std=gnu++17 $("$WX/bin/wx-config" --cxxflags) -DFMT_HEADER_ONLY -DBOOST_ALL_NO_LIB
 -DPICO_VERSION_MAJOR=0 -DPICO_VERSION_MINOR=25 -DPICO_VERSION_PATCH=0 -I$P/include -I$P/src
 -I$HERE/compat -I$HERE/compat/boostlog -I$S/libtorrent/include -I$SDK/include -I$S/fmt/include
 -I$S/json/include -idirafter $SQLITE_INCLUDE $LT_DEFS"
O="$WORK/picotorrent-obj"
mkdir -p "$O"
# the database migrations, which Windows keeps as resources
sh "$HERE/tools/stove/gen-migrations.sh" "$P" "$O/gfot_migrations.cpp"
# PicoTorrent's own list of sources (CMakeLists.txt), less the ones not
# built here; then this repository's
srcs=$(sed -n '/^add_executable(/,/^)/p' "$P/CMakeLists.txt" | grep -oE '^ *src/picotorrent/[A-Za-z0-9_/]+' \
    | tr -d ' ' | grep -v 'crashpadinitializer\|/win32/\|resources' | sort -u)
ok=0; bad=0
compile() { # source object
    if m68k-amigaos-g++ $CXXF -c "$1" -o "$2" 2> "$2.err"; then ok=$((ok + 1)); else
        bad=$((bad + 1))
        echo "FAIL ${1#"$S"/}: $(grep -m1 'error' "$2.err" | sed 's/^[^ ]* //' | cut -c1-200)"
    fi
}
for f in $srcs; do
    [ -f "$P/$f.cpp" ] || { echo "FAIL $f: no $f.cpp"; bad=$((bad + 1)); continue; }
    compile "$P/$f.cpp" "$O/$(echo "$f" | tr / _).o"
done
for c in gfot_env gfot_log gfot_lang gfot_rexx gfot_settings gfot_notray gfot_disk gfot_db gfot_http gfot_netif gfot_pql gfot_sqlvfs gfot_loop; do compile "$HERE/compat/$c.cpp" "$O/$c.o"; done
compile "$O/gfot_migrations.cpp" "$O/gfot_migrations.o"
for c in openfiledialog_amiga crashpadinitializer_amiga; do compile "$HERE/compat/picotorrent/$c.cpp" "$O/$c.o"; done
if m68k-amigaos-gcc -noixemul -m68040 -O2 -c "$HERE/compat/gfot_posix.c" -o "$O/gfot_posix.o" 2> "$O/gfot_posix.o.err"
then ok=$((ok + 1)); else bad=$((bad + 1)); echo "FAIL compat/gfot_posix.c"; fi
echo "picotorrent: $ok compiled, $bad not yet"
[ "$bad" = 0 ] || exit 3
# The link. GFOpenBoost's Asio and the toolkit's libwxamiga both carry a
# select() over bsdsocket.library's WaitSelect; for sockets they answer the
# same, and the toolkit's also counts a file or the console as ready, so
# that one is kept and Asio's is left out of a copy of libboost_asio.a.
B="$SDK/lib"
cp "$B/libboost_asio.a" "$O/libboost_asio.a"
for m in $(m68k-amigaos-ar t "$O/libboost_asio.a" | grep 'amiga_select'); do m68k-amigaos-ar d "$O/libboost_asio.a" "$m"; done
objs=$(for f in "$O"/*.o; do echo "$f"; done)
if m68k-amigaos-g++ -noixemul -m68040 -o "$WORK/GFOpenTorrent" $objs \
    "$LT_BUILD/libtorrent-rasterbar.a" "$LT_BUILD"/gfot_*.o \
    -Wl,--start-group "$O/libboost_asio.a" "$B/libboost_filesystem.a" "$B/libboost_system.a" \
    "$B/libboost_json.a" "$B/libboost_atomic.a" -Wl,--end-group \
    "$SQLITE_LIB" $("$WX/bin/wx-config" --libs std,net) > "$O/link.log" 2>&1
then
    echo "linked $WORK/GFOpenTorrent ($(wc -c < "$WORK/GFOpenTorrent") bytes)"
    # the translations, which PicoTorrent-coredb makes on Windows
    python3 "$HERE/tools/stove/gen-coredb.py" "$P/lang" "$WORK/coredb.sqlite"
else
    echo "link FAILED; missing:"
    grep -o "undefined reference to \`[^']*'" "$O/link.log" | sort | uniq -c | sort -rn | head -40
    grep 'multiple definition' "$O/link.log" | head -5
    exit 4
fi
