"""Smooth (non-pixel) version of the chibi dragons for the home screen.

    python tools/gen_hd.py [preview.png]

Same design, proportions and colours as gen_pixelart.py, but drawn with vector shapes at 16x the native
40x40 grid, then shaded, outlined and scaled down to 160x160 with anti-aliasing. The face is left out: the
firmware draws the eyes, mouth and blush with LVGL so the expression can change and the dragon can blink.
Stored as LV_IMG_CF_INDEXED_8BIT (palette with alpha) to keep each stage around 26 KB.

Output: firmware/src/hd_img.c + hd_img.h
"""
import os
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_pixelart as px  # noqa: E402

S = 16          # drawing scale (native px -> hi-res px)
OUT = 160       # final size (40 native px x 4)
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_C = os.path.join(ROOT, 'firmware', 'src', 'hd_img.c')
OUT_H = os.path.join(ROOT, 'firmware', 'src', 'hd_img.h')
N = 40 * S


def rgb(c):
    return ((c >> 16) & 255, (c >> 8) & 255, c & 255, 255)


def shift(m, dx, dy):
    out = Image.new('L', m.size, 0)
    out.paste(m, (dx, dy))
    return out


def erode(m, r):
    for _ in range(r):
        m = m.filter(ImageFilter.MinFilter(3))
    return m


class HD:
    def __init__(self):
        self.img = Image.new('RGBA', (N, N), (0, 0, 0, 0))

    def mask_ellipse(self, cx, cy, rx, ry):
        m = Image.new('L', (N, N), 0)
        ImageDraw.Draw(m).ellipse([(cx - rx) * S, (cy - ry) * S, (cx + rx) * S, (cy + ry) * S], fill=255)
        return m

    def mask_poly(self, pts):
        m = Image.new('L', (N, N), 0)
        ImageDraw.Draw(m).polygon([(x * S, y * S) for x, y in pts], fill=255)
        return m

    def paint(self, m, color):
        self.img.paste(Image.new('RGBA', (N, N), rgb(color)), (0, 0), m)

    def part(self, m, base, shade=None, light=None, outline=px.INK, shade_w=1.8, ow=0.6):
        self.paint(m, base)
        if shade is not None:
            k = int(shade_w * S)
            self.paint(ImageChops.subtract(m, shift(m, -k, -k)), shade)
        if light is not None:
            k = int(0.9 * S)
            inner = ImageChops.subtract(m, shift(m, k, k))
            self.paint(ImageChops.subtract(inner, ImageChops.subtract(m, shift(m, -int(shade_w * S), -int(shade_w * S)))), light)
        if outline is not None:
            self.paint(ImageChops.subtract(m, erode(m, int(ow * S))), outline)

    def line(self, pts, color, w):
        d = ImageDraw.Draw(self.img)
        d.line([(x * S, y * S) for x, y in pts], fill=rgb(color), width=int(w * S), joint='curve')
        for x, y in (pts[0], pts[-1]):
            r = w * S / 2
            d.ellipse([x * S - r, y * S - r, x * S + r, y * S + r], fill=rgb(color))


def mirror_pts(pts):
    return [(40 - x, y) for x, y in pts]


