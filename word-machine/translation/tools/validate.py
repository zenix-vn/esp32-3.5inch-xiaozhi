#!/usr/bin/env python3
"""Check translated parts. Usage: python validate.py [P001 G001 ...]; no arguments checks everything in done/."""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CJK = re.compile('[　-〿㐀-鿿＀-￯]')
BAD = re.compile('[^\\x20-\\x7eÀ-ÿĀ-ſƠ-ưẠ-ỹ…–]')
sys.stdout.reconfigure(encoding='utf-8', errors='replace')


def load(p):
    return [json.loads(l) for l in p.read_text(encoding='utf-8').splitlines() if l.strip()]


def check(name):
    src, done = ROOT / 'source' / f'{name}.jsonl', ROOT / 'done' / f'{name}.jsonl'
    if not done.exists():
        return None
    errs, warns = [], []
    s = {r['id']: r for r in load(src)}
    try:
        d = load(done)
    except json.JSONDecodeError as e:
        return [f'{name}: invalid JSON: {e}'], []
    ids = [r.get('id') for r in d]
    if len(set(ids)) != len(ids):
        errs.append('duplicate ids')
    errs += [f'{m}: missing' for m in sorted(set(s) - set(ids))]
    errs += [f'{m}: unknown id' for m in sorted(set(ids) - set(s))]
    for r in d:
        o = s.get(r.get('id'))
        if not o:
            continue
        i = r['id']
        if name[0] == 'P':
            m, e = r.get('meaning_vi'), r.get('example_vi', '')
            if not isinstance(m, list) or len(m) != len(o['meaning_zh']):
                errs.append(f'{i}: meaning_vi must have {len(o["meaning_zh"])} lines')
                continue
            for a, b in zip(m, o['meaning_zh']):
                if bool(a.strip()) != bool(b.strip()):
                    errs.append(f'{i}: empty line mismatch')
            if bool(e.strip()) != bool(o['example_zh'].strip()):
                errs.append(f'{i}: example_vi must be empty exactly when example_zh is empty')
            text = '\n'.join(m) + e
            if len(text.encode()) > o['soft_max_bytes']:
                warns.append(f'{i}: {len(text.encode())} > {o["soft_max_bytes"]} bytes')
        else:
            text = r.get('vi', '')
            if not text.strip():
                errs.append(f'{i}: vi is empty')
            if name[0] == 'G' and len(text) > o['max_chars']:
                errs.append(f'{i}: {len(text)} > {o["max_chars"]} chars')
            if name[0] == 'T' and len(text.encode()) > o['max_bytes']:
                errs.append(f'{i}: {len(text.encode())} > {o["max_bytes"]} bytes')
        if CJK.search(text):
            errs.append(f'{i}: Chinese text left')
        b = BAD.search(text.replace('\n', ' '))
        if b:
            errs.append(f'{i}: unsupported character {b.group()!r}')
    return errs, warns


def main():
    names = sys.argv[1:] or sorted(p.stem for p in (ROOT / 'done').glob('*.jsonl'))
    fail = 0
    for n in names:
        r = check(n)
        if r is None:
            print(n, 'has no done file')
            fail = 1
            continue
        print(f'{n}: {"OK" if not r[0] else "FAILED"}, {len(r[0])} errors, {len(r[1])} over the soft limit')
        for x in (r[0] + r[1])[:30]:
            print('  ', x)
        fail |= bool(r[0])
    return fail


if __name__ == '__main__':
    sys.exit(main())
