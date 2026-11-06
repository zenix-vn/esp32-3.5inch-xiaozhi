#!/usr/bin/env python3
"""Behavioural tests for kindledesk_native.lua on top of the stub KOReader widgets.

    python tests/logic_test.py path/to/data.json
"""
import sys
from pathlib import Path

from lupa import LuaRuntime
from PIL import ImageFont

ROOT = Path(__file__).resolve().parent.parent
PLUGIN = ROOT / "koplugin" / "desk.koplugin"
_f = {}


def measure(text, name, size):
    key = (name, int(size))
    if key not in _f:
        _f[key] = ImageFont.truetype(str(ROOT / "fonts" / name), max(6, int(size)))
    return int(_f[key].getlength(str(text)))


lua = LuaRuntime(unpack_returned_tuples=True)
lua.globals().MEASURE = measure
lua.execute((ROOT / "tests" / "koreader_stubs.lua").read_text(encoding="utf-8"))
lua.execute('package.path = "%s/?.lua;" .. package.path' % str(PLUGIN).replace("\\", "/"))
lua.execute('Native = require("kindledesk_native")')
lua.globals().DATA_TEXT = Path(sys.argv[1]).read_text(encoding="utf-8")

lua.execute("""
    function fresh(w, h)
        SCREEN.w, SCREEN.h, SCREEN.mode = w or 1264, h or 1680, 0
        SETTINGS_STUB.data = {}
        SWITCHED = nil
        local n = Native:new{ server = "https://x", token = "t", settings = SETTINGS_STUB, switch_cb = function() SWITCHED = true end }
        n.data = require("json").decode(DATA_TEXT)
        n.data_at = os.time()
        n:rebuild(true)
        n[1]:paintTo(nil, 0, 0)          -- paint once so tappable frames know where they are
        CUR = n
        return n
    end
    function tap(n, action)               -- tap the centre of the widget registered for `action`, as the glass sees it
        for _, h in ipairs(n.hits) do
            if h.action == action then
                local d = h.frame.dimen
                local px, py = n:toPhysical(math.floor(d.x + d.w / 2), math.floor(d.y + d.h / 2))
                n:onTap(nil, { pos = { x = px, y = py } })
                return true
            end
        end
        return false
    end
""")

failures = 0


def check(name, cond):
    global failures
    print(("PASS " if cond else "FAIL ") + name)
    failures += 0 if cond else 1


def ev(code):
    """Evaluate an expression, or run a statement."""
    try:
        return lua.eval(code)
    except Exception as e:
        if "error loading code" not in str(e):
            raise
        return lua.execute(code)


# --- opening the desk shows the first page with every control wired up
ev("fresh()")
ev("""function has(n, action) for _, h in ipairs(n.hits) do if h.action == action then return true end end return false end""")
check("desk page registers tab, card and header controls",
      all(ev('has(CUR, "%s")' % a) for a in ("mode:en2vi", "mode:vi2en", "mode:mixed", "flip", "menu", "page", "refresh")))

# --- leaving to KOReader (the "Đọc sách" button)
ev("fresh(); CLOSED = false")
check("header has a read-a-book button", ev('has(CUR, "@exit")'))
check("the button exits the desk", ev('tap(CUR, "@exit")') and ev("CLOSED") is True)
ev("fresh(); CLOSED = false; CUR.menu_open = true; CUR:rebuild(true)")
check("menu still has its own exit", ev('tap(CUR, "@exit")') and ev("CLOSED") is True)

# --- flashcards
ev("fresh()")
check("card starts unflipped", ev("CUR:cardState(os.time()).flipped") is False)
ev('tap(CUR, "flip")')
check("tap on card flips it", ev("CUR:cardState(os.time()).flipped") is True)
before = ev("CUR:cardState(os.time()).idx")
ev("CUR:step(1)")
check("next button moves to the next card", ev("CUR:cardState(os.time()).idx") == before % ev("#CUR:cardList()") + 1)
check("next card starts unflipped", ev("CUR:cardState(os.time()).flipped") is False)
ev("CUR:step(-1)")
check("previous button goes back", ev("CUR:cardState(os.time()).idx") == before)
ev('tap(CUR, "mode:vi2en")')
check("dock tab changes direction", ev("CUR:cardState(os.time()).dir") == "vi2en")
check("flip timer is positive and bounded", 1 <= ev("CUR:cardNext(os.time())") <= 200)
ev("CUR.data.shuffle = true; CUR.perm = nil")
check("shuffle yields a permutation", ev("""(function()
    local p, seen, ok = CUR:permutation(#CUR:cardList()), {}, true
    for _, v in ipairs(p) do if seen[v] then ok = false end seen[v] = true end
    return ok and #p == #CUR:cardList() end)()"""))

# --- pages
ev("fresh()")
n_pages = ev("#CUR.data.pages")
ev("CUR:turnPage(1)")
check("turnPage advances", ev("CUR.page") == 2)
ev("CUR:turnPage(-1)")
check("turnPage wraps back", ev("CUR.page") == 1)
ev("CUR.page = %d; CUR:turnPage(1)" % n_pages)
check("turnPage wraps past the end", ev("CUR.page") == 1)
ev("CUR.page = 2; CUR:rebuild()")
ev("CUR:step(1)")
check("page-turn key on a non-card page turns the page", ev("CUR.page") == 3)

