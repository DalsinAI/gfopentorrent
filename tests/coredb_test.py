#!/usr/bin/env python3
# GFOpenTorrent: tools/stove/gen-coredb.py makes the same translations table
# as PicoTorrent's own tool (src/l10n/main.cpp), with every string of every
# lang/*.json, and the query PicoTorrent's Translator runs reads it back.
# Usage: coredb_test.py REPO PICOTORRENT_ROOT
# Copyright (c) 2026 AmiCradle Team. MIT licence.
import json
import os
import sqlite3
import sys
import tempfile

repo, pt = sys.argv[1], sys.argv[2]
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(repo, "tools", "stove"))
gen = __import__("gen-coredb")

passed = failed = 0


def check(ok, what):
    global passed, failed
    print(("PASS " if ok else "FAIL ") + what)
    if ok:
        passed += 1
    else:
        failed += 1


l10n = open(os.path.join(pt, "src", "l10n", "main.cpp"), encoding="utf-8").read()
check('"%s"' % gen.TABLE_SQL in l10n, "the table is PicoTorrent's own (src/l10n/main.cpp)")
check('"%s"' % gen.INSERT_SQL in l10n, "the insert is PicoTorrent's own")

lang = os.path.join(pt, "lang")
want = {}
for name in os.listdir(lang):
    if name.endswith(".json"):
        with open(os.path.join(lang, name), encoding="utf-8-sig") as f:
            want[name[:-5]] = json.load(f)

with tempfile.TemporaryDirectory() as d:
    out = os.path.join(d, "coredb.sqlite")
    counts = gen.build(lang, out)
    check(counts == {k: len(v) for k, v in want.items()}, "every language, every string (%d)" % len(counts))
    db = sqlite3.connect(out)
    rows = db.execute("SELECT locale, key, value FROM translations ORDER BY locale, key ASC").fetchall()
    check(len(rows) == sum(len(v) for v in want.values()), "the Translator's query reads them all")
    got = {}
    for loc, key, val in rows:
        got.setdefault(loc, {})[key] = val
    diff = sorted(l for l in want if got.get(l, {}) != want[l])  # af-ZA and da-DK are empty
    check(not diff, "each value as in its .json, accents included %s" % diff[:3])
    bom = [n[:-5] for n in os.listdir(lang) if n.endswith(".json") and open(os.path.join(lang, n), "rb").read(3) == b"\xef\xbb\xbf"]
    check(all(got.get(b) == want[b] for b in bom), "files with a byte order mark read too (%d)" % len(bom))
    check("en-US" in got and "name" in got["en-US"], "en-US has the column names")
    db.close()
    gen.build(lang, out)
    db = sqlite3.connect(out)
    check(db.execute("SELECT COUNT(*) FROM translations").fetchone()[0] == len(rows), "rebuilt, no rows doubled")
    db.close()

build = open(os.path.join(repo, "tools", "stove", "build-picotorrent.sh"), encoding="utf-8").read()
check("gen-coredb.py" in build and "coredb.sqlite" in build, "the stove build makes coredb.sqlite beside the program")

print("%d passed, %d failed" % (passed, failed))
sys.exit(1 if failed else 0)
