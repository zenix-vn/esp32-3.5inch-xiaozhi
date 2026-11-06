#!/usr/bin/env python3
"""Run kindledesk_native.lua against stub KOReader widgets and render what it would paint.

    python tests/native_preview.py http://127.0.0.1:8080/data.json?k=TOKEN out_dir

Catches nil errors and layout mistakes without a Kindle. Text is measured with the bundled Noto fonts, so
sizes are close to the device; icons are drawn as grey placeholders.
"""
import json
import sys
import urllib.request
from pathlib import Path

from lupa import LuaRuntime
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
PLUGIN = ROOT / "koplugin" / "desk.koplugin"
FONTS = ROOT / "fonts"
_fonts = {}


def font(name, size):
    key = (name, size)
    if key not in _fonts:
        path = FONTS / name
        if not path.exists():
            path = FONTS / "NotoSans-Regular.ttf"
        _fonts[key] = ImageFont.truetype(str(path), max(6, int(size)))
    return _fonts[key]


def measure(text, name, size):
    return int(font(name, size).getlength(str(text)))


def run(data_text, outdir):
    lua = LuaRuntime(unpack_returned_tuples=True)
    lua.globals().MEASURE = measure
    lua.execute((ROOT / "tests" / "koreader_stubs.lua").read_text(encoding="utf-8"))
    lua.execute('package.path = "%s/?.lua;" .. package.path' % str(PLUGIN).replace("\\", "/"))
    lua.execute('Native = require("kindledesk_native")')
    lua.globals().DATA_TEXT = data_text
    outdir.mkdir(parents=True, exist_ok=True)
    failures = 0

    # physical glass is always 1264x1680; rotation is the plugin's own setting
    cases = [("portrait", "portrait", 90 * 0), ("landscape", "landscape_cw", 90), ("landscape_ccw", "landscape_ccw", 270)]
    for orient, setting, cw_deg in cases:
        w, h = (1680, 1264) if cw_deg in (90, 270) else (1264, 1680)
        pages = lua.eval('(function() return #(require("json").decode(DATA_TEXT).pages) end)()')
        for page in list(range(1, pages + 1)) + ["menu"]:
            lua.execute("""
                SCREEN.w, SCREEN.h = 1264, 1680
                SETTINGS_STUB.data = { rotation = "%s" }
                OPS = {}
                local n = Native:new{ server = "https://x", token = "t", settings = SETTINGS_STUB }
                n.data = require("json").decode(DATA_TEXT)
                n.data_at = os.time()
                n.page = %s
                n.menu_open = %s
                CUR = n
            """ % (setting, 1 if page == "menu" else page, "true" if page == "menu" else "false"))
            lua.execute("OPS = {}")
            ok = lua.eval("CUR:rebuild(true)")
            name = "%s_%s" % (orient, page)
            if not ok:
                print("FAIL", name, "(build error; see traceback above)")
                failures += 1
                continue
            ops = [dict(o) for o in lua.eval("OPS").values()]
            img = Image.new("L", (w, h), 255)
            d = ImageDraw.Draw(img)
            for o in ops:
                k = o["k"]
                x, y, ow, oh = o["x"], o["y"], o["w"], o["h"]
                if k == "rect":
                    box = (x, y, x + ow - 1, y + oh - 1)
                    r = min(o.get("radius") or 0, ow // 2, oh // 2)
                    bg = o.get("bg")
                    if r > 0:
                        d.rounded_rectangle(box, radius=r, fill=bg, outline=(o.get("color") or 0) if o["border"] > 0 else None, width=int(o["border"]))
                    else:
                        d.rectangle(box, fill=bg, outline=(o.get("color") or 0) if o["border"] > 0 else None, width=int(o["border"]))
                elif k == "text":
                    f = font(o["font"], o["size"])
                    asc, desc = f.getmetrics()
                    d.text((x, y + (oh - (asc + desc)) // 2), str(o["text"]), font=f, fill=o.get("color") or 0)
                elif k == "line":
                    d.rectangle((x, y, x + ow, y + max(1, oh)), fill=o.get("color") or 0)
                elif k == "image":
                    d.rounded_rectangle((x, y, x + ow, y + oh), radius=ow // 6, outline=170, width=3)
                    d.text((x + 6, y + 6), Path(str(o["file"])).stem[:5], font=font("NotoSans-Regular.ttf", max(10, ow // 6)), fill=150)
            if cw_deg:  # what the glass shows: the logical page rotated clockwise
                img = img.rotate(-cw_deg, expand=True)
            img.save(outdir / (name + ".png"))
            # overflow check: nothing may be painted outside the screen
            off = [o for o in ops if o["k"] in ("text", "rect") and (o["x"] < -2 or o["y"] < -2 or o["x"] + o["w"] > w + 2 or o["y"] + o["h"] > h + 2)]
            print("ok  ", name, "ops=%d" % len(ops), "OFFSCREEN=%d" % len(off) if off else "")
            if off:
                failures += 1
                for o in off[:3]:
                    print("     offscreen:", o["k"], o["x"], o["y"], o["w"], o["h"], o.get("text", ""))
    return failures


if __name__ == "__main__":
    src, out = sys.argv[1], Path(sys.argv[2])
    text = urllib.request.urlopen(src).read().decode("utf-8") if src.startswith("http") else Path(src).read_text(encoding="utf-8")
    sys.exit(1 if run(text, out) else 0)
