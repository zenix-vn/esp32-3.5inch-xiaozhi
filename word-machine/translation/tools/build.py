#!/usr/bin/env python3
"""Pack translated entries into a new resource file. Usage: python build.py OUT.bin [--keep-synonyms]

Everything that is Vietnamese is drawn as two-letter cells under rare Hanzi (see ../../tools/cells.py); the plain
Vietnamese the translators write never reaches the card.
"""
import json
import re
import struct
import sys
from pathlib import Path

import numpy as np

sys.stdout.reconfigure(encoding='utf-8', errors='replace')
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT.parent / 'tools'))
from entry import join, split  # noqa: E402
import cells  # noqa: E402

BK = ROOT.parent / 'backup'
BASE, TABLE, COUNT, P = 38800838, 38800842, 36893, 1 << 18
TIER_A_LAST = 8369  # D00001..D08369 are the words that appear in a book; the rest are dictionary-only
TEXT_LO, TEXT_HI = 37400000, 44560000   # titles and dictionary, the only region with Chinese text the firmware reads
raw = np.fromfile(BK / 'Gres260909.bin', dtype=np.uint8)
key = np.load(BK / 'key_fixed.npy')


def ks(o, n):
    return key[np.arange(o, o + n, dtype=np.int64) % P]


def dec(o, n):
    return (raw[o:o + n] ^ ks(o, n)).tobytes()


def load_translations():
    idx = json.loads((ROOT / 'tools/index.json').read_text())
    tr = {}
    for f in sorted((ROOT / 'done').glob('P*.jsonl')):
        for l in f.read_text(encoding='utf-8').splitlines():
            if l.strip():
                r = json.loads(l)
                tr[idx[r['id']]['tbl']] = r
    return tr


def used_hanzi():
    text = dec(TEXT_LO, TEXT_HI - TEXT_LO).decode('utf-8', 'ignore')
    return set(re.findall('[一-鿿]', text))


def apply_ui_cells(res):
    """Hanzi cells redrawn as Vietnamese menu labels (see tools/ui_map.py)."""
    import glyphs
    for ch, g in glyphs.cells().items():
        o = ord(ch) * 28
        res[o:o + 28] = np.frombuffer(glyphs.to_bytes(g), np.uint8)


def apply_titles(res, pool):
    """Book and chapter names: every one is rebuilt from the original so no Latin Extended letter reaches the card."""
    p = ROOT / 'tools/titles_all.json'
    if not p.exists():
        return
    import titles
    done = {}
    d = ROOT / 'done' / 'T001.jsonl'
    if d.exists():
        for l in d.read_text(encoding='utf-8').splitlines():
            if l.strip():
                r = json.loads(l)
                done[r['zh']] = r['vi']
    bad, ok = [], 0
    for off, n, zh in json.loads(p.read_text(encoding='utf-8')):
        vi = done.get(zh) or titles.fit_cells(zh, n)
        if vi is None:
            bad.append(zh)
            continue
        enc = titles.encode(pool, zh, vi)
        b = enc.encode('utf-8')
        if len(b) > n or re.search('[一-鿿]', vi):
            bad.append(zh)
            continue
        b = b.ljust(n, b' ')
        res[off + 2:off + 2 + n] = np.frombuffer(b, np.uint8) ^ ks(off + 2, n)
        ok += 1
    print(f'titles: {ok} rebuilt, {len(bad)} left in Chinese')
    (ROOT / 'out').mkdir(exist_ok=True)
    (ROOT / 'out/titles_left.txt').write_text('\n'.join(bad), encoding='utf-8')


def cap(m):
    """--max-senses N: keep only the first N comma-separated senses of a line (rare senses are listed last)."""
    if '--max-senses' not in sys.argv:
        return m
    n = int(sys.argv[sys.argv.index('--max-senses') + 1])
    parts = m.split(', ')
    return ', '.join(parts[:n]) if len(parts) > n else m


def main():
    keep = '--keep-synonyms' in sys.argv
    tr = load_translations()
    tab = np.frombuffer(dec(TABLE, COUNT * 4), dtype='<u4').astype(np.int64) + BASE
    start = int(tab[0])
    last = int(tab[-1])
    limit = last + 3 + struct.unpack('<H', dec(last + 1, 2))[0]
    pool = cells.Pool(used_hanzi())
    tier_b = set()
    if '--trim-unused' in sys.argv:
        idx = json.loads((ROOT / 'tools/index.json').read_text())
        tier_b = {v['tbl'] for k, v in idx.items() if int(k[1:]) > TIER_A_LAST}
    out, pos = bytearray(), []
    for i, o in enumerate(tab.tolist()):
        t = dec(o, 1)[0]
        n = struct.unpack('<H', dec(o + 1, 2))[0]
        body = dec(o + 3, n)
        if i in tier_b and i not in tr:
            # Words in no book: their English synonym lines are the cheapest bytes to give back to the translations.
            f = split(body.decode('utf-8'))
            body = join(f, rest=[''] * len(f['rest'])).encode('utf-8')
        if i in tr:
            f = split(body.decode('utf-8'))
            r = tr[i]
            rest = f['rest'] if keep else [''] * len(f['rest'])
            meaning = [pool.meaning(cap(m)) if m.strip() else '' for m in r['meaning_vi']]
            example = pool.line(r['example_vi']) if r.get('example_vi', '').strip() and '--no-example' not in sys.argv else ''
            body = join(f, meaning, example, rest).encode('utf-8')
        pos.append(len(out))
        out += bytes([t]) + struct.pack('<H', len(body)) + body
    span = limit - start
    relocate = len(out) > span and '--in-place' not in sys.argv
    if len(out) > span and '--in-place' in sys.argv:
        sys.exit(f'--in-place: {len(out) - span} bytes over')
    print(f'translated {len(tr)}/{COUNT}, dictionary {len(out)} bytes, original span {span}, '
          f'{"over by " + str(len(out) - span) + ", moved to the end of the file" if relocate else "kept in place"}')
    if relocate:
        origin = len(raw)                      # old end of file; already a multiple of the 4 KiB cluster size
        size = -(-(origin + len(out)) // 4096) * 4096
        res = np.zeros(size, np.uint8)
        res[:len(raw)] = raw
    else:
        origin, res = start, raw.copy()
    apply_ui_cells(res)
    apply_titles(res, pool)
    blob = bytes(out) if relocate else bytes(out).ljust(span, bytes(1))
    res[origin:origin + len(blob)] = np.frombuffer(blob, np.uint8) ^ ks(origin, len(blob))
    t = np.array([origin + p - BASE for p in pos], dtype='<u4').tobytes()
    res[TABLE:TABLE + len(t)] = np.frombuffer(t, np.uint8) ^ ks(TABLE, len(t))
    for cp, b in pool.table().items():
        res[cp * 28:cp * 28 + 28] = np.frombuffer(b, np.uint8)
    print(f'cells in use: {len(pool.map)} of {len(pool.free)}')
    res.tofile(sys.argv[1])
    print('wrote', sys.argv[1])


if __name__ == '__main__':
    main()
