"""Encodes Vietnamese as Private Use cells: each word is drawn in Tahoma and sliced into 14 px cells.

The firmware gives every multi-byte character a full 14 px advance, so ordinary Vietnamese letters look spaced out.
Private Use cells are empty in the font table and collide with nothing, so one cell can hold two letters.
"""
import numpy as np

from glyphs import _raster, FONT, to_bytes

FIRST, LAST = 0xE000, 0xF8FF


class Encoder:
    def __init__(self):
        self.cells = {}  # cell bytes -> code point

    def _cp(self, g):
        b = to_bytes(g)
        if b not in self.cells:
            cp = FIRST + len(self.cells)
            if cp > LAST:
                raise OverflowError('out of Private Use cells')
            self.cells[b] = cp
        return self.cells[b]

    def word(self, w):
        s = _raster(w, FONT)
        n = -(-(s.shape[1] + 1) // 14)  # keep at least one blank column so the next word never touches this one
        c = np.zeros((16, n * 14), np.uint8)
        c[:, :s.shape[1]] = s
        return ''.join(chr(self._cp(c[:, i * 14:(i + 1) * 14])) for i in range(n))

    def text(self, t):
        out = []
        for w in t.split(' '):
            if w:
                out.append(self.word(w))
        return ' '.join(out)

    def table(self):
        return {cp: b for b, cp in self.cells.items()}
