"""Pixel-art scene and UI sprites for the "Luyện rồng" game.

    python tools/gen_scene.py [preview.png]

Output: firmware/src/scene_img.c + scene_img.h
  sc_garden   480x192 garden background (drawn at 240x96, scaled x2 nearest), LV_IMG_CF_TRUE_COLOR
  ui_*        32x32 button icons, 16x16 stat icons, 24x24 mood faces, panel stripe tile (TRUE_COLOR_ALPHA)
"""
import os
import random
import sys

from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_dragons  # noqa: E402  (to_lvgl)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_C = os.path.join(ROOT, 'firmware', 'src', 'scene_img.c')
OUT_H = os.path.join(ROOT, 'firmware', 'src', 'scene_img.h')


def h(c):
    return ((c >> 16) & 255, (c >> 8) & 255, c & 255)


def garden():
    W, H = 240, 96
    rnd = random.Random(7)
    im = Image.new('RGB', (W, H))
    d = ImageDraw.Draw(im)
    # sky: soft bands with the faint vertical stripes of the reference style
    bands = [0x86D3F5, 0x95D9F6, 0xA4DFF7, 0xB4E6F8, 0xC4ECF9]
    for y in range(0, 60):
        c = h(bands[min(4, y // 11)])
        for x in range(W):
            cc = tuple(min(255, v + 8) for v in c) if x % 4 == 0 else c
            im.putpixel((x, y), cc)
    # sun
    sx, sy = 202, 15
    for r, col in ((10, 0xFFF4B8), (8, 0xFFE070), (6, 0xFFD23C)):
        d.ellipse([sx - r, sy - r, sx + r, sy + r], fill=h(col))
    for dx, dy in ((0, -14), (0, 14), (-14, 0), (14, 0), (-10, -10), (10, -10), (-10, 10), (10, 10)):
        d.rectangle([sx + dx - 1, sy + dy - 1, sx + dx, sy + dy], fill=h(0xFFE070))

    def cloud(x, y, s):
        for ox, oy, r in ((0, 3, 4), (5, 0, 6), (12, 2, 5), (17, 4, 4)):
            d.ellipse([x + (ox - r) * s, y + (oy - r) * s, x + (ox + r) * s, y + (oy + r) * s], fill=h(0xFFFFFF))
        d.rectangle([x - 3 * s, y + 4 * s, x + 20 * s, y + 7 * s], fill=h(0xDDEFFA))
    cloud(70, 10, 1)
    cloud(120, 22, 1)
    cloud(18, 26, 1)
    # far hills
    d.ellipse([-30, 40, 90, 90], fill=h(0x9BD77A))
    d.ellipse([60, 44, 190, 96], fill=h(0xA6DD85))
    d.ellipse([150, 38, 280, 96], fill=h(0x9BD77A))

    def tree(x, y, r):
        d.rectangle([x - 2, y, x + 2, y + 18], fill=h(0x8B5A2B))
        d.rectangle([x + 1, y, x + 2, y + 18], fill=h(0x6E4420))
        for ox, oy, rr, col in ((0, 0, r, 0x2E8B3E), (-r // 2, -r // 2, r - 2, 0x3FA14A), (r // 2, -r // 3, r - 3, 0x3FA14A),
                                (0, -r + 2, r - 4, 0x55B95A), (-2, -r // 2 - 2, r // 2, 0x7ED957)):
            d.ellipse([x + ox - rr, y + oy - rr, x + ox + rr, y + oy + rr], fill=h(col))
    for x, y, r in ((10, 40, 14), (34, 44, 11), (226, 38, 14), (204, 46, 10)):
        tree(x, y, r)
    # picket fence
    for x in range(44, 196, 6):
        d.polygon([(x, 49), (x + 2, 47), (x + 4, 49), (x + 4, 60), (x, 60)], fill=h(0xFFFFFF))
        d.line([(x + 4, 49), (x + 4, 60)], fill=h(0xC9D8EA))
    d.rectangle([44, 52, 198, 53], fill=h(0xFFFFFF))
    d.rectangle([44, 57, 198, 58], fill=h(0xFFFFFF))
    d.line([(44, 54), (198, 54)], fill=h(0xC9D8EA))
    # grass
    d.rectangle([0, 60, W, H], fill=h(0x7ED957))
    for y in range(60, H, 3):
        for x in range((y * 7) % 5, W, 9):
            d.point((x, y), fill=h(0x6CCB48))
    # bushes along the fence with flowers
    for x in range(0, W, 14):
        d.ellipse([x - 8, 55, x + 8, 66], fill=h(0x5BBF3A))
        d.ellipse([x - 5, 54, x + 4, 61], fill=h(0x6FCF4A))
    for _ in range(26):
        x, y = rnd.randrange(0, W), rnd.randrange(54, 64)
        d.point((x, y), fill=h(rnd.choice([0xFF8FB8, 0xFFE070, 0xFFFFFF, 0xFF6B6B])))
    # tufts and flowers on the lawn
    for _ in range(60):
        x, y = rnd.randrange(0, W), rnd.randrange(66, H)
        d.line([(x, y), (x - 1, y - 2)], fill=h(0x5BBF3A))
        d.line([(x, y), (x + 1, y - 2)], fill=h(0x5BBF3A))
    for _ in range(22):
        x, y = rnd.randrange(0, W), rnd.randrange(68, H)
        c = h(rnd.choice([0xFFFFFF, 0xFF8FB8, 0xFFE070]))
        d.point((x, y), fill=c)
        d.point((x + 1, y), fill=c)
        d.point((x, y + 1), fill=h(0xFFD23C))
    # stepping-stone path
    for x, y, w in ((38, 78, 10), (62, 86, 11), (92, 90, 12), (126, 88, 12), (158, 84, 11), (186, 79, 10)):
        d.ellipse([x - w, y - 3, x + w, y + 4], fill=h(0xC8C2B4))
        d.ellipse([x - w, y - 4, x + w, y + 2], fill=h(0xEDE8DC))
    return im.resize((W * 2, H * 2), Image.NEAREST)


class P:  # 16x16 pixel icon canvas, exported scaled
    def __init__(self, n=16):
        self.n = n
        self.im = Image.new('RGBA', (n, n), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im)

    def ell(self, box, c):
        self.d.ellipse(box, fill=h(c) + (255,))

    def rect(self, box, c):
        self.d.rectangle(box, fill=h(c) + (255,))

    def poly(self, pts, c):
        self.d.polygon(pts, fill=h(c) + (255,))

    def px(self, x, y, c):
        self.im.putpixel((x, y), h(c) + (255,))

    def done(self, scale, outline=True):
        im = self.im
        if outline:
            out = Image.new('RGBA', im.size, (0, 0, 0, 0))
            a = im.split()[3]
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                sh = Image.new('RGBA', im.size, (0, 0, 0, 0))
                sh.paste(Image.new('RGBA', im.size, (90, 52, 28, 255)), (dx, dy), a)
                out.alpha_composite(sh)
            out.alpha_composite(im)
            im = out
        return im.resize((self.n * scale, self.n * scale), Image.NEAREST)


def ic_food():
    p = P()
    p.ell([2, 3, 11, 11], 0xC8733A)
    p.ell([3, 4, 9, 8], 0xE08A4A)
    p.rect([9, 9, 13, 11], 0xF6EEDC)
    p.ell([11, 8, 14, 12], 0xF6EEDC)
    return p.done(2)


def ic_play():
    p = P()
    p.ell([2, 2, 13, 13], 0xFF6B6B)
    p.poly([(2, 7), (13, 7), (13, 9), (2, 9)], 0xFFFFFF)
    p.ell([6, 6, 9, 9], 0x4C9AFF)
    p.px(5, 4, 0xFFFFFF)
    return p.done(2)


def ic_bath():
    p = P()
    for x, y, r in ((5, 9, 4), (10, 6, 3), (11, 11, 2)):
        p.ell([x - r, y - r, x + r, y + r], 0xBFEBFF)
        p.px(x - r // 2, y - r // 2, 0xFFFFFF)
    return p.done(2)


def ic_sleep():
    p = P()
    p.ell([2, 2, 12, 12], 0xFFE070)
    p.ell([5, 0, 15, 10], (0, 0, 0)[0] or 0x000000)
    p.im.putalpha(Image.eval(p.im.split()[3], lambda a: a))
    # cut the crescent: clear the second circle
    mask = Image.new('L', p.im.size, 0)
    ImageDraw.Draw(mask).ellipse([5, 0, 15, 10], fill=255)
    p.im.paste((0, 0, 0, 0), (0, 0), mask)
    p.rect([11, 11, 14, 11], 0x6E7BD8)
    p.rect([14, 12, 14, 12], 0x6E7BD8)
    p.rect([11, 13, 14, 13], 0x6E7BD8)
    return p.done(2)


def ic_med():
    p = P()
    p.poly([(3, 9), (9, 3), (13, 7), (7, 13)], 0xFFFFFF)
    p.poly([(3, 9), (6, 6), (10, 10), (7, 13)], 0xFF6B8A)
    return p.done(2)


def ic_shop():
    p = P()
    p.poly([(1, 7), (8, 1), (15, 7)], 0xE0603C)
    p.rect([3, 7, 13, 14], 0xFFF2D8)
    p.rect([7, 10, 9, 14], 0x8B5A2B)
    p.rect([4, 9, 5, 10], 0x8FD0FF)
    p.rect([11, 9, 12, 10], 0x8FD0FF)
    return p.done(2)


def ic_badge():
    p = P()
    p.poly([(4, 1), (7, 1), (8, 6), (5, 6)], 0x4C9AFF)
    p.poly([(9, 1), (12, 1), (11, 6), (8, 6)], 0xFF6B6B)
    p.ell([3, 5, 13, 15], 0xFFC93C)
    p.ell([6, 8, 10, 12], 0xFFE27A)
    return p.done(2)


def ic_coin():
    p = P(10)
    p.ell([0, 0, 9, 9], 0xFFC93C)
    p.ell([2, 2, 7, 7], 0xFFE27A)
    p.rect([4, 3, 5, 6], 0xE0A020)
    return p.done(2, outline=False)


def stat(kind):
    p = P(8)
    if kind == 'food':
        p.ell([0, 1, 6, 7], 0xC8733A)
        p.rect([5, 5, 7, 6], 0xF6EEDC)
    elif kind == 'mood':
        p.ell([0, 1, 3, 4], 0xFF5C8A)
        p.ell([4, 1, 7, 4], 0xFF5C8A)
        p.poly([(0, 3), (7, 3), (4, 7), (3, 7)], 0xFF5C8A)
    elif kind == 'clean':
        p.poly([(3, 0), (4, 0), (7, 5), (5, 7), (2, 7), (0, 5)], 0x4FB8FF)
        p.px(2, 4, 0xFFFFFF)
    else:  # energy
        p.poly([(4, 0), (1, 4), (3, 4), (2, 7), (6, 3), (4, 3), (5, 0)], 0xFFD23C)
    return p.done(2, outline=False)


def face(kind):
    p = P(12)
    p.ell([0, 0, 11, 11], 0xFFD23C)
    p.ell([1, 1, 10, 10], 0xFFE070)
    if kind == 'happy':
        p.d.arc([2, 2, 5, 6], 200, 340, fill=h(0x5A3A22) + (255,))
        p.d.arc([6, 2, 9, 6], 200, 340, fill=h(0x5A3A22) + (255,))
        p.d.arc([3, 4, 8, 9], 20, 160, fill=h(0x5A3A22) + (255,))
    elif kind == 'ok':
        p.rect([3, 4, 3, 5], 0x5A3A22)
        p.rect([8, 4, 8, 5], 0x5A3A22)
        p.rect([4, 8, 7, 8], 0x5A3A22)
    else:
        p.rect([3, 4, 3, 5], 0x5A3A22)
        p.rect([8, 4, 8, 5], 0x5A3A22)
        p.d.arc([3, 7, 8, 11], 200, 340, fill=h(0x5A3A22) + (255,))
        p.rect([2, 6, 2, 7], 0x4FB8FF)
    return p.done(2)


def stripe():  # panel background tile (drawn tiled): wide, so few tiles per panel
    im = Image.new('RGBA', (64, 8))
    for x in range(64):
        for y in range(8):
            im.putpixel((x, y), (214, 246, 250, 255) if x % 8 < 4 else (198, 239, 246, 255))
    return im


SPRITES = [('ui_stripe', stripe)]  # icons and faces now come from gen_pixelart.py


def main():
    bg = garden()
    sprites = [(n, f()) for n, f in SPRITES]
    if len(sys.argv) > 1:
        sheet = Image.new('RGBA', (480, 192 + 60), (40, 52, 98, 255))
        sheet.paste(bg, (0, 0))
        x = 4
        for _, im in sprites:
            sheet.alpha_composite(im, (x, 196))
            x += im.size[0] + 4
        sheet.save(sys.argv[1])
    total = 0
    with open(OUT_C, 'w', encoding='utf-8', newline='\n') as f:
        f.write('// Generated by tools/gen_scene.py - do not edit.\n#include "scene_img.h"\n\n')
        # background: plain RGB565, no alpha
        data = bytearray()
        for r, g, b in bg.getdata():
            v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            data += bytes((v & 255, v >> 8))
        total += len(data)
        f.write('static const uint8_t sc_garden_map[] = {\n')
        for i in range(0, len(data), 24):
            f.write('  ' + ','.join(f'0x{x:02x}' for x in data[i:i + 24]) + ',\n')
        f.write('};\nconst lv_img_dsc_t sc_garden = {\n  .header = {.cf = LV_IMG_CF_TRUE_COLOR, .always_zero = 0, .reserved = 0, '
                f'.w = {bg.size[0]}, .h = {bg.size[1]}}},\n  .data_size = {len(data)},\n  .data = sc_garden_map,\n}};\n\n')
        for name, im in sprites:
            w, hh, data = gen_dragons.to_lvgl(im)
            total += len(data)
            f.write(f'static const uint8_t {name}_map[] = {{\n')
            for i in range(0, len(data), 24):
                f.write('  ' + ','.join(f'0x{x:02x}' for x in data[i:i + 24]) + ',\n')
            f.write('};\n')
            f.write(f'const lv_img_dsc_t {name} = {{\n  .header = {{.cf = LV_IMG_CF_TRUE_COLOR_ALPHA, .always_zero = 0, .reserved = 0, .w = {w}, .h = {hh}}},\n'
                    f'  .data_size = {len(data)},\n  .data = {name}_map,\n}};\n\n')
    with open(OUT_H, 'w', encoding='utf-8', newline='\n') as f:
        f.write('#pragma once\n// Generated by tools/gen_scene.py - do not edit.\n#include <lvgl.h>\n\n#ifdef __cplusplus\nextern "C" {\n#endif\n')
        f.write('extern const lv_img_dsc_t sc_garden;\n')
        for name, _ in sprites:
            f.write(f'extern const lv_img_dsc_t {name};\n')
        f.write('#ifdef __cplusplus\n}\n#endif\n')
    print(f'{len(sprites) + 1} images, {total // 1024} KB')


if __name__ == '__main__':
    main()
