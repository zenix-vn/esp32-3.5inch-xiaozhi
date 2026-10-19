"""Hand-designed pixel art for "Luyện rồng": chibi dragons (8 stages x 4 expressions), eggs and 16x16 icons.

    python tools/gen_pixelart.py [preview.png]

Everything is drawn at its native pixel size (dragons 40x40, eggs 24x30, icons 16x16) from simple shapes,
then each part gets a 1-px dark outline, a two-tone cel shade (bottom-right) and a highlight (top-left).
The firmware shows them with integer nearest-neighbour zoom so the pixels stay crisp.

Output: firmware/src/pixel_img.c + pixel_img.h
"""
import math
import os
import sys

from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_dragons  # noqa: E402  (to_lvgl)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_C = os.path.join(ROOT, 'firmware', 'src', 'pixel_img.c')
OUT_H = os.path.join(ROOT, 'firmware', 'src', 'pixel_img.h')
INK = 0x1E2140


def rgb(c):
    return ((c >> 16) & 255, (c >> 8) & 255, c & 255)


def mix(c1, c2, t):
    a, b = rgb(c1), rgb(c2)
    return (int(a[0] + (b[0] - a[0]) * t) << 16) | (int(a[1] + (b[1] - a[1]) * t) << 8) | int(a[2] + (b[2] - a[2]) * t)


# ---------------------------------------------------------------- shapes (sets of pixels)
def ellipse(cx, cy, rx, ry):
    s = set()
    for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
        for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
            if ((x + 0.5 - cx) / rx) ** 2 + ((y + 0.5 - cy) / ry) ** 2 <= 1.0:
                s.add((x, y))
    return s


def polygon(pts, w=64, h=64):
    im = Image.new('L', (w, h), 0)
    ImageDraw.Draw(im).polygon(pts, fill=255)
    return {(x, y) for y in range(h) for x in range(w) if im.getpixel((x, y))}


def line(pts, w=64, h=64, width=1):
    im = Image.new('L', (w, h), 0)
    ImageDraw.Draw(im).line(pts, fill=255, width=width)
    return {(x, y) for y in range(h) for x in range(w) if im.getpixel((x, y))}


def mirror(s, W):
    return {(W - 1 - x, y) for x, y in s}


