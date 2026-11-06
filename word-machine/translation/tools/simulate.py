#!/usr/bin/env python3
"""Draw dictionary pages and titles the way the device does, from a built resource file.
Usage: simulate.py RES.bin OUT.png word [word ...]   (words are headwords; titles are shown with --titles N)
Reads the offset table, the entry bytes and the 14x16 font cells from the file itself, so it checks the whole chain.
"""
import json
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT.parent / 'tools'))
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
BASE, TABLE, COUNT, P = 38800838, 38800842, 36893, 1 << 18
res = np.fromfile(sys.argv[1], dtype=np.uint8)
key = np.load(ROOT.parent / 'backup/key_fixed.npy')
ascii_font = ImageFont.truetype('C:/Windows/Fonts/consola.ttf', 12)


def dec(o, n):
    return (res[o:o + n] ^ key[np.arange(o, o + n, dtype=np.int64) % P]).tobytes()


def cell(cp):
    b = res[cp * 28:cp * 28 + 28]
    g = np.zeros((16, 14), np.uint8)
    for p in range(2):
        for x in range(14):
            v = int(b[p * 14 + x])
            for y in range(8):
                g[p * 8 + y, x] = v >> y & 1
    return g


def asc(ch):
    im = Image.new('1', (7, 16), 0)
    d = ImageDraw.Draw(im)
    d.fontmode = '1'
    d.text((0, 13), ch, font=ascii_font, fill=1, anchor='ls')
    return np.array(im, dtype=np.uint8)


def strip(text):
    parts = [asc(c) if ord(c) < 0x80 else cell(ord(c)) for c in text if c != '\n']
    return np.concatenate(parts, axis=1) if parts else np.zeros((16, 0), np.uint8)


def wrap(text, units=33):
    lines, cur, w = [], [], 0
    for ch in text:
        cw = 1 if ord(ch) < 0x80 else 2
        if w + cw > units:
            lines.append(''.join(cur))
            cur, w = [], 0
        cur.append(ch)
        w += cw
    lines.append(''.join(cur))
    return lines


def entries():
    tab = np.frombuffer(dec(TABLE, COUNT * 4), dtype='<u4').astype(np.int64) + BASE
    for o in tab.tolist():
        n = struct.unpack('<H', dec(o + 1, 2))[0]
        yield dec(o + 3, n).decode('utf-8')


def main():
    out, words = sys.argv[2], [w for w in sys.argv[3:] if not w.startswith('--')]
    want = {w: None for w in words}
    for t in entries():
        h = t.split('\n')[0]
        if h in want and want[h] is None:
            want[h] = t
    blocks = []
    for w, t in want.items():
        if t is None:
            continue
        ls = t.split('\n')
        i = 2 if ls[1].startswith('[') else 1
        j = next(k for k, l in enumerate(ls) if l.startswith(';'))
        blocks.append([f'== {w} {ls[1] if i == 2 else ""}'] + [' '.join(x for x in ls[i:j] if x)] + [ls[j][1:]] + [ls[j + 1]])
    ti = [json.loads(l) for l in (ROOT / 'tools/titles_all.json').read_text(encoding='utf-8').splitlines()] if False else json.loads((ROOT / 'tools/titles_all.json').read_text(encoding='utf-8'))
    extra = []
    if '--titles' in sys.argv:
        step = max(1, len(ti) // 24)
        for off, n, zh in ti[::step][:24]:
            extra.append(dec(off + 2, n).decode('utf-8', 'replace').rstrip())
    H = sum(len(wrap(t)) * 18 + 4 for b in blocks for t in b if t) + len(extra) * 20 + 40
    img = np.zeros((H, 240), np.uint8)
    y = 2
    for b in blocks:
        for t in b:
            if not t:
                continue
            for ln in wrap(t):
                s = strip(ln)
                img[y:y + 16, :min(240, s.shape[1])] = s[:, :240]
                y += 18
            y += 4
        y += 8
    for t in extra:
        s = strip(t)
        img[y:y + 16, :min(240, s.shape[1])] = s[:, :240]
        y += 20
    Image.fromarray((1 - img[:y + 4]) * 255).resize((240 * 3, (y + 4) * 3), Image.NEAREST).save(out)
    print('saved', out)


if __name__ == '__main__':
    main()
