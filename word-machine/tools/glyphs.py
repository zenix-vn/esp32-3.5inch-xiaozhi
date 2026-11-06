"""Draws Vietnamese into 14x16 cells with hinted 1-bit Tahoma."""
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from ui_map import CELLS, GROUPS, RIGHT, LEFT
FONT = ImageFont.truetype('C:/Windows/Fonts/tahoma.ttf', 12)
BASE = 13  # baseline row; tall stacked marks and descenders both fit in 16 rows
MARGIN = 1
SPECIAL = {'按':('Nút',9,0),'长':('Giữ',9,0),'调':('Đổi',9,0),'设':('Cài',10,0),'置':('đặt',9,1),'备':('đặt',9,1),'版':('bản',9,3),'新':('Mới',9,0),'当':('Nay',9,0),'看':('xem',8,0)}  # (text, Tahoma size, x)
TIGHT = set('的新词')  # cells that must leave room for a word gap
SMALL = ImageFont.truetype('C:/Windows/Fonts/tahoma.ttf', 11)
def raster(text, limit=None, sizes=(11,)):
    a = _raster(text, FONT)
    if limit:
        for size in sizes:          # shrink until a long syllable fits one cell
            if a.shape[1] <= limit:
                break
            a = _raster(text, ImageFont.truetype('C:/Windows/Fonts/tahoma.ttf', size))
    return a
def _raster(text, FONT):
    w = int(FONT.getlength(text)) + 4
    im = Image.new('1', (w, 16), 0)
    d = ImageDraw.Draw(im); d.fontmode = '1'
    d.text((1, BASE), text, font=FONT, fill=1, anchor='ls')
    a = np.array(im, dtype=np.uint8)
    cols = np.nonzero(a.sum(axis=0))[0]
    return a[:, cols.min():cols.max() + 1]
def cells():
    out = {}
    for chars, text in GROUPS.items():
        W = 14 * len(chars); s = raster(text, W, (11, 10, 9))
        assert s.shape[1] <= W, (chars, text, s.shape[1], W)
        c = np.zeros((16, W), np.uint8); x = (W - s.shape[1]) // 2
        c[:, x:x + s.shape[1]] = s
        for i, ch in enumerate(chars):
            out[ch] = c[:, i * 14:(i + 1) * 14]
    for ch, (t, size, x) in SPECIAL.items():
        s = _raster(t, ImageFont.truetype('C:/Windows/Fonts/tahoma.ttf', size)); w = min(s.shape[1], 14 - x)
        c = np.zeros((16, 14), np.uint8); c[:, x:x + w] = s[:, :w]
        out[ch] = c
    for ch, t in CELLS.items():
        if ch in out: continue
        c = np.zeros((16, 14), np.uint8)
        if t:
            s = _raster(t, SMALL) if ch in TIGHT else raster(t, 14); w = s.shape[1]
            assert w <= 14, (ch, t, w)
            x = 14 - w - MARGIN if ch in RIGHT else (2 if ch in LEFT else (14 - w) // 2)
            x = min(max(x, 0), 14 - w)
            c[:, x:x + w] = s
        out[ch] = c
    return out
def to_bytes(g):
    b = bytearray(28)
    for p in range(2):
        for x in range(14):
            v = 0
            for y in range(8):
                v |= int(g[p * 8 + y, x]) << y
            b[p * 14 + x] = v
    return bytes(b)
