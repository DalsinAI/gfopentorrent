#!/bin/sh
# GFOpenTorrent: fetch the sources torrent.lock pins into WORK (default
# ~/.cache/gfopentorrent/src), check each commit, and apply patches/<name>/.
#   WORK=dir scripts/fetch-sources.sh [libtorrent] [picotorrent]
# A source already there at the pinned commit is reset and patched again.
# Copyright (c) 2026 AmiCradle Team. MIT licence.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
. "$HERE/torrent.lock"
WORK=${WORK:-"$HOME/.cache/gfopentorrent/src"}
mkdir -p "$WORK"

get() { # name url tag commit
    d="$WORK/$1"
    if [ ! -d "$d/.git" ]; then
        git -c advice.detachedHead=false clone -q --depth 1 -b "$3" "$2" "$d"
    fi
    got=$(git -C "$d" rev-parse HEAD)
    [ "$got" = "$4" ] || { echo "$1: $3 is $got, torrent.lock pins $4"; exit 1; }
    git -C "$d" reset -q --hard "$4"
    git -C "$d" clean -qfdx
}

patch_all() { # name
    for p in "$HERE/patches/$1"/*.patch; do
        [ -f "$p" ] || continue
        git -C "$WORK/$1" apply --whitespace=nowarn "$p" || { echo "$1: $(basename "$p") does not apply"; exit 1; }
        echo "$1: $(basename "$p")"
    done
}

want=${*:-"libtorrent picotorrent"}
for n in $want; do
    case "$n" in
    libtorrent)
        get libtorrent "$LIBTORRENT_URL" "$LIBTORRENT_TAG" "$LIBTORRENT_COMMIT"
        git -C "$WORK/libtorrent" submodule update -q --init --depth 1 deps/try_signal
        t=$(git -C "$WORK/libtorrent/deps/try_signal" rev-parse HEAD)
        [ "$t" = "$TRY_SIGNAL_COMMIT" ] || { echo "try_signal is $t, torrent.lock pins $TRY_SIGNAL_COMMIT"; exit 1; }
        patch_all libtorrent ;;
    picotorrent)
        get picotorrent "$PICOTORRENT_URL" "$PICOTORRENT_TAG" "$PICOTORRENT_COMMIT"
        patch_all picotorrent ;;
    *) echo "unknown source $n"; exit 1 ;;
    esac
done