def dragon_hd(stage):
    name, body, shade, light, belly, wing, horn, accent, ws, ex = stage
    h = HD()
    big = 'big' in ex
    wing_shade = px.mix(wing, 0x402040, 0.28)
    horn_shade = px.mix(horn, 0x804000, 0.3)
    wl = px.scale_pts([(12, 23), (8, 15), (4, 11), (5, 16), (1, 18), (4, 21), (1, 25), (5, 26), (6, 29), (12, 29)], 12, 26, ws)
    for pts in (wl, mirror_pts(wl)):
        h.part(h.mask_poly(pts), wing, wing_shade, px.mix(wing, 0xFFFFFF, 0.35))
    for bone in ([(12, 25), (5, 12)], [(12, 26), (2, 19)], [(12, 27), (2, 25)]):
        b = px.scale_pts(bone, 12, 26, ws)
        h.line(b, wing_shade, 0.6)
        h.line(mirror_pts(b), wing_shade, 0.6)
    h.part(h.mask_poly([(26, 31), (32, 33), (36, 31), (37, 26), (34, 28), (33, 30), (28, 29)]), body, shade, light)
    h.part(h.mask_poly([(35, 26), (37, 22), (39, 26), (37, 27)]), accent, px.mix(accent, 0x000000, 0.25))
    h.part(h.mask_ellipse(20, 29, 10 if big else 9, 7.5), body, shade, light)
    for fx in (15, 25):
        h.part(h.mask_ellipse(fx, 35.5, 3.6, 2.4), body, shade, light)
        for cx in (fx - 2, fx, fx + 2):
            h.part(h.mask_ellipse(cx + 0.5, 37.2, 0.6, 0.6), 0xFFFFFF, None, None, outline=None)
    for ax in (11.5, 28.5):
        h.part(h.mask_ellipse(ax, 28.5, 2.4, 3.2), body, shade, light)
    bel = h.mask_ellipse(20, 30, 5.6 if big else 5, 5.6)
    h.part(bel, belly, px.mix(belly, 0xE0A040, 0.25), None, outline=shade, ow=0.5)
    for yy in (28.2, 30.2, 32.2):
        h.line([(16.8, yy), (23.2, yy)], px.mix(belly, 0xE0A040, 0.35), 0.45)
    if 'spikes' in ex:
        for sx in (16, 20, 24):
            h.part(h.mask_poly([(sx - 2, 7), (sx, 2 if sx == 20 else 4), (sx + 2, 7)]), accent, px.mix(accent, 0x000000, 0.25))
    hl = 'horns+' in ex
    horn_l = [(11, 9), (16, 8), (14, 2), (11, -1)] if hl else [(12, 9), (16, 8), (13, 2), (12, 2)]
    for pts in (horn_l, mirror_pts(horn_l)):
        h.part(h.mask_poly(pts), horn, horn_shade, px.mix(horn, 0xFFFFFF, 0.5), shade_w=1.0)
    if 'horn2' in ex:
        for pts in ([(9, 10), (11, 8), (8, 5)], mirror_pts([(9, 10), (11, 8), (8, 5)])):
            h.part(h.mask_poly(pts), horn, horn_shade, None, shade_w=1.0)
    fin = [(9, 11), (2, 9), (4, 13), (2, 16), (9, 16)]
    for pts in (fin, mirror_pts(fin)):
        h.part(h.mask_poly(pts), wing, wing_shade, None)
    h.part(h.mask_ellipse(20, 15.5, 13, 9.2), body, shade, light)
    shine = h.mask_ellipse(13.8, 10.6, 2.6, 1.2)
    h.paint(shine, px.mix(body, 0xFFFFFF, 0.45))
    if 'crown' in ex:
        h.part(h.mask_poly([(14, 7), (14, 2), (16, 4), (18, 1), (20, 4), (22, 1), (24, 4), (26, 2), (26, 7)]), 0xFFD23C, 0xD9A019, 0xFFF0A0)
        h.part(h.mask_ellipse(20, 5, 1.1, 1.1), 0xFF4D6D, None, None, outline=None)
    if 'halo' in ex:
        ring = ImageChops.subtract(h.mask_ellipse(20, 2.5, 7, 2.2), h.mask_ellipse(20, 2.5, 5.4, 1.2))
        h.paint(ring, 0xFFE27A)
    if 'sparkle' in ex:
        for sx, sy in ((3, 4), (35, 8), (2, 31), (37, 36)):
            h.part(h.mask_poly([(sx, sy - 1.6), (sx + 0.5, sy - 0.5), (sx + 1.6, sy), (sx + 0.5, sy + 0.5), (sx, sy + 1.6),
                                (sx - 0.5, sy + 0.5), (sx - 1.6, sy), (sx - 0.5, sy - 0.5)]), 0xFFFFFF, None, None, outline=None)
    return h.img.resize((OUT, OUT), Image.LANCZOS)


