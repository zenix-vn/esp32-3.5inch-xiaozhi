#!/usr/bin/env python3
"""Put a built resource file into a card image. Usage: make_card.py RES.bin BASE.img OUT.img

The file always starts at cluster 5 (LBA 8248) and stays contiguous. If it grew, the FAT chain and the size in the
root directory entry are extended to match.
"""
import hashlib
import struct
import sys
from pathlib import Path

LBA0, FAT1, FAT2, DATA, CL = 8248, 6320 * 512, 7272 * 512, 8224 * 512, 4096
OFF = LBA0 * 512
OLD = 464781312


def main():
    res, base, out = (Path(a) for a in sys.argv[1:4])
    data = res.read_bytes()
    img = bytearray(base.read_bytes())
    n = len(data)
    if OFF + n > len(img):
        sys.exit('resource does not fit inside the card image')
    img[OFF:OFF + n] = data
    ncl = -(-n // CL)
    for c in range(5, 5 + ncl):
        v = c + 1 if c < 4 + ncl else 0x0FFFFFFF
        for f in (FAT1, FAT2):
            struct.pack_into('<I', img, f + c * 4, v)
    # root directory is cluster 2; find the short entry of the file that starts at cluster 5
    root = DATA
    for i in range(0, CL, 32):
        e = img[root + i:root + i + 32]
        if e[0] == 0:
            break
        if e[0] != 0xE5 and e[11] != 0x0F and not e[11] & 8:
            first = (struct.unpack('<H', e[20:22])[0] << 16) | struct.unpack('<H', e[26:28])[0]
            if first == 5:
                struct.pack_into('<I', img, root + i + 28, n)
                print(f'directory entry {bytes(e[:11])!r}: size {OLD} -> {n}, {ncl} clusters')
                break
    else:
        sys.exit('directory entry not found')
    out.write_bytes(img)
    print(f'{out} {len(img)} bytes sha256 {hashlib.sha256(img).hexdigest()}')
    old = base.read_bytes()
    chunks = [i >> 20 for i in range(0, len(img), 1 << 20) if img[i:i + (1 << 20)] != old[i:i + (1 << 20)]]
    print(f'{len(chunks)} MiB chunks differ from base: {chunks[:30]}{" ..." if len(chunks) > 30 else ""}')


if __name__ == '__main__':
    main()
