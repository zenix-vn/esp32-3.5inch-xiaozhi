"""Turns the animal sprite sheets in assets/ into the LVGL images that walk on top of the home screen clock.

    python3 tools/gen_pets.py [preview.png]

Each sheet (assets/<name>.png, transparent background) holds one row of four frames, side view facing right: walking,
walking (the other step), lying down, asleep. Frames are found by the empty columns between them (an animal may spill
over its 512 px cell), scaled together so the walking ones are PET_H tall, and stood on one baseline in a PET_W x PET_H
box. Images are LV_IMG_CF_INDEXED_8BIT (255 colours + transparent, alpha in the palette). Only the right-facing frames
are stored: the firmware mirrors them in RAM when the animal walks left (home.h).
Output: firmware/src/pets_img.c + pets_img.h (PET_N, PET_W, PET_H, PET_FRAMES[pet][4], PET_NAMES[pet][2]).
"""
import os
import sys

import numpy as np
from PIL import Image, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'assets')
OUT_C = os.path.join(ROOT, 'firmware', 'src', 'pets_img.c')
OUT_H = os.path.join(ROOT, 'firmware', 'src', 'pets_img.h')
PET_W, PET_H = 48, 40

PETS = [  # file, Vietnamese name, English name
    ('meo', 'Mèo cam', 'Ginger cat'),
    ('cho', 'Cún con', 'Puppy'),
    ('voi', 'Voi con', 'Baby elephant'),
    ('cu-meo', 'Cú mèo', 'Owl'),
    ('canh-cut', 'Chim cánh cụt', 'Penguin'),
]


