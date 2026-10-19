"""Draws the virtual-dragon sprites (one per growth stage, plus egg variants) and writes them as LVGL images.

    python tools/gen_dragons.py [preview.png]

Output: firmware/src/dragon_img.c + dragon_img.h (LV_IMG_CF_TRUE_COLOR_ALPHA, RGB565 little-endian + alpha).
Eyes are not part of the sprites: the firmware draws them on top (open / closed / happy) at the exported
eye position, so one sprite per stage covers every mood.
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

SS = 4  # supersampling
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_C = os.path.join(ROOT, 'firmware', 'src', 'dragon_img.c')
OUT_H = os.path.join(ROOT, 'firmware', 'src', 'dragon_img.h')

W0, H0 = 230, 172  # logical canvas (dragon faces right, feet on y ~ 164)
OY = 22  # headroom above the drawing for wings, horns and halo
FIT = 0.80  # the game picture box is 140 px tall


def hexc(h, a=255):
    return ((h >> 16) & 255, (h >> 8) & 255, h & 255, a)


def mix(c1, c2, t):
    return tuple(int(c1[i] * (1 - t) + c2[i] * t) for i in range(3)) + (255,)


def bez(p0, p1, p2, p3, n=24):
    pts = []
    for i in range(n + 1):
        t = i / n
        u = 1 - t
        x = u ** 3 * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t ** 3 * p3[0]
        y = u ** 3 * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t ** 3 * p3[1]
        pts.append((x, y))
    return pts


def taper(center, w0, w1):
    """Closed polygon around a centre line whose width goes from w0 to w1."""
    left, right = [], []
    n = len(center)
    for i, (x, y) in enumerate(center):
        a = center[max(i - 1, 0)]
        b = center[min(i + 1, n - 1)]
        dx, dy = b[0] - a[0], b[1] - a[1]
        ln = math.hypot(dx, dy) or 1
        nx, ny = -dy / ln, dx / ln
        w = (w0 + (w1 - w0) * i / (n - 1)) / 2
        left.append((x + nx * w, y + ny * w))
        right.append((x - nx * w, y - ny * w))
    return left + right[::-1]


class Pen:
    def __init__(self, k):
        self.k = k
        self.img = Image.new('RGBA', (int(W0 * k), int(H0 * k)), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.img)

    def P(self, pts):
        return [(x * self.k, (y + OY) * self.k) for x, y in pts]

    def poly(self, pts, col):
        self.d.polygon(self.P(pts), fill=col)

    def ell(self, cx, cy, rx, ry, col):
        k = self.k
        box = [(cx - rx) * k, (cy + OY - ry) * k, (cx + rx) * k, (cy + OY + ry) * k]
        if len(col) == 4 and col[3] < 255:  # translucent: blend instead of overwriting the alpha
            layer = Image.new('RGBA', self.img.size, (0, 0, 0, 0))
            ImageDraw.Draw(layer).ellipse(box, fill=col)
            self.img.alpha_composite(layer)
        else:
            self.d.ellipse(box, fill=col)

    def line(self, pts, col, w):
        self.d.line(self.P(pts), fill=col, width=max(1, int(w * self.k)), joint='curve')
        for x, y in (pts[0], pts[-1]):
            self.ell(x, y, w / 2, w / 2, col)

    def arc(self, cx, cy, r, a0, a1, col, w):
        k = self.k
        self.d.arc([(cx - r) * k, (cy + OY - r) * k, (cx + r) * k, (cy + OY + r) * k], a0, a1, fill=col, width=max(1, int(w * k)))


# ---------------------------------------------------------------------------------------------
# stage look
# ---------------------------------------------------------------------------------------------
STAGES = [
    # name, scale, body, belly, accent(horns/spikes), wing, params
    ('baby', 0.62, 0x6FD98A, 0xF2FFD8, 0xFFC93C, 0x4FB874, dict(head=1.45, neck=0.35, wing=0.35, horn=0.35, spikes=0, legs=0.9)),
    ('lv1', 0.68, 0x45C9B8, 0xE4FFF6, 0xFFC93C, 0x2E9E95, dict(head=1.3, neck=0.5, wing=0.55, horn=0.55, spikes=3, legs=0.95)),
    ('lv2', 0.74, 0x4C9AFF, 0xE6F0FF, 0xFFB347, 0x2F6FD6, dict(head=1.18, neck=0.65, wing=0.75, horn=0.7, spikes=5, legs=1.0)),
    ('lv3', 0.80, 0x6F63F0, 0xECE8FF, 0xFFB347, 0x4A3FC4, dict(head=1.1, neck=0.8, wing=0.9, horn=0.85, spikes=7, legs=1.0)),
    ('adult', 0.86, 0xA45CF0, 0xF3E6FF, 0xFFD54A, 0x7436C2, dict(head=1.0, neck=1.0, wing=1.05, horn=1.0, spikes=9, legs=1.0)),
    ('king', 0.92, 0xD93E6C, 0xFFE3EA, 0xFFD54A, 0x9E1F49, dict(head=1.0, neck=1.05, wing=1.15, horn=1.15, spikes=10, legs=1.0, horn2=1)),
    ('emperor', 0.97, 0xF07A2E, 0xFFEAD2, 0xFFE27A, 0xB8461A, dict(head=1.0, neck=1.1, wing=1.22, horn=1.2, spikes=11, legs=1.0, horn2=1, crown=1, aura=0xFFB347)),
    ('divine', 1.0, 0xF5C842, 0xFFF8DC, 0x6FE7FF, 0xE89B1A, dict(head=1.0, neck=1.15, wing=1.3, horn=1.25, spikes=12, legs=1.0, horn2=1, whiskers=1, halo=1, aura=0x6FE7FF)),
]


def draw_dragon(scale, body_c, belly_c, accent_c, wing_c, p):
    k = scale * SS
    pen = Pen(k)
    body = hexc(body_c)
    shade = mix(body, (20, 10, 40), 0.28)
    belly = hexc(belly_c)
    accent = hexc(accent_c)
    wing = hexc(wing_c)
    wing_far = mix(wing, (10, 5, 30), 0.35)
    membrane = mix(wing, (255, 255, 255), 0.18)

    hs = p['head']
    # skeleton
    bx, by = 98, 100            # body centre
    neck0 = (122, 86)           # neck base
    nl = p['neck']
    head = (neck0[0] + 26 + 22 * nl, neck0[1] - 22 - 30 * nl)  # skull centre
    hx, hy = head
    if hs > 1.2:                # babies: head sits low and close
        hy += 8 * (hs - 1.2) * 4

    # --- wings (behind) ---
    def wing_shape(sx, sy, s, col, mem):
        tips = [(sx - 62 * s, sy - 40 * s), (sx - 30 * s, sy - 70 * s), (sx + 6 * s, sy - 64 * s)]
        root = (sx + 14 * s, sy + 6 * s)
        pts = [(sx, sy)]
        pts += bez((sx, sy), (sx - 20 * s, sy - 40 * s), (tips[0][0] + 20 * s, tips[0][1] - 10 * s), tips[0], 10)
        prev = tips[0]
        for t in tips[1:]:
            mid = ((prev[0] + t[0]) / 2 + 4 * s, (prev[1] + t[1]) / 2 + 16 * s)
            pts += bez(prev, mid, mid, t, 8)
            prev = t
        pts += bez(prev, (prev[0] + 6 * s, prev[1] + 30 * s), (root[0], root[1] - 20 * s), root, 10)
        pen.poly(pts, mem)
        for t in tips:  # finger bones
            pen.line(bez((sx, sy), ((sx + t[0]) / 2, (sy + t[1]) / 2 - 6 * s), t, t, 10), col, 3.2 * min(1, s + 0.3))
        pen.ell(sx, sy, 5 * max(s, 0.6), 5 * max(s, 0.6), col)

    ws = p['wing']
    wing_shape(bx + 4, by - 26, ws * 0.92, wing_far, mix(wing_far, (255, 255, 255), 0.1))
    wing_shape(bx + 16, by - 22, ws, wing, membrane)

    # --- dorsal spikes (behind body line) ---
    back = bez((40, 104), (60, 70), (110, 62), neck0, 20) + bez(neck0, (neck0[0] + 6, neck0[1] - 20), (hx - 18, hy + 4), (hx - 10, hy - 6), 14)
    nsp = p['spikes']
    for i in range(nsp):
        t = 0.08 + 0.84 * i / max(nsp - 1, 1)
        j = int(t * (len(back) - 1))
        x, y = back[j]
        a = back[max(j - 1, 0)]
        b = back[min(j + 1, len(back) - 1)]
        dx, dy = b[0] - a[0], b[1] - a[1]
        ln = math.hypot(dx, dy) or 1
        nx, ny = dy / ln, -dx / ln
        if ny > 0:
            nx, ny = -nx, -ny
        h = 9 + 3 * math.sin(t * math.pi)
        pen.poly([(x - dx / ln * 5, y - dy / ln * 5), (x + nx * h + dx / ln * 2, y + ny * h + dy / ln * 2), (x + dx / ln * 5, y + dy / ln * 5)], accent)

    # --- tail ---
    tail = bez((70, 112), (30, 140), (4, 118), (14, 84), 30)
    pen.poly(taper(tail, 26, 5), body)
    tx, ty = tail[-1]
    pen.poly([(tx, ty + 4), (tx - 10, ty - 4), (tx + 1, ty - 16), (tx + 9, ty - 3)], accent)  # spade tip
    pen.poly(taper(bez((70, 120), (40, 140), (16, 124), (16, 100), 16), 10, 3), belly)          # tail belly stripe

    # --- far legs ---
    lg = p['legs']
    pen.poly(taper([(84, 112), (82, 126), (86, 138)], 18 * lg, 11 * lg), shade)
    pen.ell(92, 139, 10 * lg, 5, shade)
    pen.poly(taper([(126, 104), (130, 122), (134, 138)], 13 * lg, 9 * lg), shade)
    pen.ell(140, 139, 8 * lg, 4.5, shade)

    # --- body ---
    pen.ell(bx, by, 44, 31, body)
    pen.ell(bx + 2, by + 12, 34, 18, belly)
    # --- neck ---
    neck = bez(neck0, (neck0[0] + 8, neck0[1] - 14), (hx - 22, hy + 22), (hx - 4, hy + 6), 18)
    pen.poly(taper(neck, 34, 22 * min(hs, 1.2)), body)
    pen.poly(taper([(x + 7, y + 6) for x, y in neck[:-3]], 12, 8), belly)
    # belly scales
    for i in range(5):
        cx = bx - 18 + i * 10
        pen.arc(cx, by + 6, 9, 40, 140, mix(belly, (150, 120, 80), 0.35), 1.4)

    # --- near legs ---
    pen.ell(80, 112, 20 * lg, 17 * lg, body)                                 # thigh
    pen.poly(taper([(80, 116), (78, 128), (84, 138)], 16 * lg, 11 * lg), body)
    pen.ell(90, 139, 11 * lg, 5.5, body)                                     # foot
    for i in range(3):
        pen.ell(96 + i * 4 - 4, 141, 2.4, 2.4, (250, 250, 240, 255))         # claws
    pen.poly(taper([(122, 104), (125, 122), (128, 137)], 13 * lg, 9 * lg), body)  # front leg
    pen.ell(133, 139, 9 * lg, 5, body)
    for i in range(3):
        pen.ell(136 + i * 3.6 - 2, 141, 2.1, 2.1, (250, 250, 240, 255))

    # --- head ---
    s = hs * 0.95
    # horns (behind the skull)
    hl = p['horn']
    for off, ln, col in ((-4, 1.0, mix(accent, (90, 60, 0), 0.25)), (4, 0.85, accent)):
        base = (hx - 8 * s + off, hy - 12 * s)
        tip = (hx - 8 * s - 26 * hl * s + off, hy - 22 * s - 18 * hl * s)
        pen.poly(taper(bez(base, (base[0] - 2, base[1] - 14 * hl * s), (tip[0] + 12 * s, tip[1] + 4), tip, 12), 8 * s, 1.5), col)
    if p.get('horn2'):
        base = (hx + 2 * s, hy - 16 * s)
        tip = (hx - 6 * s, hy - 40 * s * hl)
        pen.poly(taper(bez(base, (base[0] + 2, base[1] - 10), (tip[0] + 6, tip[1] + 8), tip, 10), 6 * s, 1.2), accent)
    # ear frill
    pen.poly([(hx - 14 * s, hy - 2 * s), (hx - 30 * s, hy - 4 * s), (hx - 24 * s, hy + 6 * s), (hx - 32 * s, hy + 12 * s), (hx - 12 * s, hy + 8 * s)], wing)
    # skull + snout + jaw
    pen.ell(hx, hy, 17 * s, 15 * s, body)
    snout = [(hx - 2 * s, hy - 10 * s), (hx + 16 * s, hy - 9 * s), (hx + 28 * s, hy - 3 * s), (hx + 30 * s, hy + 4 * s),
             (hx + 22 * s, hy + 8 * s), (hx + 2 * s, hy + 10 * s)]
    pen.poly(snout, body)
    pen.ell(hx + 27 * s, hy + 1 * s, 4.5 * s, 5 * s, body)
    jaw = [(hx - 6 * s, hy + 6 * s), (hx + 24 * s, hy + 8 * s), (hx + 18 * s, hy + 14 * s), (hx - 2 * s, hy + 15 * s)]
    pen.poly(jaw, belly)
    pen.line([(hx + 4 * s, hy + 8.5 * s), (hx + 23 * s, hy + 8 * s)], mix(body, (60, 20, 40), 0.55), 1.4)  # mouth
    pen.ell(hx + 25 * s, hy - 2.5 * s, 1.8 * s, 1.4 * s, mix(body, (40, 20, 30), 0.7))                  # nostril
    pen.ell(hx + 4 * s, hy - 7 * s, 9 * s, 4 * s, mix(body, (255, 255, 255), 0.22))                     # brow highlight
    pen.ell(hx + 10 * s, hy + 5 * s, 4 * s, 2.6 * s, (255, 140, 160, 150))                             # cheek
    eye = (hx + 6 * s, hy - 3 * s, 6.2 * s)

    if p.get('whiskers'):
        pen.line(bez((hx + 26 * s, hy + 2 * s), (hx + 44, hy + 2), (hx + 46, hy + 22), (hx + 30, hy + 34), 16), accent, 1.8)
        pen.line(bez((hx + 22 * s, hy + 6 * s), (hx + 30, hy + 20), (hx + 14, hy + 30), (hx + 20, hy + 44), 16), accent, 1.6)
    if p.get('crown'):
        cx, cy = hx - 2 * s, hy - 15 * s
        gold = hexc(0xFFD23C)
        pen.poly([(cx - 11, cy + 3), (cx - 12, cy - 9), (cx - 6, cy - 3), (cx, cy - 13), (cx + 6, cy - 3), (cx + 12, cy - 9), (cx + 11, cy + 3)], gold)
        pen.ell(cx, cy - 1, 2.6, 2.6, hexc(0xFF4D6D))

    # cartoon outline under everything drawn so far
    a = pen.img.split()[3]
    grown = a.filter(ImageFilter.MaxFilter(2 * int(1.6 * SS) + 1))
    outline_col = mix(body, (25, 10, 45), 0.75)
    out = Image.new('RGBA', pen.img.size, (0, 0, 0, 0))
    out.paste(Image.new('RGBA', pen.img.size, outline_col), (0, 0), grown)
    out.alpha_composite(pen.img)

    final = Image.new('RGBA', pen.img.size, (0, 0, 0, 0))
    if p.get('aura'):
        glow = Image.new('RGBA', pen.img.size, (0, 0, 0, 0))
        gd = ImageDraw.Draw(glow)
        c = hexc(p['aura'])
        gd.ellipse([20 * k, (OY - 4) * k, 210 * k, (OY + 150) * k], fill=c[:3] + (90,))
        glow = glow.filter(ImageFilter.GaussianBlur(14 * k / SS * SS))
        final.alpha_composite(glow)
    final.alpha_composite(out)
    if p.get('halo'):
        hd = ImageDraw.Draw(final)
        cx, cy = hx - 4, hy - 34 + OY
        hd.ellipse([(cx - 16) * k, (cy - 5) * k, (cx + 16) * k, (cy + 5) * k], outline=(255, 240, 150, 255), width=int(2.4 * k))
        for sx, sy, r in ((30, 30, 2.2), (196, 70, 1.8), (24, 96, 1.6), (206, 22, 2.4), (150, 12, 1.5)):
            sy += OY
            hd.ellipse([(sx - r) * k, (sy - r) * k, (sx + r) * k, (sy + r) * k], fill=(255, 255, 255, 230))
    size = (int(W0 * scale), int(H0 * scale))
    return final.resize(size, Image.LANCZOS), (eye[0] * scale, (eye[1] + OY) * scale, eye[2] * scale)


def draw_egg(cold, crack):
    scale = 0.95
    k = scale * SS
    pen = Pen(k)
    base = hexc(0xCFE6FF if cold else 0xFFF1D6)
    pen.ell(115, 82, 34, 44, base)
    spots = [(103, 70, 8, 0x9CC7FF if cold else 0xFFC93C), (128, 88, 6.5, 0xB5D5FF if cold else 0x7BE495),
             (106, 102, 7, 0x9CC7FF if cold else 0x6BB5FF), (127, 60, 4.5, 0xB5D5FF if cold else 0xFF8FA3)]
    for x, y, r, c in spots:
        pen.ell(x, y, r, r, hexc(c))
    pen.ell(102, 58, 6, 10, (255, 255, 255, 140))
    dark = hexc(0x6B4F2A)
    if crack >= 1:
        pen.line([(112, 42), (108, 52), (116, 58), (110, 66)], dark, 1.6)
    if crack >= 2:
        pen.line([(116, 58), (126, 62), (122, 70)], dark, 1.6)
        pen.line([(108, 52), (98, 56)], dark, 1.4)
    a = pen.img.split()[3]
    grown = a.filter(ImageFilter.MaxFilter(2 * int(1.6 * SS) + 1))
    out = Image.new('RGBA', pen.img.size, (0, 0, 0, 0))
    out.paste(Image.new('RGBA', pen.img.size, (70, 50, 40, 255)), (0, 0), grown)
    out.alpha_composite(pen.img)
    out = out.crop((int(70 * k), int((30 + OY) * k), int(160 * k), int((134 + OY) * k)))
    return out.resize((int(90 * scale), int(104 * scale)), Image.LANCZOS)


def to_lvgl(img):
    w, h = img.size
    data = bytearray()
    for r, g, b, a in img.getdata():
        if a == 0:
            r = g = b = 0
        v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
        data += bytes((v & 255, v >> 8, a))
    return w, h, data


def main():
    sprites = []
    eyes = []
    for name, scale, b, bl, ac, wc, p in STAGES:
        img, eye = draw_dragon(scale * FIT, b, bl, ac, wc, p)
        sprites.append(('dragon_' + name, img))
        eyes.append(eye)
    for name, cold, crack in (('egg_warm', 0, 0), ('egg_crack1', 0, 1), ('egg_crack2', 0, 2), ('egg_cold', 1, 0)):
        sprites.append((name, draw_egg(cold, crack)))

    if len(sys.argv) > 1:  # preview sheet on a dark background
        cols = 4
        cw, ch = int(W0 + 10), int(H0 + 10)
        rows = (len(sprites) + cols - 1) // cols
        sheet = Image.new('RGBA', (cw * cols, ch * rows), (21, 27, 51, 255))
        for i, (n, im) in enumerate(sprites):
            x, y = (i % cols) * cw + 5, (i // cols) * ch + 5
            sheet.alpha_composite(im, (x + (W0 - im.size[0]) // 2, y + (H0 - im.size[1]))) if n.startswith('dragon') else sheet.alpha_composite(im, (x + 60, y + 30))
            if n.startswith('dragon'):
                ex, ey, er = eyes[i]
                d = ImageDraw.Draw(sheet)
                ox, oy = x + (W0 - im.size[0]) // 2, y + (H0 - im.size[1])
                d.ellipse([ox + ex - er, oy + ey - er, ox + ex + er, oy + ey + er], fill=(255, 255, 255, 255))
                d.ellipse([ox + ex - er * 0.25, oy + ey - er * 0.6, ox + ex + er * 0.75, oy + ey + er * 0.6], fill=(30, 20, 50, 255))
        sheet.save(sys.argv[1])

    total = 0
    with open(OUT_C, 'w', encoding='utf-8', newline='\n') as f:
        f.write('// Generated by tools/gen_dragons.py - do not edit.\n#include "dragon_img.h"\n\n')
        for name, im in sprites:
            w, h, data = to_lvgl(im)
            total += len(data)
            f.write(f'static const uint8_t {name}_map[] = {{\n')
            for i in range(0, len(data), 24):
                f.write('  ' + ','.join(f'0x{x:02x}' for x in data[i:i + 24]) + ',\n')
            f.write('};\n')
            f.write(f'const lv_img_dsc_t {name} = {{\n  .header = {{.cf = LV_IMG_CF_TRUE_COLOR_ALPHA, .always_zero = 0, .reserved = 0, .w = {w}, .h = {h}}},\n'
                    f'  .data_size = {len(data)},\n  .data = {name}_map,\n}};\n\n')
    with open(OUT_H, 'w', encoding='utf-8', newline='\n') as f:
        f.write('#pragma once\n// Generated by tools/gen_dragons.py - do not edit.\n#include <lvgl.h>\n\n#ifdef __cplusplus\nextern "C" {\n#endif\n')
        for name, _ in sprites:
            f.write(f'extern const lv_img_dsc_t {name};\n')
        f.write('#ifdef __cplusplus\n}\n#endif\n\n')
        f.write('// eye centre and radius per dragon stage (1..8), in sprite pixels\n')
        f.write('static const float DRAGON_EYE[8][3] = {\n')
        for ex, ey, er in eyes:
            f.write(f'    {{{ex:.1f}f, {ey:.1f}f, {er:.1f}f}},\n')
        f.write('};\n')
    print(f'{len(sprites)} sprites, {total // 1024} KB')


if __name__ == '__main__':
    main()
