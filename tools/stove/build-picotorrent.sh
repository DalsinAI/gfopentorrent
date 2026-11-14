#!/bin/sh
# GFOpenTorrent: PicoTorrent's sources compiled for AmigaOS on the stove,
# one by one, against the GFOpenWX Toolkit SDK, with this repository's
# patches and its Amiga files in place of the Windows ones. Each source's
# object and errors go to WORK/picotorrent-obj; the report says which
# compile. Linking comes once they all do.
#   STOVE=.../stoves/os32-gcc16-open6 WX=<toolkit sdk> GFOPENBOOST_SDK=<sdk> \
#       SQLITE_INCLUDE=<dir with sqlite3.h> tools/stove/build-picotorrent.sh
# WX is a GFOpenWX Toolkit sdk/ (bin/wx-config). Exit 0 when every source
# compiles, 3 when some do not yet (the report lists them).
# Not built for the Amiga: crashpad, geoip, l10n (a build-machine tool),
# resources.rc (Windows resources) and the Windows-only openfiledialog
# (wx's own file dialog is used).
# Copyright (c) 2026 AmiCradle Team. MIT licence.
set -eu
renice -n 19 $$ >/dev/null 2>&1 || true
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
. "$HERE/torrent.lock"
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open6"}
TC=${TOOLCHAIN:-"$STOVE/prefix"}
WX=${WX:?set WX to a GFOpenWX Toolkit sdk/ directory}
SDK=${GFOPENBOOST_SDK:?set GFOPENBOOST_SDK to an unpacked GFOpenBoost SDK}
SQLITE_INCLUDE=${SQLITE_INCLUDE:?set SQLITE_INCLUDE to a directory with sqlite3.h}
WORK=${WORK:-"$HOME/.cache/gfopentorrent"}
PATH="$TC/bin:$PATH"
[ -x "$WX/bin/wx-config" ] || { echo "$WX has no bin/wx-config"; exit 1; }
[ -f "$SQLITE_INCLUDE/sqlite3.h" ] || { echo "no sqlite3.h in $SQLITE_INCLUDE"; exit 1; }
WORK="$WORK/src" sh "$HERE/scripts/fetch-sources.sh" libtorrent picotorrent fmt json
S="$WORK/src"; P="$S/picotorrent"
# the Amiga files in place of the Windows ones
cp "$HERE/compat/picotorrent/environment_amiga.cpp" "$P/src/picotorrent/core/environment.cpp"
cp "$HERE/compat/picotorrent/utils_amiga.cpp" "$P/src/picotorrent/core/utils.cpp"
cp "$HERE/compat/picotorrent/ipc_server_amiga.cpp" "$P/src/picotorrent/ipc/server.cpp"
cp "$HERE/compat/picotorrent/httpclient_amiga.cpp" "$P/src/picotorrent/http/httpclient.cpp"
sed -e "s/@GITVERSION_VAR_SHORTSHA@/$(echo "$PICOTORRENT_COMMIT" | cut -c1-8)/g" -e 's/@[A-Za-z_]*@/0/g' \
    "$P/src/picotorrent/buildinfo.cpp.in" > "$P/src/picotorrent/buildinfo.cpp"
CXXF="-noixemul -m68040 -O2 -std=gnu++17 $("$WX/bin/wx-config" --cxxflags) -DFMT_HEADER_ONLY -DBOOST_ALL_NO_LIB
 -DPICO_VERSION_MAJOR=0 -DPICO_VERSION_MINOR=25 -DPICO_VERSION_PATCH=0 -I$P/include -I$P/src
 -I$HERE/compat -I$HERE/compat/boostlog -I$S/libtorrent/include -I$SDK/include -I$S/fmt/include
 -I$S/json/include -idirafter $SQLITE_INCLUDE"
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
for c in gfot_env gfot_log gfot_lang gfot_rexx gfot_settings gfot_notray gfot_disk gfot_db gfot_http gfot_netif; do compile "$HERE/compat/$c.cpp" "$O/$c.o"; done
compile "$O/gfot_migrations.cpp" "$O/gfot_migrations.o"
echo "picotorrent: $ok compiled, $bad not yet"
[ "$bad" = 0 ] || exit 3
