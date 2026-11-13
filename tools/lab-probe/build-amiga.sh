#!/bin/sh
# GFOpenTorrent: build IfqProbe (tests/ifqprobe.c with compat/gfot_ifq_amiga.c)
# on the stove, for a lab.
#   STOVE=.../stoves/os32-gcc16-open6 OUT=out tools/lab-probe/build-amiga.sh
# Copyright (c) 2026 AmiCradle Team. MIT licence.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open6"}
PATH="${TOOLCHAIN:-$STOVE/prefix}/bin:$PATH"
OUT=${OUT:-out}
mkdir -p "$OUT"
m68k-amigaos-gcc -noixemul -m68040 -O2 -std=gnu99 -Wall -I"$HERE/compat" -o "$OUT/IfqProbe" \
    "$HERE/tests/ifqprobe.c" "$HERE/compat/gfot_ifq_amiga.c" "$HERE/compat/gfot_ifq_common.c" -lsocket
sha256sum "$OUT/IfqProbe"
