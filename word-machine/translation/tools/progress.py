#!/usr/bin/env python3
"""Regenerate STATUS.md from source/, done/ and claims.csv."""
import csv
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def lines(p):
    return sum(1 for l in p.read_text(encoding='utf-8').splitlines() if l.strip())


def main():
    parts = json.loads((ROOT / 'tools/parts.json').read_text())
    with (ROOT / 'claims.csv').open(encoding='utf-8') as f:
        claims = {r['part']: r for r in csv.DictReader(f)}
    extra = [dict(part=p, tier=p[0], entries=lines(ROOT / 'source' / f'{p}.jsonl')) for p in ('T001', 'G001')]
    order = [r for r in parts if r['tier'] == 'A'] + extra + [r for r in parts if r['tier'] == 'B']
    out, total, done = [], 0, 0
    for r in order:
        total += r['entries']
        p = r['part']
        state = 'chờ'
        if (ROOT / 'done' / f'{p}.jsonl').exists():
            ok = subprocess.run([sys.executable, str(ROOT / 'tools/validate.py'), p], capture_output=True).returncode == 0
            state = 'xong' if ok else 'lỗi'
            done += r['entries'] if ok else 0
        elif p in claims:
            state = 'đang dịch'
        out.append(f"| {p} | {r['tier']} | {r['entries']} | {state} | {claims.get(p, {}).get('agent', '')} |")
    head = (f'# Tiến độ dịch\n\nĐã xong {done}/{total} mục ({done * 100 // total}%). '
            'File này sinh tự động bằng `python tools/progress.py`, đừng sửa tay.\n\n'
            '| Phần | Nhóm | Số mục | Trạng thái | Người nhận |\n|---|---|---|---|---|\n')
    (ROOT / 'STATUS.md').write_text(head + '\n'.join(out) + '\n', encoding='utf-8')
    print(f'{done}/{total}')


if __name__ == '__main__':
    main()