class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.px = {}

    def part(self, mask, base, shade=None, light=None, outline=INK, shade_w=2):
        """Fill a shape with a cel-shaded look and its own outline (drawn inside the shape's border)."""
        mask = {(x, y) for x, y in mask if 0 <= x < self.w and 0 <= y < self.h}
        for x, y in mask:
            c = base
            if shade is not None and any((x + k, y + k) not in mask for k in range(1, shade_w + 1)):
                c = shade
            elif light is not None and (x - 1, y - 1) not in mask and (x - 1, y) in mask or \
                    light is not None and (x - 1, y - 1) not in mask and (x, y - 1) in mask:
                c = light
            self.px[(x, y)] = c
        if outline is not None:
            for x, y in mask:
                if any((x + dx, y + dy) not in mask for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                    self.px[(x, y)] = outline

    def put(self, pts, c):
        for p in pts:
            if 0 <= p[0] < self.w and 0 <= p[1] < self.h:
                self.px[p] = c

    def image(self):
        im = Image.new('RGBA', (self.w, self.h), (0, 0, 0, 0))
        for (x, y), c in self.px.items():
            im.putpixel((x, y), rgb(c) + (255,))
        return im


# ---------------------------------------------------------------- dragons
STAGES = [
    # name, body, shade, light, belly, wing, horn, accent, wing scale, extras
    ('baby', 0x8FE3B0, 0x58B984, 0xC6F6D8, 0xFFF1B8, 0xFFB3C7, 0xFFD86B, 0xFF8FB0, 0.55, set()),
    ('lv1', 0x8ED8F8, 0x56ADE0, 0xC9EFFF, 0xFFF1B8, 0xFFB3C7, 0xFFD86B, 0xFF8FB0, 0.70, set()),
    ('lv2', 0x6FB2F7, 0x3F7FD3, 0xAFD7FF, 0xFFEFA8, 0xFFC46B, 0xFFD86B, 0xFF9F43, 0.82, {'spikes'}),
    ('lv3', 0xA48DF6, 0x735ED0, 0xD5C8FF, 0xFFEFC2, 0xFF9FC8, 0xFFD86B, 0xFF7FB0, 0.9, {'spikes', 'horns+'}),
    ('adult', 0x5B86EC, 0x3456B8, 0x9BB8FF, 0xFFE9A8, 0xFFB347, 0xFFE27A, 0xFF9F43, 0.97, {'spikes', 'horns+', 'big'}),
    ('king', 0xF0657E, 0xBE3C57, 0xFFA8B7, 0xFFE3C8, 0xFFC46B, 0xFFD23C, 0xFFD23C, 1.0, {'spikes', 'horns+', 'big', 'horn2'}),
    ('emperor', 0xFF9F55, 0xD7702C, 0xFFCF97, 0xFFF0C8, 0xFFE07A, 0xFFF0A0, 0xFF5C7A, 1.03, {'spikes', 'horns+', 'big', 'horn2', 'crown'}),
    ('divine', 0xFFE07A, 0xDDA63C, 0xFFF6CC, 0xFFFFFF, 0x9FE8FF, 0x6FE0FF, 0x6FE0FF, 1.05, {'spikes', 'horns+', 'big', 'horn2', 'halo', 'sparkle'}),
]
EXPRS = ['normal', 'happy', 'sad', 'sleep']
DW = DH = 40


def scale_pts(pts, ax, ay, s):
    return [(ax + (x - ax) * s, ay + (y - ay) * s) for x, y in pts]


def dragon(stage, expr):
    name, body, shade, light, belly, wing, horn, accent, ws, ex = stage
    c = Canvas(DW, DH)
    big = 'big' in ex
    wing_shade = mix(wing, 0x402040, 0.28)
    horn_shade = mix(horn, 0x804000, 0.3)
    # wings (behind everything): scalloped membranes, mirrored
    # bat wing: three fingers fanning out from the shoulder, scalloped membrane between them
    wl = scale_pts([(12, 23), (8, 15), (4, 11), (5, 16), (1, 18), (4, 21), (1, 25), (5, 26), (6, 29), (12, 29)], 12, 26, ws)
    wmask = polygon(wl)
    for m in (wmask, mirror(wmask, DW)):
        c.part(m, wing, wing_shade, mix(wing, 0xFFFFFF, 0.35))
    for m_pts in ([(12, 25), (5, 12)], [(12, 26), (2, 19)], [(12, 27), (2, 25)]):  # finger bones
        bone = line(scale_pts(m_pts, 12, 26, ws))
        c.put(bone, wing_shade)
        c.put(mirror(bone, DW), wing_shade)
    # tail (right side) with a spade tip
    c.part(polygon([(26, 31), (32, 33), (36, 31), (37, 26), (34, 28), (33, 30), (28, 29)]), body, shade, light)
    c.part(polygon([(35, 26), (37, 22), (39, 26), (37, 27)]), accent, mix(accent, 0x000000, 0.25))
    # body, feet, arms, belly
    bx = 10 if big else 9
    c.part(ellipse(20, 29, bx, 7.5), body, shade, light)
    for fx in (15, 25):
        c.part(ellipse(fx, 35.5, 3.6, 2.4), body, shade, light)
        c.put([(fx - 2, 37), (fx, 37), (fx + 2, 37)], 0xFFFFFF)  # claws
        c.put([(fx - 2, 37), (fx, 37), (fx + 2, 37)], 0xFFFFFF)
    for ax_ in (11.5, 28.5):
        c.part(ellipse(ax_, 28.5, 2.4, 3.2), body, shade, light)
    bel = ellipse(20, 30, 5.6 if big else 5, 5.6)
    c.part(bel, belly, mix(belly, 0xE0A040, 0.25), None, outline=shade)
    for yy in (28, 30, 32):  # belly plates
        c.put([(x, yy) for x in range(17, 24) if (x, yy) in bel and (x, yy - 1) in bel], mix(belly, 0xE0A040, 0.35))
    # back spikes peeking over the head
    if 'spikes' in ex:
        for sx in (16, 20, 24):
            c.part(polygon([(sx - 2, 7), (sx, 2 if sx == 20 else 4), (sx + 2, 7)]), accent, mix(accent, 0x000000, 0.25))
    # horns
    hl = 'horns+' in ex
    horn_l = polygon([(11, 9), (16, 8), (14, 2), (11, -1)]) if hl else polygon([(12, 9), (16, 8), (13, 2), (12, 2)])
    for m in (horn_l, mirror(horn_l, DW)):
        c.part(m, horn, horn_shade, mix(horn, 0xFFFFFF, 0.5), shade_w=1)
    if 'horn2' in ex:
        small = polygon([(9, 10), (11, 8), (8, 5)])
        for m in (small, mirror(small, DW)):
            c.part(m, horn, horn_shade, None, shade_w=1)
    # ear fins
    fin = polygon([(9, 11), (2, 9), (4, 13), (2, 16), (9, 16)])
    for m in (fin, mirror(fin, DW)):
        c.part(m, wing, wing_shade, None)
    # head
    head = ellipse(20, 15.5, 13, 9.2)
    c.part(head, body, shade, light)
    c.put([(x, y) for x, y in head if 10 <= y <= 11 and 12 <= x <= 15], mix(body, 0xFFFFFF, 0.45))  # shine
    if 'crown' in ex:
        crown = polygon([(14, 7), (14, 2), (16, 4), (18, 1), (20, 4), (22, 1), (24, 4), (26, 2), (26, 7)])
        c.part(crown, 0xFFD23C, 0xD9A019, 0xFFF0A0)
        c.put([(19, 5), (20, 5)], 0xFF4D6D)
    if 'halo' in ex:
        ring = ellipse(20, 2.5, 7, 2.2) - ellipse(20, 2.5, 5, 1.2)
        c.put(ring, 0xFFE27A)
    # face
    c.put([(9, 18), (10, 18), (11, 18), (10, 19)], 0xFF9DB0)       # blush
    c.put([(28, 18), (29, 18), (30, 18), (29, 19)], 0xFF9DB0)
    if expr == 'sleep':
        for exx in (13, 24):
            c.put([(exx, 15), (exx + 1, 16), (exx + 2, 16), (exx + 3, 15)], INK)
        c.put([(19, 19), (20, 19)], INK)
    elif expr == 'happy':
        for exx in (13, 24):
            c.put([(exx, 16), (exx + 1, 15), (exx + 2, 14), (exx + 3, 15), (exx + 4, 16)][:4] if False else
                  [(exx, 16), (exx + 1, 15), (exx + 2, 15), (exx + 3, 16)], INK)
        c.put([(18, 18), (19, 18), (20, 18), (21, 18), (18, 19), (21, 19), (19, 20), (20, 20)], INK)
        c.put([(19, 19), (20, 19)], 0xFF6B8A)
    else:
        for exx in (13, 24):
            eye = [(exx + i, 13 + j) for i in range(3) for j in range(4)]
            c.put(eye, INK)
            c.put([(exx, 13 if expr != 'sad' else 14)], 0xFFFFFF)
            c.put([(exx + 2, 16)], 0x8FA0FF)
        if expr == 'sad':
            c.put([(13, 12), (14, 12)], INK)   # worried brows
            c.put([(26, 12), (25, 12)], INK)
            c.put([(12, 17), (12, 18)], 0x6BC8FF)  # tear
            c.put([(18, 20), (19, 19), (20, 19), (21, 20)], INK)
        else:
            c.put([(18, 18), (19, 19), (20, 19), (21, 18)], INK)
    if 'whiskers' in ex:
        c.put(line([(14, 19), (9, 20), (6, 23)]), accent)
        c.put(mirror(line([(14, 19), (9, 20), (6, 23)]), DW), accent)
    if 'sparkle' in ex:
        for sx, sy in ((3, 4), (35, 8), (2, 31), (37, 36)):
            c.put([(sx, sy), (sx - 1, sy), (sx + 1, sy), (sx, sy - 1), (sx, sy + 1)], 0xFFFFFF)
    return c.image()


# ---------------------------------------------------------------- eggs
def egg(variant):
    c = Canvas(24, 30)
    cold = variant == 'cold'
    shell = ellipse(12, 16, 10.5, 13.2)
    base = 0xDCEFFF if cold else 0xFFFBEA
    c.part(shell, base, 0xB8C4E8 if cold else 0xC9C3E4, 0xFFFFFF, shade_w=4)
    spots = [(7.5, 9, 2.6, 0xF0507A), (16, 6, 2.2, 0x2EA8F0), (12, 17, 3.2, 0x2EA8F0), (5, 20, 2.2, 0x2EA8F0), (17, 23, 2.6, 0xD02E6A)]
    for sx, sy, r, col in spots:
        if cold:
            col = mix(col, 0xDCEFFF, 0.55)
        m = ellipse(sx, sy, r, r) & shell
        c.part(m, col, mix(col, 0x000000, 0.2), mix(col, 0xFFFFFF, 0.4), outline=None, shade_w=1)
    c.part(shell, base, None, None)  # re-outline over the spots
    for p, col in list(c.px.items()):
        pass
    # re-apply outline only (part() above repainted everything; restore spots underneath)
    c2 = Canvas(24, 30)
    c2.part(shell, base, 0xB8C4E8 if cold else 0xC9C3E4, 0xFFFFFF, shade_w=4)
    for sx, sy, r, col in spots:
        if cold:
            col = mix(col, 0xDCEFFF, 0.55)
        m = {p for p in ellipse(sx, sy, r, r) & shell if c2.px.get(p) != INK}
        c2.part(m, col, mix(col, 0x000000, 0.2), mix(col, 0xFFFFFF, 0.4), outline=None, shade_w=1)
    c2.put([(7, 5), (6, 6), (6, 7)], 0xFFFFFF)
    if variant in ('crack1', 'crack2'):
        c2.put(line([(12, 3), (10, 6), (13, 8), (11, 11)], 24, 30), INK)
    if variant == 'crack2':
        c2.put(line([(13, 8), (17, 9), (15, 12)], 24, 30), INK)
        c2.put(line([(10, 6), (6, 8)], 24, 30), INK)
    return c2.image()


# ---------------------------------------------------------------- 16x16 icons
def icon(kind):
    c = Canvas(16, 16)
    if kind == 'heart':
        m = ellipse(5, 6, 3.6, 3.6) | ellipse(11, 6, 3.6, 3.6) | polygon([(1, 7), (15, 7), (8, 15)], 16, 16)
        c.part(m, 0xF0406A, 0xB82050, 0xFF8FA8)
        c.put([(4, 4), (5, 4), (4, 5)], 0xFFFFFF)
    elif kind == 'drumstick':
        c.part(polygon([(9, 9), (12, 12), (11, 14), (13, 15), (15, 13), (13, 11), (10, 8)], 16, 16), 0xFFF4E0, 0xD8C8B0, None)
        c.part(ellipse(6.5, 6.5, 5.4, 5), 0xD9803A, 0x9C5420, 0xF5A866)
    elif kind == 'drop':
        c.part(polygon([(8, 1), (13, 9), (8, 15), (3, 9)], 16, 16) | ellipse(8, 10, 5, 4.6), 0x4FB8FF, 0x2A7FD4, 0xA8DCFF)
        c.put([(6, 8), (6, 9)], 0xFFFFFF)
    elif kind.startswith('face'):
        c.part(ellipse(8, 8, 7.4, 7.4), 0xFFD23C, 0xE09A10, 0xFFF0A0)
        if kind == 'face_happy':
            c.put([(4, 7), (5, 6), (6, 7), (9, 7), (10, 6), (11, 7)], INK)
            c.put([(5, 10), (6, 11), (7, 11), (8, 11), (9, 11), (10, 10)], INK)
            c.put([(7, 12), (8, 12)], 0xFF6B8A)
            c.put([(3, 9), (12, 9)], 0xFF9DB0)
        elif kind == 'face_ok':
            c.put([(5, 6), (5, 7), (10, 6), (10, 7)], INK)
            c.put([(6, 11), (7, 11), (8, 11), (9, 11)], INK)
        else:
            c.put([(5, 6), (5, 7), (10, 6), (10, 7), (4, 5), (11, 5)], INK)
            c.put([(6, 12), (7, 11), (8, 11), (9, 12)], INK)
            c.put([(3, 9), (3, 10)], 0x4FB8FF)
    elif kind == 'bolt':
        c.part(polygon([(9, 0), (2, 9), (7, 9), (5, 15), (13, 6), (8, 6), (11, 0)], 16, 16), 0xFFD23C, 0xE0961A, 0xFFF0A0)
    elif kind == 'coin':
        c.part(ellipse(8, 8, 7.2, 7.4), 0xFFC93C, 0xD98A12, 0xFFF0A0)
        c.part(ellipse(8, 8, 4.4, 4.6), 0xFFB020, 0xFFD86B, None, outline=0xD98A12, shade_w=1)
        c.put([(8, 5), (8, 6), (8, 7), (8, 8), (8, 9), (8, 10)], 0xD98A12)
    elif kind == 'bag':
        c.part(polygon([(4, 5), (12, 5), (15, 11), (13, 15), (3, 15), (1, 11)], 16, 16), 0xC8733A, 0x8F4C20, 0xE8A060)
        c.part(polygon([(5, 1), (11, 1), (10, 5), (6, 5)], 16, 16), 0xC8733A, 0x8F4C20, None)
        c.put([(5, 5), (6, 5), (7, 5), (8, 5), (9, 5), (10, 5)], 0xE53950)
        c.part(ellipse(8, 10.5, 2.4, 2.6), 0x4FCB5A, 0x2E9A3A, None, outline=None, shade_w=1)
    elif kind == 'star':
        pts = []
        for i in range(10):
            a = -math.pi / 2 + i * math.pi / 5
            r = 7.6 if i % 2 == 0 else 3.4
            pts.append((8 + r * math.cos(a), 8.6 + r * math.sin(a)))
        c.part(polygon(pts, 16, 16), 0xFFD23C, 0xE0961A, 0xFFF0A0)
    elif kind == 'moon':
        m = ellipse(7, 8, 6.4, 6.4) - ellipse(10.5, 5.5, 5.2, 5.2)
        c.part(m, 0xFFE27A, 0xE0A93A, 0xFFF6CC)
        c.put([(11, 10), (12, 10), (13, 10), (12, 11), (11, 12), (12, 12), (13, 12)], 0x6E7BD8)
    elif kind == 'potion':
        c.part(ellipse(8, 10.5, 5.6, 5), 0xCFE8F0, 0x9FB8C8, 0xFFFFFF)
        liq = {p for p in ellipse(8, 10.5, 4.4, 3.8) if p[1] >= 9}
        c.part(liq, 0xA45CF0, 0x7436C2, None, outline=None, shade_w=1)
        c.part(polygon([(6, 2), (10, 2), (10, 6), (6, 6)], 16, 16), 0xCFE8F0, 0x9FB8C8, None)
        c.part(polygon([(6, 0), (10, 0), (10, 2), (6, 2)], 16, 16), 0xB0773C, 0x8A5520, None, shade_w=1)
        c.put([(6, 9), (6, 10)], 0xFFFFFF)
    elif kind == 'ball':
        c.part(ellipse(8, 8, 7.2, 7.2), 0xFF5C5C, 0xC83A3A, 0xFF9F9F)
        c.put([(x, y) for x, y in ellipse(8, 8, 6.2, 6.2) if 7 <= y <= 8], 0xFFFFFF)
        c.part(ellipse(8, 8, 2.2, 2.2), 0x4C9AFF, None, None)
    elif kind == 'bubbles':
        for x, y, r in ((6, 9, 4.6), (11.5, 5.5, 3.4), (12, 12, 2.6)):
            c.part(ellipse(x, y, r, r), 0xBFEBFF, 0x7FC6F0, 0xFFFFFF, outline=0x4FA8E0, shade_w=1)
            c.put([(int(x - r / 2), int(y - r / 2))], 0xFFFFFF)
    elif kind == 'apple':
        c.part(ellipse(8, 9.5, 6.4, 5.8), 0xF0404A, 0xB02030, 0xFF8F8F)
        c.part(polygon([(8, 4), (9, 0), (10, 0), (9, 4)], 16, 16), 0x8B5A2B, None, None, shade_w=1)
        c.part(ellipse(11.5, 2.5, 2.4, 1.4), 0x4FCB5A, 0x2E9A3A, None, shade_w=1)
    elif kind == 'steak':
        c.part(ellipse(8, 9, 7.2, 5.6), 0xC8483A, 0x8F2C20, 0xF07A66)
        c.part(ellipse(10.5, 9, 2.4, 2), 0xFFE8D8, None, None, outline=None)
    elif kind == 'cake':
        c.part(polygon([(2, 8), (14, 8), (14, 15), (2, 15)], 16, 16), 0xF7C8D8, 0xD898B0, 0xFFE4EE)
        c.part(polygon([(2, 6), (14, 6), (14, 9), (2, 9)], 16, 16), 0xFFFFFF, 0xE0E0F0, None, shade_w=1)
        c.part(polygon([(8, 0), (11, 3), (8, 6), (5, 3)], 16, 16), 0x38E1FF, 0x1FA8D0, 0xB8F4FF, shade_w=1)
    elif kind == 'gem':
        c.part(polygon([(4, 2), (12, 2), (15, 6), (8, 15), (1, 6)], 16, 16), 0xFF4D7A, 0xC02050, 0xFF9FB8)
        c.put([(5, 4), (6, 4), (4, 5)], 0xFFFFFF)
    return c.image()


ICONS = [('st_heart', 'heart'), ('ui_food', 'drumstick'), ('st_food', 'drumstick'), ('st_clean', 'drop'), ('st_energy', 'bolt'),
         ('fc_happy', 'face_happy'), ('fc_ok', 'face_ok'), ('fc_sad', 'face_sad'),
         ('ui_coin', 'coin'), ('ui_shop', 'bag'), ('ui_badge', 'star'), ('ui_sleep', 'moon'), ('ui_med', 'potion'),
         ('ui_play', 'ball'), ('ui_bath', 'bubbles'), ('fx_heart', 'heart'), ('fx_food0', 'apple'), ('fx_food1', 'steak'),
         ('fx_food2', 'cake'), ('fx_coin', 'coin'), ('fx_gem', 'gem')]


def main():
    images = []
    for st in STAGES:
        for ex in EXPRS:
            images.append((f'dg_{st[0]}_{ex}', dragon(st, ex)))
    for v in ('warm', 'crack1', 'crack2', 'cold'):
        images.append((f'egg_{v}', egg(v)))
    for name, kind in ICONS:
        images.append((name, icon(kind)))
    if len(sys.argv) > 1:  # preview: dragons x4, eggs x4, icons x3 on a garden-blue background
        Z = 4
        sheet = Image.new('RGBA', (8 * 44 * Z // 2 + 20, 4 * 44 * 2 + 300), (134, 211, 245, 255))
        for i, st in enumerate(STAGES):
            for j, ex in enumerate(EXPRS):
                im = dragon(st, ex).resize((DW * 2, DH * 2), Image.NEAREST)
                sheet.alpha_composite(im, (10 + i * 88, 10 + j * 88))
        x = 10
        for v in ('warm', 'crack1', 'crack2', 'cold'):
            sheet.alpha_composite(egg(v).resize((96, 120), Image.NEAREST), (x, 370))
            x += 104
        x = 10
        for name, kind in ICONS:
            sheet.alpha_composite(icon(kind).resize((48, 48), Image.NEAREST), (x, 500))
            x += 52
        sheet.save(sys.argv[1])
    total = 0
    with open(OUT_C, 'w', encoding='utf-8', newline='\n') as f:
        f.write('// Generated by tools/gen_pixelart.py - do not edit.\n#include "pixel_img.h"\n\n')
        for name, im in images:
            w, h, data = gen_dragons.to_lvgl(im)
            total += len(data)
            f.write(f'static const uint8_t {name}_map[] = {{\n')
            for i in range(0, len(data), 24):
                f.write('  ' + ','.join(f'0x{x:02x}' for x in data[i:i + 24]) + ',\n')
            f.write('};\n')
            f.write(f'const lv_img_dsc_t {name} = {{\n  .header = {{.cf = LV_IMG_CF_TRUE_COLOR_ALPHA, .always_zero = 0, .reserved = 0, .w = {w}, .h = {h}}},\n'
                    f'  .data_size = {len(data)},\n  .data = {name}_map,\n}};\n\n')
    with open(OUT_H, 'w', encoding='utf-8', newline='\n') as f:
        f.write('#pragma once\n// Generated by tools/gen_pixelart.py - do not edit.\n#include <lvgl.h>\n\n#ifdef __cplusplus\nextern "C" {\n#endif\n')
        for name, _ in images:
            f.write(f'extern const lv_img_dsc_t {name};\n')
        f.write('#ifdef __cplusplus\n}\n#endif\n')
    print(f'{len(images)} images, {total // 1024} KB')


if __name__ == '__main__':
    main()
