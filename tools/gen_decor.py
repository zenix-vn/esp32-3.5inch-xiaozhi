"""Draws the dragon room decorations and small effect sprites, written as LVGL images.

    python tools/gen_decor.py [preview.png]

Output: firmware/src/decor_img.c + decor_img.h (LV_IMG_CF_TRUE_COLOR_ALPHA).
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_dragons  # noqa: E402

SS = 0.5  # drawn at half size without smoothing, then doubled with nearest-neighbour: pixel-art look
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_C = os.path.join(ROOT, 'firmware', 'src', 'decor_img.c')
OUT_H = os.path.join(ROOT, 'firmware', 'src', 'decor_img.h')


def hexc(h, a=255):
    return ((h >> 16) & 255, (h >> 8) & 255, h & 255, a)


class C:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.img = Image.new('RGBA', (int(w * SS), int(h * SS)), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.img)

    def ell(self, cx, cy, rx, ry, col):
        self.d.ellipse([(cx - rx) * SS, (cy - ry) * SS, (cx + rx) * SS, (cy + ry) * SS], fill=hexc(col) if isinstance(col, int) else col)

    def rect(self, x0, y0, x1, y1, r, col):
        self.d.rounded_rectangle([x0 * SS, y0 * SS, x1 * SS, y1 * SS], radius=r * SS, fill=hexc(col) if isinstance(col, int) else col)

    def poly(self, pts, col):
        self.d.polygon([(x * SS, y * SS) for x, y in pts], fill=hexc(col) if isinstance(col, int) else col)

    def line(self, pts, w, col):
        self.d.line([(x * SS, y * SS) for x, y in pts], fill=hexc(col), width=max(1, int(w * SS)), joint='curve')

    def done(self, outline=True):
        img = self.img
        if outline:
            a = img.split()[3].filter(ImageFilter.MaxFilter(3))
            out = Image.new('RGBA', img.size, (0, 0, 0, 0))
            out.paste(Image.new('RGBA', img.size, (30, 20, 50, 255)), (0, 0), a)
            out.alpha_composite(img)
            img = out
        return img.resize((self.w, self.h), Image.NEAREST)


def plant():
    c = C(40, 56)
    c.rect(9, 36, 31, 54, 4, 0xC8693E)
    c.rect(7, 34, 33, 40, 3, 0xE07C4A)
    for (x, y, rx, ry, col) in ((20, 22, 7, 14, 0x2F9E5B), (11, 26, 6, 11, 0x3DBA6E), (29, 26, 6, 11, 0x3DBA6E), (20, 12, 5, 10, 0x55D184)):
        c.ell(x, y, rx, ry, col)
    return c.done()


def lamp():
    c = C(36, 64)
    c.ell(18, 60, 11, 3, 0x6B4F2A)
    c.rect(16, 24, 20, 60, 2, 0x8A6A3A)
    c.poly([(6, 26), (30, 26), (24, 6), (12, 6)], 0xFFE08A)
    c.rect(6, 24, 30, 28, 2, 0xF0C060)
    return c.done()


def rug():
    c = C(150, 26)
    c.ell(75, 13, 72, 12, 0xB0457B)
    c.ell(75, 13, 60, 9, 0xD9649A)
    c.ell(75, 13, 40, 6, 0xF08FB8)
    return c.done(outline=False)


def painting():
    c = C(54, 42)
    c.rect(1, 1, 53, 41, 3, 0xC89B4A)
    c.rect(5, 5, 49, 37, 2, 0x8FD0FF)
    c.poly([(5, 37), (20, 18), (30, 30), (37, 22), (49, 37)], 0x3DA36E)
    c.ell(40, 13, 5, 5, 0xFFE27A)
    return c.done()


def lantern():
    c = C(34, 60)
    c.line([(17, 0), (17, 12)], 1.5, 0x6B4F2A)
    c.rect(11, 10, 23, 14, 2, 0xFFC93C)
    c.ell(17, 30, 14, 16, 0xE8403C)
    for dx in (-8, 0, 8):
        c.line([(17 + dx * 0.6, 15), (17 + dx, 30), (17 + dx * 0.6, 45)], 1.2, 0xB82A2A)
    c.rect(11, 44, 23, 48, 2, 0xFFC93C)
    c.line([(17, 48), (17, 58)], 2, 0xFFC93C)
    return c.done()


def chest():
    c = C(50, 40)
    c.rect(3, 16, 47, 38, 4, 0x9A5A2A)
    c.rect(3, 6, 47, 20, 8, 0xB86E34)
    c.rect(3, 16, 47, 19, 1, 0xFFC93C)
    c.rect(21, 16, 29, 26, 2, 0xFFC93C)
    for x, y in ((10, 6), (24, 3), (38, 6)):
        c.ell(x, y, 4, 4, 0xFFE27A)
    return c.done()


def heart():
    c = C(18, 16)
    c.ell(5.5, 5.5, 4.5, 4.5, 0xFF5C8A)
    c.ell(12.5, 5.5, 4.5, 4.5, 0xFF5C8A)
    c.poly([(1.6, 7.5), (16.4, 7.5), (9, 15)], 0xFF5C8A)
    c.ell(4.5, 4, 1.6, 1.4, (255, 255, 255, 200))
    return c.done(outline=False)


def bubble():
    c = C(16, 16)
    c.ell(8, 8, 7, 7, (200, 235, 255, 150))
    c.ell(5.5, 5.5, 2, 2, (255, 255, 255, 230))
    return c.done(outline=False)


def food(kind):
    c = C(26, 26)
    if kind == 0:  # apple
        c.ell(13, 15, 9, 9, 0xE8403C)
        c.rect(12, 3, 14, 8, 1, 0x6B4F2A)
        c.ell(17, 5, 4, 2, 0x3DBA6E)
    elif kind == 1:  # grilled meat
        c.rect(15, 15, 25, 19, 2, 0xF2E6D0)
        c.ell(11, 11, 9, 8, 0xA0522D)
        c.ell(10, 9, 5, 3, 0xC8733A)
    else:  # jewel cake
        c.rect(4, 12, 22, 23, 3, 0xF7C8D8)
        c.rect(4, 10, 22, 15, 3, 0xFFFFFF)
        c.poly([(13, 2), (17, 7), (13, 11), (9, 7)], 0x38E1FF)
    return c.done()


def coin():
    c = C(18, 18)
    c.ell(9, 9, 8, 8, 0xFFC93C)
    c.ell(9, 9, 5.5, 5.5, 0xFFE27A)
    c.rect(8, 5, 10, 13, 1, 0xE0A020)
    return c.done(outline=False)


def flyer():  # the lv1 pixel dragon, doubled
    import gen_pixelart
    img = gen_pixelart.dragon(gen_pixelart.STAGES[1], 'happy')
    img = img.crop(img.getbbox())
    return img.resize((img.size[0] * 2, img.size[1] * 2), Image.NEAREST)


SPRITES = [
    ('dc_plant', plant), ('dc_lamp', lamp), ('dc_rug', rug), ('dc_painting', painting), ('dc_lantern', lantern), ('dc_chest', chest),
    ('fx_bubble', bubble), ('fx_flyer', flyer),  # hearts, food and coins come from gen_pixelart.py
]


def main():
    out = [(n, f()) for n, f in SPRITES]
    if len(sys.argv) > 1:
        W = sum(im.size[0] + 10 for _, im in out) + 10
        sheet = Image.new('RGBA', (W, 90), (40, 52, 98, 255))
        x = 10
        for _, im in out:
            sheet.alpha_composite(im, (x, 10))
            x += im.size[0] + 10
        sheet.resize((W * 2, 180), Image.NEAREST).save(sys.argv[1])
    total = 0
    with open(OUT_C, 'w', encoding='utf-8', newline='\n') as f:
        f.write('// Generated by tools/gen_decor.py - do not edit.\n#include "decor_img.h"\n\n')
        for name, im in out:
            w, h, data = gen_dragons.to_lvgl(im)
            total += len(data)
            f.write(f'static const uint8_t {name}_map[] = {{\n')
            for i in range(0, len(data), 24):
                f.write('  ' + ','.join(f'0x{x:02x}' for x in data[i:i + 24]) + ',\n')
            f.write('};\n')
            f.write(f'const lv_img_dsc_t {name} = {{\n  .header = {{.cf = LV_IMG_CF_TRUE_COLOR_ALPHA, .always_zero = 0, .reserved = 0, .w = {w}, .h = {h}}},\n'
                    f'  .data_size = {len(data)},\n  .data = {name}_map,\n}};\n\n')
    with open(OUT_H, 'w', encoding='utf-8', newline='\n') as f:
        f.write('#pragma once\n// Generated by tools/gen_decor.py - do not edit.\n#include <lvgl.h>\n\n#ifdef __cplusplus\nextern "C" {\n#endif\n')
        for name, _ in out:
            f.write(f'extern const lv_img_dsc_t {name};\n')
        f.write('#ifdef __cplusplus\n}\n#endif\n')
    print(f'{len(out)} sprites, {total // 1024} KB')


if __name__ == '__main__':
    main()