def egg_hd(cold):
    h = HD()
    shell = h.mask_ellipse(20, 22, 10.5, 13.2)
    base = 0xDCEFFF if cold else 0xFFFBEA
    h.part(shell, base, 0xB8C4E8 if cold else 0xC9C3E4, 0xFFFFFF, shade_w=3.2)
    for sx, sy, r, col in ((15.5, 15, 2.6, 0xF0507A), (24, 12, 2.2, 0x2EA8F0), (20, 23, 3.2, 0x2EA8F0), (13, 26, 2.2, 0x2EA8F0),
                           (25, 29, 2.6, 0xD02E6A)):
        if cold:
            col = px.mix(col, 0xDCEFFF, 0.55)
        spot = ImageChops.multiply(h.mask_ellipse(sx, sy, r, r), erode(shell, int(0.7 * S)))
        h.part(spot, col, px.mix(col, 0x000000, 0.2), px.mix(col, 0xFFFFFF, 0.4), outline=None, shade_w=0.8)
    h.paint(h.mask_ellipse(15, 12.5, 1.2, 2), 0xFFFFFF)
    return h.img.resize((OUT, OUT), Image.LANCZOS)


def to_indexed(img):
    """RGBA -> LVGL INDEXED_8BIT: 256-entry palette (B, G, R, A) followed by one index per pixel."""
    q = img.quantize(colors=255, method=Image.Quantize.FASTOCTREE)
    pal = q.getpalette('RGBA')[:255 * 4] if hasattr(q, 'getpalette') else None
    pal = q.palette.colors  # {(r,g,b,a): index}
    entries = [(0, 0, 0, 0)] * 256
    for col, idx in pal.items():
        if idx < 256:
            entries[idx] = col if len(col) == 4 else col + (255,)
    data = bytearray()
    for r, g, b, a in entries:
        data += bytes((b, g, r, a))
    data += bytes(q.getdata())
    return data


def main():
    out = [(f'hd_{st[0]}', dragon_hd(st)) for st in px.STAGES] + [('hd_egg', egg_hd(False)), ('hd_egg_cold', egg_hd(True))]
    if len(sys.argv) > 1:
        sheet = Image.new('RGBA', (8 * 170 + 10, 180), (40, 52, 98, 255))
        for i, (_, im) in enumerate(out):
            sheet.alpha_composite(im, (10 + i * 170, 10))
        sheet.save(sys.argv[1])
    total = 0
    with open(OUT_C, 'w', encoding='utf-8', newline='\n') as f:
        f.write('// Generated by tools/gen_hd.py - do not edit.\n#include "hd_img.h"\n\n')
        for name, im in out:
            data = to_indexed(im)
            total += len(data)
            f.write(f'static const uint8_t {name}_map[] = {{\n')
            for i in range(0, len(data), 24):
                f.write('  ' + ','.join(f'0x{x:02x}' for x in data[i:i + 24]) + ',\n')
            f.write('};\n')
            f.write(f'const lv_img_dsc_t {name} = {{\n  .header = {{.cf = LV_IMG_CF_INDEXED_8BIT, .always_zero = 0, .reserved = 0, .w = {OUT}, .h = {OUT}}},\n'
                    f'  .data_size = {len(data)},\n  .data = {name}_map,\n}};\n\n')
    with open(OUT_H, 'w', encoding='utf-8', newline='\n') as f:
        f.write('#pragma once\n// Generated by tools/gen_hd.py - do not edit.\n#include <lvgl.h>\n\n#ifdef __cplusplus\nextern "C" {\n#endif\n')
        for name, _ in out:
            f.write(f'extern const lv_img_dsc_t {name};\n')
        f.write('#ifdef __cplusplus\n}\n#endif\n')
    print(f'{len(out)} images, {total // 1024} KB')


if __name__ == '__main__':
    main()