def frames_of(sheet):
    """The four animals of a sheet. Animals may touch or spill over their 512 px cell, so the blobs of ink are found
    (at quarter scale) and each goes to the cell that holds its centre."""
    q = 4
    small = np.asarray(sheet.resize((sheet.size[0] // q, sheet.size[1] // q), Image.BOX))[:, :, 3] > 10
    h, w = small.shape
    lab = np.zeros((h, w), dtype=np.int32)
    n = 0
    for y0 in range(h):
        for x0 in range(w):
            if not small[y0, x0] or lab[y0, x0]:
                continue
            n += 1
            stack = [(y0, x0)]
            lab[y0, x0] = n
            while stack:
                y, x = stack.pop()
                for yy, xx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
                    if 0 <= yy < h and 0 <= xx < w and small[yy, xx] and not lab[yy, xx]:
                        lab[yy, xx] = n
                        stack.append((yy, xx))
    cell = sheet.size[0] // 4 // q
    owner = np.zeros(n + 1, dtype=np.int32)
    for k in range(1, n + 1):
        xs = np.nonzero(lab == k)[1]
        owner[k] = min(3, int(xs.mean()) // cell)
    out = []
    for c in range(4):
        m = np.isin(lab, np.nonzero(owner == c)[0]) & (lab > 0)
        mask = Image.fromarray((m * 255).astype(np.uint8)).resize(sheet.size, Image.NEAREST)
        mask = mask.filter(ImageFilter.MaxFilter(9))  # a margin for the soft edges lost at quarter scale
        part = sheet.copy()
        part.putalpha(Image.fromarray(np.minimum(np.asarray(sheet)[:, :, 3], np.asarray(mask))))
        out.append(part.crop(part.getbbox()))
    return out


def fit(frames):
    """One scale for all four (walking ones PET_H tall, all within PET_W), bottom-aligned and centred."""
    k = min(PET_H / max(f.size[1] for f in frames[:2]), PET_W / max(f.size[0] for f in frames))
    out = []
    for f in frames:
        w, h = max(1, round(f.size[0] * k)), max(1, round(f.size[1] * k))
        small = f.resize((w, h), Image.LANCZOS)
        box = Image.new('RGBA', (PET_W, PET_H), (0, 0, 0, 0))
        box.alpha_composite(small, ((PET_W - w) // 2, PET_H - h))
        out.append(box)
    return out


def quantize(img, n=255):
    """RGBA -> (256-entry RGBA palette, index per pixel), index 0 transparent; k-means on premultiplied colours."""
    a = np.asarray(img, dtype=np.float32) / 255
    flat = a.reshape(-1, 4)
    vis = flat[:, 3] > 0.04
    pts = flat[vis].copy()
    pts[:, :3] *= pts[:, 3:4]
    uniq = np.unique(np.round(pts * 255), axis=0) / 255
    n = min(n, len(uniq))
    rng = np.random.default_rng(1)
    cent = uniq[rng.choice(len(uniq), n, replace=False)]
    for _ in range(12):
        lab = ((pts[:, None, :] - cent[None, :, :]) ** 2).sum(-1).argmin(1)
        for k in range(n):
            m = pts[lab == k]
            if len(m):
                cent[k] = m.mean(0)
    pal = [(0, 0, 0, 0)]
    for c in cent:
        al = max(c[3], 1e-6)
        pal.append(tuple(int(round(min(1, v / al) * 255)) for v in c[:3]) + (int(round(c[3] * 255)),))
    pal += [(0, 0, 0, 0)] * (256 - len(pal))
    idx = np.zeros(len(flat), dtype=np.uint8)
    idx[vis] = lab + 1
    return pal, idx.reshape(PET_H, PET_W)


def to_lvgl_i8(pal, idx):
    data = bytearray()
    for r, g, b, a in pal:  # lv_color32_t: blue, green, red, alpha
        data += bytes((b, g, r, a))
    data += idx.astype(np.uint8).tobytes()
    return data


def render(pal, idx):
    return Image.fromarray(np.array([[pal[v] for v in row] for row in idx], dtype=np.uint8))


def main():
    pets = []
    for file, vi, en in PETS:
        frames = fit(frames_of(Image.open(os.path.join(SRC, file + '.png')).convert('RGBA')))
        pets.append([quantize(f) for f in frames])

    if len(sys.argv) > 1:  # preview: every frame 4x, right and mirrored, on the dark home background
        z = 4
        sheet = Image.new('RGBA', ((PET_W + 4) * 8 * z, (PET_H + 4) * len(pets) * z), (21, 27, 51, 255))
        for pi, fr in enumerate(pets):
            for fi, (pal, idx) in enumerate(fr + [(p, i[:, ::-1]) for p, i in fr]):
                im = render(pal, idx).resize((PET_W * z, PET_H * z), Image.NEAREST)
                sheet.alpha_composite(im, (fi * (PET_W + 4) * z, pi * (PET_H + 4) * z))
        sheet.save(sys.argv[1])

    total = 0
    with open(OUT_C, 'w', encoding='utf-8', newline='\n') as f:
        f.write('// Generated by tools/gen_pets.py from assets/*.png - do not edit.\n#include "pets_img.h"\n\n')
        for pi, fr in enumerate(pets):
            for fi, (pal, idx) in enumerate(fr):
                data = to_lvgl_i8(pal, idx)
                total += len(data)
                name = f'pet{pi}_{fi}'
                f.write(f'static const uint8_t {name}_map[] = {{\n')
                for i in range(0, len(data), 24):
                    f.write('  ' + ','.join(f'0x{x:02x}' for x in data[i:i + 24]) + ',\n')
                f.write(f'}};\nstatic const lv_img_dsc_t {name} = {{\n  .header = {{.cf = LV_IMG_CF_INDEXED_8BIT, .always_zero = 0, .reserved = 0, '
                        f'.w = {PET_W}, .h = {PET_H}}},\n  .data_size = {len(data)},\n  .data = {name}_map,\n}};\n')
        f.write('\nconst lv_img_dsc_t *const PET_FRAMES[PET_N][4] = {\n')
        for pi in range(len(pets)):
            f.write('  {' + ', '.join(f'&pet{pi}_{fi}' for fi in range(4)) + '},\n')
        f.write('};\n\nconst char *const PET_NAMES[PET_N][2] = {\n')
        for _, vi, en in PETS:
            f.write(f'  {{"{vi}", "{en}"}},\n')
        f.write('};\n')
    with open(OUT_H, 'w', encoding='utf-8', newline='\n') as f:
        f.write('#pragma once\n// Generated by tools/gen_pets.py - do not edit.\n#include <lvgl.h>\n\n#ifdef __cplusplus\nextern "C" {\n#endif\n')
        f.write(f'#define PET_N {len(pets)}\n#define PET_W {PET_W}\n#define PET_H {PET_H}\n')
        f.write('// [pet][frame], facing right: 0 walk, 1 walk (other step), 2 lying down, 3 asleep. 8-bit indexed.\n')
        f.write('extern const lv_img_dsc_t *const PET_FRAMES[PET_N][4];\nextern const char *const PET_NAMES[PET_N][2];  // Vietnamese, English\n')
        f.write('#ifdef __cplusplus\n}\n#endif\n')
    print(f'{len(pets)} pets, {total // 1024} KB')


if __name__ == '__main__':
    main()