# --- pomodoro
ev("fresh(); CUR.page = 3; CUR:rebuild(); CUR[1]:paintTo(nil, 0, 0)")
check("focus page is the timer page", ev("CUR.page_name") == "focus")
ev('tap(CUR, "pomo")')
check("tap starts focus", ev("CUR:pomoView(os.time()).state") == "focus")
check("remaining minutes match the setting", ev("CUR:pomoView(os.time()).remain_min") == ev("CUR.data.pomo_focus"))
ev("CUR.pomo.stop = os.time() - 1")
check("timer reports done after it ends", ev("CUR:pomoView(os.time()).state") == "done_focus")
ev('tap(CUR, "pomo")')
check("tap after focus starts the break", ev("CUR:pomoView(os.time()).state") == "break")

# --- menu and rotation
ev("fresh()")
ev('tap(CUR, "menu")')
check("weekday opens the menu", ev("CUR.menu_open") is True)
ev('tap(CUR, "@rot:landscape_cw")')
check("rotation is saved", ev('SETTINGS_STUB.data.rotation') == "landscape_cw")
check("page is now laid out landscape on a portrait glass", ev("CUR.rot") == 90 and ev("CUR.W") == 1680 and ev("CUR.PW") == 1264)
check("KOReader's own rotation is left alone", ev("SCREEN.mode") == 0)
ev('tap(CUR, "@ref:5")')
check("refresh interval is saved", ev("SETTINGS_STUB.data.refresh_min") == 5)
ev('tap(CUR, "@night")')
check("night mode toggles", ev("SETTINGS_STUB.data.night") is True)
ev('tap(CUR, "@nightpreset")')
check("night preset cycles (23h-6h -> 0h-6h)", ev("SETTINGS_STUB.data.night_from") == 0 and ev("SETTINGS_STUB.data.night_to") == 6)
ev('tap(CUR, "@close")')
check("close returns to the page", ev("CUR.menu_open") is False)

# --- landscape geometry
for rotation, deg in (("landscape_cw", 90), ("portrait_inv", 180), ("landscape_ccw", 270), ("portrait", 0)):
    ev("fresh(); SETTINGS_STUB.data.rotation = '%s'; CUR:rebuild(true)" % rotation)
    check("%s: layout size and orientation" % rotation, ev("CUR.rot") == deg and ev("CUR.land") == (deg in (90, 270)))
    check("%s: glass <-> page mapping round-trips" % rotation, ev("""(function()
        for _, p in ipairs({ { 10, 20 }, { 300, 700 }, { CUR.W - 5, CUR.H - 9 } }) do
            local px, py = CUR:toPhysical(p[1], p[2])
            local lx, ly = CUR:toLogical(px, py)
            if lx ~= p[1] or ly ~= p[2] then return false end
            if px < 0 or py < 0 or px >= CUR.PW or py >= CUR.PH then return false end
        end
        return true end)()"""))
    check("%s: tapping the card on the glass flips it" % rotation, ev('tap(CUR, "flip")') and ev("CUR:cardState(os.time()).flipped") is True)
    check("%s: tapping a dock tab on the glass changes direction" % rotation, ev('tap(CUR, "mode:vi2en")') and ev("CUR:cardState(os.time()).dir") == "vi2en")
# --- auto rotation follows the gsensor (device turned clockwise -> page turned counter-clockwise)
ev("fresh()")
check("auto is the default and starts upright", ev("CUR.rot") == 0 and ev("CUR:cfg('rotation', 'auto')") == "auto")
check("gsensor event is consumed", ev("CUR:onSetRotationMode(1)") is True)
check("device turned clockwise -> page rotated 270", ev("CUR.rot") == 270 and ev("CUR.land") is True)
ev("CUR:onSetRotationMode(3)")
check("device turned counter-clockwise -> page rotated 90", ev("CUR.rot") == 90)
ev("CUR:onSetRotationMode(2)")
check("upside down -> 180", ev("CUR.rot") == 180)
ev("CUR:onSetRotationMode(0)")
check("upright again -> 0", ev("CUR.rot") == 0)
ev("fresh(); SETTINGS_STUB.data.rotation = 'landscape_cw'; CUR:rebuild(true)")
check("gsensor event is consumed but ignored when rotation is fixed", ev("CUR:onSetRotationMode(0)") is True and ev("CUR.rot") == 90)

ev("CUR.rot = 90")
check("rotatedCopy(90) direction was calibrated", ev("CUR:copyDegrees()") in (90, 270))

# --- dates
ev("fresh()")
check("daysUntil today is 0", ev("""(function() local t = CUR:clock()
    return CUR:daysUntil(string.format('%04d-%02d-%02d', t.year, t.month, t.day)) end)()""") == 0)
check("daysUntil tomorrow is 1", ev("""(function() local t = CUR:clock()
    return CUR:daysUntil(string.format('%04d-%02d-%02d', t.year, t.month, t.day + 1)) end)()""") == 1)
check("events sorted soonest first", ev("""(function() local e = CUR:events()
    for i = 2, #e do if e[i].days >= 0 and e[i - 1].days >= 0 and e[i].days < e[i - 1].days then return false end end
    return true end)()"""))

# --- no data yet (first run, offline): still draws something
ev("fresh(); CUR.data = nil; CUR.page = 1")
check("builds without data", ev("CUR:rebuild(true)") is True)

# --- repeated build errors fall back to the server images
ev("fresh(); SETTINGS_STUB.data.ui = nil")
ev("CUR.buildPage = function() error('boom') end")
ev("CUR:rebuild(); CUR:rebuild()")
check("two build errors switch to image mode", ev("SETTINGS_STUB.data.ui") == "image" and ev("SWITCHED") is True)

print("\n%d failure(s)" % failures)
sys.exit(1 if failures else 0)
