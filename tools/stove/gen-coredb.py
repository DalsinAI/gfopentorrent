#!/usr/bin/env python3
# GFOpenTorrent: PicoTorrent's translations database, coredb.sqlite, made on
# the build machine from PicoTorrent's lang/*.json. On Windows the
# PicoTorrent-coredb tool (src/l10n/main.cpp) does this after the build; it
# is not built for the Amiga, so this makes the same table:
#   translations(id, locale, key, value), one row per locale and key,
#   the locale being the file's name without .json.
# Some of the .json files start with a UTF-8 byte order mark; it is skipped.
# Usage: gen-coredb.py LANG_DIR OUT_FILE
# Copyright (c) 2026 AmiCradle Team. MIT licence.
import json
import os
import sqlite3
import sys

TABLE_SQL = ("CREATE TABLE translations (id INTEGER PRIMARY KEY, locale TEXT NOT NULL, "
             "key TEXT NOT NULL, value TEXT, UNIQUE(locale, key));")
INSERT_SQL = "INSERT INTO translations (locale, key, value) VALUES (?, ?, ?);"


def build(lang_dir, out_file):
    if os.path.exists(out_file):
        os.remove(out_file)
    db = sqlite3.connect(out_file)
    db.execute(TABLE_SQL)
    counts = {}
    for name in sorted(os.listdir(lang_dir)):
        if not name.endswith(".json"):
            continue
        loc = name[:-len(".json")]
        with open(os.path.join(lang_dir, name), encoding="utf-8-sig") as f:
            items = json.load(f)
        db.executemany(INSERT_SQL, [(loc, k, v) for k, v in items.items()])
        counts[loc] = len(items)
    db.commit()
    db.execute("VACUUM")
    db.close()
    return counts


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.stderr.write("usage: gen-coredb.py LANG_DIR OUT_FILE\n")
        sys.exit(1)
    c = build(sys.argv[1], sys.argv[2])
    print("gen-coredb: %d languages, %d strings" % (len(c), sum(c.values())))
