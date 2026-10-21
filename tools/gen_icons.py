"""Draws the launcher app icons (56x56, rounded gradient tile + white glyph) and writes them as LVGL images.

    python tools/gen_icons.py [preview.png]

Output: firmware/src/icons_img.c + icons_img.h (LV_IMG_CF_TRUE_COLOR_ALPHA). The dragon icon reuses the
baby dragon from gen_dragons.py.
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_dragons  # noqa: E402

SZ = 56
SS = 8
K = SZ * SS / 100.0  # glyphs are drawn on a 100x100 grid
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_C = os.path.join(ROOT, 'firmware', 'src', 'icons_img.c')
OUT_H = os.path.join(ROOT, 'firmware', 'src', 'icons_img.h')
WHITE = (255, 255, 255, 255)


def hexc(h, a=255):
    return ((h >> 16) & 255, (h >> 8) & 255, h & 255, a)


def tile(c1, c2):
    n = SZ * SS
    grad = Image.new('RGBA', (n, n))
    g = ImageDraw.Draw(grad)
    a, b = hexc(c1), hexc(c2)
    for y in range(n):
        t = y / (n - 1)
        g.line([(0, y), (n, y)], fill=tuple(int(a[i] * (1 - t) + b[i] * t) for i in range(3)) + (255,))
    mask = Image.new('L', (n, n), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, n - 1, n - 1], radius=int(n * 0.26), fill=255)
    out = Image.new('RGBA', (n, n), (0, 0, 0, 0))
    out.paste(grad, (0, 0), mask)
    hl = Image.new('RGBA', (n, n), (0, 0, 0, 0))  # glossy top
    ImageDraw.Draw(hl).rounded_rectangle([0, 0, n - 1, int(n * 0.5)], radius=int(n * 0.26), fill=(255, 255, 255, 34))
    hl.putalpha(Image.composite(hl.split()[3], Image.new('L', (n, n), 0), mask))
    out.alpha_composite(hl)
    return out


class G:
    def __init__(self, img):
        self.img = img
        self.d = ImageDraw.Draw(img)

    def P(self, pts):
        return [(x * K, y * K) for x, y in pts]

    def ell(self, cx, cy, rx, ry, col=WHITE):
        self.d.ellipse([(cx - rx) * K, (cy - ry) * K, (cx + rx) * K, (cy + ry) * K], fill=col)

    def rect(self, x0, y0, x1, y1, r, col=WHITE):
        self.d.rounded_rectangle([x0 * K, y0 * K, x1 * K, y1 * K], radius=r * K, fill=col)

    def poly(self, pts, col=WHITE):
        self.d.polygon(self.P(pts), fill=col)

    def arc(self, cx, cy, r, a0, a1, w, col=WHITE):
        self.d.arc([(cx - r) * K, (cy - r) * K, (cx + r) * K, (cy + r) * K], a0, a1, fill=col, width=int(w * K))

    def line(self, pts, w, col=WHITE):
        self.d.line(self.P(pts), fill=col, width=int(w * K), joint='curve')
        for x, y in (pts[0], pts[-1]):
            self.ell(x, y, w / 2, w / 2, col)


def glyph_assistant(g, c):
    # four rounded bars of different heights in blue / red / yellow / green: a voice "speaking" (like a sound wave)
    bars = ((0x4285F4, 44), (0xEA4335, 52), (0xFBBC05, 70), (0x34A853, 40))
    w, gap = 15, 6
    x = (100 - (4 * w + 3 * gap)) / 2
    for col, h in bars:
        g.rect(x, 50 - h / 2, x + w, 50 + h / 2, w / 2, hexc(col))
        x += w + gap


def glyph_weather(g, c):
    g.ell(38, 38, 15, 15, hexc(0xFFE27A))
    for a in range(0, 360, 45):
        r = math.radians(a)
        g.line([(38 + 20 * math.cos(r), 38 + 20 * math.sin(r)), (38 + 25 * math.cos(r), 38 + 25 * math.sin(r))], 4, hexc(0xFFE27A))
    g.ell(48, 62, 13, 13)
    g.ell(62, 54, 16, 16)
    g.ell(76, 63, 11, 11)
    g.rect(40, 60, 84, 74, 7)


def glyph_photos(g, c):
    g.rect(16, 24, 84, 76, 10)
    g.rect(22, 30, 78, 70, 6, hexc(c))
    g.poly([(24, 68), (42, 44), (56, 60), (64, 52), (76, 68)])
    g.ell(66, 40, 6, 6)


def glyph_wifi(g, c):
    for r in (34, 23, 12):
        g.arc(50, 70, r, 225, 315, 7)
    g.ell(50, 70, 6, 6)


def glyph_storage(g, c):
    g.poly([(30, 18), (64, 18), (74, 28), (74, 82), (30, 82)])
    for i in range(4):
        g.rect(38 + i * 8, 24, 43 + i * 8, 36, 1.5, hexc(c))


def glyph_settings(g, c):
    for a in range(0, 360, 45):
        r = math.radians(a)
        cx, cy = 50 + 26 * math.cos(r), 50 + 26 * math.sin(r)
        g.ell(cx, cy, 8, 8)
    g.ell(50, 50, 27, 27)
    g.ell(50, 50, 11, 11, hexc(c))


def glyph_about(g, c):
    g.ell(50, 50, 32, 32)
    g.ell(50, 34, 5, 5, hexc(c))
    g.rect(45, 44, 55, 70, 4, hexc(c))


def glyph_clock(g, c):
    g.ell(50, 52, 31, 31)
    g.ell(50, 52, 25, 25, hexc(c))
    g.line([(50, 52), (50, 34)], 5)
    g.line([(50, 52), (63, 59)], 5)
    g.ell(50, 52, 4, 4)
    g.rect(28, 14, 40, 20, 3)
    g.rect(60, 14, 72, 20, 3)


def glyph_sleep(g, c):
    clear = (0, 0, 0, 0)
    g.ell(46, 52, 27, 27, hexc(0xFFE27A))  # crescent moon: a disc minus a shifted disc
    g.ell(58, 44, 24, 24, clear)
    for x, y, r in ((70, 30, 4.2), (80, 52, 3), (62, 66, 2.6)):  # stars
        g.ell(x, y, r, r, WHITE)


def glyph_games(g, c):
    g.rect(14, 32, 86, 72, 20)  # controller body
    g.ell(26, 70, 10, 10)
    g.ell(74, 70, 10, 10)
    g.rect(29, 44, 41, 56, 2, hexc(c))  # d-pad cross
    g.rect(24, 49, 46, 55, 2, hexc(c))
    g.rect(31.5, 41, 38.5, 59, 2, hexc(c))
    for x, y, col in ((66, 46, 0xFFC93C), (76, 52, 0x4FC3F7), (66, 58, 0xFF6B8A)):  # buttons
        g.ell(x, y, 4.6, 4.6, hexc(col))
    g.rect(44, 36, 56, 41, 2, hexc(c))  # a notch on top


def glyph_qr(g, c):
    for x, y in ((20, 20), (58, 20), (20, 58)):
        g.rect(x, y, x + 22, y + 22, 4)
        g.rect(x + 5, y + 5, x + 17, y + 17, 2, hexc(c))
        g.rect(x + 8, y + 8, x + 14, y + 14, 1)
    for x, y in ((60, 60), (72, 60), (60, 72), (74, 74), (48, 48), (48, 64), (64, 46)):
        g.rect(x, y, x + 8, y + 8, 1.5)


def glyph_2048(g, c):
    cols = (WHITE, (255, 255, 255, 150), (255, 226, 122, 255), (255, 255, 255, 90))
    for i, (x, y) in enumerate(((18, 18), (52, 18), (18, 52), (52, 52))):
        g.rect(x, y, x + 30, y + 30, 7, cols[i])
    g.line([(26, 30), (40, 30)], 5, hexc(c))  # "2" on the first tile, bars on the gold one
    g.line([(26, 36), (40, 36)], 5, hexc(c))
    for k in range(3):
        g.rect(58, 58 + k * 7, 76, 61 + k * 7, 1.5, hexc(c))


def glyph_caro(g, c):
    for k in (38, 62):
        g.line([(k, 16), (k, 84)], 4, (255, 255, 255, 150))
        g.line([(16, k), (84, k)], 4, (255, 255, 255, 150))
    g.line([(19, 19), (33, 33)], 6)
    g.line([(33, 19), (19, 33)], 6)
    g.line([(43, 43), (57, 43 + 14)], 6)
    g.line([(57, 43), (43, 57)], 6)
    g.arc(74, 50, 8, 0, 360, 5, hexc(0xFFE27A))
    g.arc(26, 74, 8, 0, 360, 5, hexc(0xFFE27A))
    g.arc(74, 74, 8, 0, 360, 5, hexc(0xFFE27A))


def dragon_icon(c1, c2):
    out = tile(c1, c2)
    import gen_pixelart  # the baby pixel dragon, scaled up with whole pixels
    img = gen_pixelart.dragon(gen_pixelart.STAGES[0], 'happy')
    img = img.crop(img.getbbox())
    n = SZ * SS
    k = max(1, int(n * 0.9 / max(img.size)))
    img = img.resize((img.size[0] * k, img.size[1] * k), Image.NEAREST)
    w, h = img.size
    out.alpha_composite(img, ((n - w) // 2, (n - h) // 2 + int(n * 0.04)))
    return out


ICONS = [
    # name, top colour, bottom colour, glyph
    ('ic_assistant', 0xFFFFFF, 0xA3B4F0, glyph_assistant),  # light gradient tile so the coloured bars stand out
    ('ic_weather', 0x5AB0FF, 0x2F5FD0, glyph_weather),
    ('ic_dragon', 0xB38BFF, 0x5B2BD6, None),
    ('ic_photos', 0xFFB84A, 0xE8602A, glyph_photos),
    ('ic_wifi', 0x3DEBA0, 0x13906E, glyph_wifi),
    ('ic_storage', 0x6FB6FF, 0x3B5FD6, glyph_storage),
    ('ic_settings', 0xA3ACCB, 0x4E5878, glyph_settings),
    ('ic_about', 0x7BDCB5, 0x2A7F86, glyph_about),
    ('ic_clock', 0xFF8FA3, 0xD03A6A, glyph_clock),
    ('ic_qr', 0xFFD54A, 0xD98A12, glyph_qr),
    ('ic_sleep', 0x6C7CFF, 0x2A2478, glyph_sleep),
    ('ic_games', 0xC77BFF, 0x7A2BD6, glyph_games),
    ('ic_calendar', 0xFFF6DC, 0xE3AC52, 'logo'),  # the Zin100 calendar logo (tools/assets/calendar_logo.png) on a cream-to-gold tile
    ('ic_2048', 0xFFB86B, 0xE0603A, glyph_2048),
    ('ic_caro', 0x6BE0C8, 0x2A78B8, glyph_caro),
]


def logo_icon(path, c1, c2):
    """A rounded gradient tile like the other icons, with the logo (transparent background) fitted inside it."""
    big = tile(c1, c2)
    logo = Image.open(path).convert('RGBA')
    x0, y0, x1, y1 = logo.getbbox()
    logo = logo.crop((x0 + 22, y0, x1 - 23, y1))  # the beak and tail tips are thin: trimming them lets the calendar be larger
    n = SZ * SS
    k = (n * 0.84) / logo.size[0]  # the logo is wide: it spans 84% of the tile
    logo = logo.resize((max(1, round(logo.size[0] * k)), max(1, round(logo.size[1] * k))), Image.LANCZOS)
    pos = ((n - logo.size[0]) // 2, (n - logo.size[1]) // 2 + int(n * 0.02))
    sh = logo.split()[3].filter(ImageFilter.GaussianBlur(SS * 1.2))  # soft drop shadow, as the glyph icons have
    shadow = Image.new('RGBA', big.size, (0, 0, 0, 0))
    shadow.paste(Image.new('RGBA', logo.size, (60, 20, 0, 90)), (pos[0], pos[1] + int(SS * 1.2)), sh)
    big.alpha_composite(shadow)
    big.alpha_composite(logo, pos)
    return big


def make(name, c1, c2, fn):
    if fn == 'logo':
        big = logo_icon(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'assets', 'calendar_logo.png'), c1, c2)
    elif fn is None:
        big = dragon_icon(c1, c2)
    else:
        big = tile(c1, c2)
        layer = Image.new('RGBA', big.size, (0, 0, 0, 0))
        fn(G(layer), c2)
        sh = layer.split()[3].filter(ImageFilter.GaussianBlur(SS * 1.2))  # soft drop shadow
        shadow = Image.new('RGBA', big.size, (0, 0, 0, 0))
        shadow.paste(Image.new('RGBA', big.size, (0, 0, 40, 90)), (0, int(SS * 1.2)), sh)
        big.alpha_composite(shadow)
        big.alpha_composite(layer)
    return big.resize((SZ, SZ), Image.LANCZOS)


def main():
    icons = [(n, make(n, a, b, f)) for n, a, b, f in ICONS]
    if len(sys.argv) > 1:
        sheet = Image.new('RGBA', (len(icons) * (SZ + 12) + 12, SZ + 24), (21, 27, 51, 255))
        for i, (n, im) in enumerate(icons):
            sheet.alpha_composite(im, (12 + i * (SZ + 12), 12))
        sheet.resize((sheet.size[0] * 2, sheet.size[1] * 2), Image.NEAREST).save(sys.argv[1])
    total = 0
    with open(OUT_C, 'w', encoding='utf-8', newline='\n') as f:
        f.write('// Generated by tools/gen_icons.py - do not edit.\n#include "icons_img.h"\n\n')
        for name, im in icons:
            w, h, data = gen_dragons.to_lvgl(im)
            total += len(data)
            f.write(f'static const uint8_t {name}_map[] = {{\n')
            for i in range(0, len(data), 24):
                f.write('  ' + ','.join(f'0x{x:02x}' for x in data[i:i + 24]) + ',\n')
            f.write('};\n')
            f.write(f'const lv_img_dsc_t {name} = {{\n  .header = {{.cf = LV_IMG_CF_TRUE_COLOR_ALPHA, .always_zero = 0, .reserved = 0, .w = {w}, .h = {h}}},\n'
                    f'  .data_size = {len(data)},\n  .data = {name}_map,\n}};\n\n')
    with open(OUT_H, 'w', encoding='utf-8', newline='\n') as f:
        f.write('#pragma once\n// Generated by tools/gen_icons.py - do not edit.\n#include <lvgl.h>\n\n#ifdef __cplusplus\nextern "C" {\n#endif\n')
        for name, _ in icons:
            f.write(f'extern const lv_img_dsc_t {name};\n')
        f.write('#ifdef __cplusplus\n}\n#endif\n')
    print(f'{len(icons)} icons, {total // 1024} KB')


if __name__ == '__main__':
    main()
