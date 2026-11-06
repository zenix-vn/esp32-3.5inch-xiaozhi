#!/usr/bin/env python3
"""List what is still Chinese in a built resource file. Usage: python tools/list_untranslated.py out/res_full.bin

Writes untranslated.csv (every entry, most visible first) and UNTRANSLATED.md (the summary). An entry counts as
translated only if its meaning differs from the original bytes in the built file, so the list reflects the card image
rather than which files exist in done/.
"""
import csv
import json
import re
import struct
import sys
from collections import Counter
from pathlib import Path

import numpy as np

sys.stdout.reconfigure(encoding='utf-8', errors='replace')
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
from entry import split  # noqa: E402

BK = ROOT.parent / 'backup'
BASE, TABLE, COUNT, P = 38800838, 38800842, 36893, 1 << 18
CJK = re.compile('[一-鿿]')
key = np.load(BK / 'key_fixed.npy')


class Res:
    def __init__(self, path):
        self.a = np.fromfile(path, dtype=np.uint8)

    def dec(self, o, n):
        return (self.a[o:o + n] ^ key[np.arange(o, o + n, dtype=np.int64) % P]).tobytes()

    def entries(self):
        tab = np.frombuffer(self.dec(TABLE, COUNT * 4), dtype='<u4').astype(np.int64) + BASE
        for o in tab.tolist():
            n = struct.unpack('<H', self.dec(o + 1, 2))[0]
            yield split(self.dec(o + 3, n).decode('utf-8'))


def main():
    orig = list(Res(BK / 'Gres260909.bin').entries())
    built = list(Res(sys.argv[1]).entries())
    idx = json.loads((ROOT / 'tools/index.json').read_text())
    with (ROOT / 'all_entries.csv').open(encoding='utf-8-sig') as f:
        rows = list(csv.DictReader(f))

    left, nothing, done = [], 0, 0
    parts = Counter()
    for r in rows:
        t = idx[r['id']]['tbl']
        o, b = orig[t], built[t]
        if o['word'] != b['word']:
            sys.exit(f"{r['id']}: table order mismatch, {o['word']} vs {b['word']}")
        if not CJK.search('\n'.join(o['meaning']) + o['example_zh']):
            nothing += 1
        elif o['meaning'] != b['meaning']:
            done += 1
        else:
            left.append(r)
            parts[r['part']] += 1

    with (ROOT / 'untranslated.csv').open('w', encoding='utf-8-sig', newline='') as f:
        w = csv.DictWriter(f, fieldnames=['id', 'part', 'tier', 'word', 'ipa', 'meaning_zh', 'example_en', 'example_zh'],
                           extrasaction='ignore')
        w.writeheader()
        w.writerows(left)

    a = [r for r in left if r['tier'] == 'A']
    b = [r for r in left if r['tier'] == 'B']
    titles = [l for l in (ROOT / 'out/titles_left.txt').read_text(encoding='utf-8').splitlines() if l.strip()]
    gloss = sum(1 for l in (ROOT / 'source/G001.jsonl').read_text(encoding='utf-8').splitlines() if l.strip())
    pa = sorted({r['part'] for r in a})  # the part where tier A ends holds both tiers
    pb = sorted({r['part'] for r in b})
    md = [
        '# Danh sách chưa dịch', '',
        f'Sinh tự động từ `{Path(sys.argv[1]).name}` bằng `python tools/list_untranslated.py`, đừng sửa tay.', '',
        '| Nhóm | Chưa dịch | Phần | Ghi chú |', '|---|---:|---|---|',
        f'| A: từ có trong sách | {len(a):,} | {min(pa)}–{max(pa)} | Làm trước, người dùng gặp khi học |' if pa else '| A | 0 | | |',
        f'| B: chỉ có trong từ điển | {len(b):,} | {min(pb)}–{max(pb)} | Chưa rõ máy có hiện không |' if pb else '| B | 0 | | |',
        f'| T: tên sách, tên chương | {len(titles)} | T001 | Xem bên dưới |',
        f'| G: nghĩa cụm từ trong sách | {gloss:,} | G001 | `build.py` hiện chưa nạp được nhóm này |', '',
        f'Đã dịch trên bản dựng: {done:,} mục. Không có chữ Hán nên không cần dịch: {nothing:,} mục.', '',
        'Danh sách đầy đủ từng từ: [untranslated.csv](untranslated.csv), xếp theo mức xuất hiện trong sách.', '',
        '## 50 từ gặp nhiều nhất còn chưa dịch', '',
        ', '.join(r['word'] for r in a[:50]), '',
        '## Tên sách còn tiếng Trung', '',
    ] + [f'- {t}' for t in titles] + ['']
    (ROOT / 'UNTRANSLATED.md').write_text('\n'.join(md), encoding='utf-8')
    print(f'done {done}, no Chinese {nothing}, left A {len(a)} B {len(b)}, titles {len(titles)}, gloss {gloss}')
    print('first:', [r['word'] for r in a[:12]])


if __name__ == '__main__':
    main()
