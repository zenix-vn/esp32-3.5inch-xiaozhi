"""Vietnamese text as hand-drawn 14x16 cells, two letters per cell, stored under rare Hanzi code points.

Why: the device draws every non-ASCII character in a fixed 14 px advance, so a 7 px wide accented letter leaves a hole,
and some Latin Extended code points render as noise (ô, ú, ũ, ờ, ớ, ộ, ệ, ừ were seen). Ordinary Hanzi cells are read
correctly. So Vietnamese is drawn in monospace Consolas on a 7 px grid, two letters to a cell, and every distinct pair
gets one rare Hanzi that no Chinese string on the card uses.
"""
import re

import numpy as np
from PIL import Image, ImageDraw, ImageFont

FONT_PATH = 'C:/Windows/Fonts/consola.ttf'
SIZE = 12
BASE = 13          # baseline row inside the 16 row cell
SLOT = 7
_font = ImageFont.truetype(FONT_PATH, SIZE)
_letters = {}


def letter(ch):
    """16 x 7 bitmap of one character on the 7 px grid."""
    if ch not in _letters:
        im = Image.new('1', (14, 16), 0)
        d = ImageDraw.Draw(im)
        d.fontmode = '1'
        d.text((0, BASE), ch, font=_font, fill=1, anchor='ls')
        _letters[ch] = np.array(im, dtype=np.uint8)[:, :SLOT]
    return _letters[ch]


def pack(text):
    """Pairs of characters; an odd last character is padded with a space."""
    if len(text) % 2:
        text += ' '
    return [text[i:i + 2] for i in range(0, len(text), 2)]


def draw(pair):
    return np.concatenate([letter(pair[0]), letter(pair[1])], axis=1)


def cell_bytes(g):
    b = bytearray(28)
    for p in range(2):
        for x in range(14):
            v = 0
            for y in range(8):
                v |= int(g[p * 8 + y, x]) << y
            b[p * 14 + x] = v
    return bytes(b)


POS = re.compile(r'^(?:(?:aux|modal|det|pron|prep|conj|int|interj|num|art|adj|adv|vt|vi|v|n|abbr|pl)\.(?:&\s?)?\s?)+')


class Pool:
    """Hands out code points from rare Hanzi that no Chinese string on the card uses."""

    def __init__(self, exclude=()):
        ex = set(exclude)
        self.free = []
        for cp in range(0x9FA5, 0x4DFF, -1):
            ch = chr(cp)
            try:
                ch.encode('gb2312')
                continue
            except UnicodeEncodeError:
                pass
            if ch not in ex:
                self.free.append(cp)
        self.map = {}   # pair -> code point

    def code(self, pair):
        if pair not in self.map:
            if len(self.map) >= len(self.free):
                raise OverflowError(f'out of cells ({len(self.free)})')
            self.map[pair] = self.free[len(self.map)]
        return chr(self.map[pair])

    def line(self, text):
        return ''.join(self.code(p) for p in pack(text))

    def meaning(self, text):
        """Keep the part-of-speech tag in plain ASCII (the device formats it), draw the rest."""
        m = POS.match(text)
        if m:
            return m.group(0).rstrip() + ' ' + self.line(text[m.end():])
        return self.line(text)

    def table(self):
        return {cp: cell_bytes(draw(p)) for p, cp in self.map.items()}
