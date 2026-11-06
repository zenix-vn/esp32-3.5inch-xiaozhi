#!/usr/bin/env python3
"""Kindle Desk server: renders the desk pages (clock, weather, flashcards, calendar, focus timer, quotes)
and the sleep screen as e-ink PNGs for a Kindle running the desk.koplugin.

    pip install -r requirements.txt && python -m playwright install chromium
    python server.py

Every endpoint except /healthz needs ?k=<DESK_TOKEN> when DESK_TOKEN is set.
    /                       HTML preview            ?p=<page index>&w=&h=
    /screen.png             page as PNG              ?p=&w=&h=      (+ X-Desk-Zones header: tap zones)
    /sleep.png              sleep screen PNG         ?w=&h=&bat=
    /next /prev /flip       flashcard controls
    /mode?m=en2vi|vi2en|mixed
    /pomo?a=toggle          focus timer
    /quote?d=1              next quote
    /settings               web settings page
"""
import base64
import io
import json
import math
import os
import random
import time
import urllib.parse
import urllib.request
from datetime import date, datetime, timedelta, timezone
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path

ROOT = Path(__file__).parent
DATA = Path(os.environ.get("DESK_DATA", ROOT / "data"))
DATA.mkdir(parents=True, exist_ok=True)

TOKEN = os.environ.get("DESK_TOKEN", "")
BASE = json.loads((ROOT / "config.json").read_text(encoding="utf-8"))
PORT = int(os.environ.get("PORT", BASE["port"]))
PAGE = (ROOT / "desk.html").read_text(encoding="utf-8")
SETTINGS_HTML = (ROOT / "settings.html").read_text(encoding="utf-8")

MODES = ("en2vi", "vi2en", "mixed")
ALL_PAGES = ("desk", "calendar", "focus", "vocab", "quote")
WEEKDAYS = ["Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy", "Chủ Nhật"]

DEFAULTS = {
    "city": BASE["city"], "latitude": BASE["latitude"], "longitude": BASE["longitude"],
    "timezone": BASE["timezone"], "utc_offset_hours": BASE["utc_offset_hours"],
    "units": "C",              # C or F
    "hour12": False,
    "theme": "light",          # light or dark (inverted)
    "show_lunar": True,
    "card_seconds": BASE["card_seconds"], "flip_after_seconds": BASE["flip_after_seconds"],
    "mode": BASE["default_mode"], "shuffle": False,
    "pages": list(ALL_PAGES),
    "events": [{"name": "Tết Dương lịch", "date": "2027-01-01"}, {"name": "Tết Nguyên Đán", "date": "2027-02-06"}],
    "pomo_focus": 25, "pomo_break": 5,
    "quotes": [],              # extra quotes: [{"text": "...", "by": "..."}]
}

QUOTES = [
    {"text": "Học, học nữa, học mãi.", "by": "V. I. Lenin"},
    {"text": "Không có gì quý hơn độc lập, tự do.", "by": "Hồ Chí Minh"},
    {"text": "The secret of getting ahead is getting started.", "by": "Mark Twain"},
    {"text": "It always seems impossible until it is done.", "by": "Nelson Mandela"},
    {"text": "Một hành trình ngàn dặm bắt đầu từ một bước chân.", "by": "Lão Tử"},
    {"text": "What we learn with pleasure we never forget.", "by": "Alfred Mercier"},
    {"text": "Thất bại là mẹ thành công.", "by": "Tục ngữ"},
    {"text": "Well done is better than well said.", "by": "Benjamin Franklin"},
    {"text": "Có công mài sắt, có ngày nên kim.", "by": "Tục ngữ"},
    {"text": "The best way to predict the future is to create it.", "by": "Peter Drucker"},
    {"text": "Đi một ngày đàng, học một sàng khôn.", "by": "Tục ngữ"},
    {"text": "Stay hungry, stay foolish.", "by": "Steve Jobs"},
]

# ------------------------------------------------------------ settings ----

_lock_state = {"settings": None, "cards": None}


def load_settings():
    s = dict(DEFAULTS)
    f = DATA / "settings.json"
    if f.exists():
        try:
            s.update(json.loads(f.read_text(encoding="utf-8")))
        except Exception as e:
            print("settings.json unreadable:", e)
    return s


def load_cards():
    for f in (DATA / "cards.json", ROOT / "cards.json"):
        if f.exists():
            try:
                return json.loads(f.read_text(encoding="utf-8"))
            except Exception as e:
                print(f, "unreadable:", e)
    return []


def sanitize(s):
    """Coerce a settings dict coming from the web form into safe values."""
    out = dict(DEFAULTS)
    for k, default in DEFAULTS.items():
        v = s.get(k, default)
        if isinstance(default, bool):
            v = bool(v)
        elif isinstance(default, int) and not isinstance(default, bool):
            v = int(v)
        elif isinstance(default, float):
            v = float(v)
        elif isinstance(default, str):
            v = str(v)
        out[k] = v
    out["units"] = out["units"] if out["units"] in ("C", "F") else "C"
    out["theme"] = out["theme"] if out["theme"] in ("light", "dark") else "light"
    out["mode"] = out["mode"] if out["mode"] in MODES else "mixed"
    out["card_seconds"] = max(20, min(3600, out["card_seconds"]))
    out["flip_after_seconds"] = max(5, min(out["card_seconds"] - 5, out["flip_after_seconds"]))
    out["pomo_focus"] = max(1, min(180, out["pomo_focus"]))
    out["pomo_break"] = max(1, min(60, out["pomo_break"]))
    pages = [p for p in s.get("pages", out["pages"]) if p in ALL_PAGES]
    out["pages"] = pages or ["desk"]
    out["events"] = [{"name": str(e.get("name", ""))[:40], "date": str(e.get("date", ""))[:10]}
                     for e in s.get("events", []) if isinstance(e, dict) and e.get("date")][:12]
    out["quotes"] = [{"text": str(q.get("text", ""))[:200], "by": str(q.get("by", ""))[:40]}
                     for q in s.get("quotes", []) if isinstance(q, dict) and q.get("text")][:50]
    return out


SETTINGS = load_settings()
CARDS = load_cards()


def save_settings(new_settings, new_cards):
    global SETTINGS, CARDS
    SETTINGS = sanitize(new_settings)
    (DATA / "settings.json").write_text(json.dumps(SETTINGS, ensure_ascii=False, indent=2), encoding="utf-8")
    cards = [{k: str(c.get(k, ""))[:120] for k in ("en", "ipa", "vi", "ex_en", "ex_vi")}
             for c in new_cards if isinstance(c, dict) and c.get("en") and c.get("vi")][:500]
    if cards:
        CARDS = cards
        (DATA / "cards.json").write_text(json.dumps(CARDS, ensure_ascii=False, indent=2), encoding="utf-8")
    _weather["at"] = 0.0      # location/units may have changed
    _png.update(key=None)


def font_css():
    """Embed the bundled Noto fonts so the render is identical on Windows and Linux."""
    faces = [("Desk Sans", 400, "normal", "NotoSans-Regular"), ("Desk Sans", 700, "normal", "NotoSans-Bold"),
             ("Desk Sans", 400, "italic", "NotoSans-Italic"), ("Desk Serif", 700, "normal", "NotoSerif-Bold")]
    css = ""
    for fam, w, st, f in faces:
        b64 = base64.b64encode((ROOT / "fonts" / (f + ".ttf")).read_bytes()).decode()
        css += ("@font-face{font-family:'%s';font-weight:%d;font-style:%s;"
                "src:url(data:font/ttf;base64,%s) format('truetype');}\n" % (fam, w, st, b64))
    return css


PAGE = PAGE.replace("/*FONTS*/", font_css())

# ---------------------------------------------------------------- time ----

def now_local():
    try:
        from zoneinfo import ZoneInfo
        return datetime.now(ZoneInfo(SETTINGS["timezone"]))
    except Exception:
        return datetime.now(timezone(timedelta(hours=SETTINGS["utc_offset_hours"])))


# Vietnamese lunar calendar (Ho Ngoc Duc's algorithm).
def _jd(dd, mm, yy):
    a = (14 - mm) // 12
    y = yy + 4800 - a
    m = mm + 12 * a - 3
    jd = dd + (153 * m + 2) // 5 + 365 * y + y // 4 - y // 100 + y // 400 - 32045
    if jd < 2299161:
        jd = dd + (153 * m + 2) // 5 + 365 * y + y // 4 - 32083
    return jd


def _new_moon(k):
    T = k / 1236.85
    T2, T3, dr = T * T, T * T * T, math.pi / 180
    jd1 = 2415020.75933 + 29.53058868 * k + 0.0001178 * T2 - 0.000000155 * T3
    jd1 += 0.00033 * math.sin((166.56 + 132.87 * T - 0.009173 * T2) * dr)
    M = 359.2242 + 29.10535608 * k - 0.0000333 * T2 - 0.00000347 * T3
    Mpr = 306.0253 + 385.81691806 * k + 0.0107306 * T2 + 0.00001236 * T3
    F = 21.2964 + 390.67050646 * k - 0.0016528 * T2 - 0.00000239 * T3
    c1 = (0.1734 - 0.000393 * T) * math.sin(M * dr) + 0.0021 * math.sin(2 * dr * M)
    c1 += -0.4068 * math.sin(Mpr * dr) + 0.0161 * math.sin(dr * 2 * Mpr) - 0.0004 * math.sin(dr * 3 * Mpr)
    c1 += 0.0104 * math.sin(dr * 2 * F) - 0.0051 * math.sin(dr * (M + Mpr))
    c1 += -0.0074 * math.sin(dr * (M - Mpr)) + 0.0004 * math.sin(dr * (2 * F + M))
    c1 += -0.0004 * math.sin(dr * (2 * F - M)) - 0.0006 * math.sin(dr * (2 * F + Mpr))
    c1 += 0.0010 * math.sin(dr * (2 * F - Mpr)) + 0.0005 * math.sin(dr * (2 * Mpr + M))
    if T < -11:
        dt = 0.001 + 0.000839 * T + 0.0002261 * T2 - 0.00000845 * T3 - 0.000000081 * T * T3
    else:
        dt = -0.000278 + 0.000265 * T + 0.000262 * T2
    return jd1 + c1 - dt


def _nm_day(k, tz):
    return int(_new_moon(k) + 0.5 + tz / 24)


def _sun_long(jdn, tz):
    T = (jdn - 0.5 - tz / 24 - 2451545.0) / 36525
    T2, dr = T * T, math.pi / 180
    M = 357.52910 + 35999.05030 * T - 0.0001559 * T2 - 0.00000048 * T * T2
    L0 = 280.46645 + 36000.76983 * T + 0.0003032 * T2
    dl = (1.914600 - 0.004817 * T - 0.000014 * T2) * math.sin(dr * M)
    dl += (0.019993 - 0.000101 * T) * math.sin(dr * 2 * M) + 0.000290 * math.sin(dr * 3 * M)
    L = (L0 + dl) * dr
    L -= math.pi * 2 * int(L / (math.pi * 2))
    return int(L / math.pi * 6)


def _month11(yy, tz):
    off = _jd(31, 12, yy) - 2415021
    k = int(off / 29.530588853)
    nm = _nm_day(k, tz)
    if _sun_long(nm, tz) >= 9:
        nm = _nm_day(k - 1, tz)
    return nm


def _leap_offset(a11, tz):
    k = int((a11 - 2415021.076998695) / 29.530588853 + 0.5)
    i = 1
    arc = _sun_long(_nm_day(k + i, tz), tz)
    while True:
        last = arc
        i += 1
        arc = _sun_long(_nm_day(k + i, tz), tz)
        if arc == last or i >= 14:
            break
    return i - 1


def lunar(dd, mm, yy, tz=7):
    """Solar date -> (lunar day, month, year, is_leap_month)."""
    day_number = _jd(dd, mm, yy)
    k = int((day_number - 2415021.076998695) / 29.530588853)
    month_start = _nm_day(k + 1, tz)
    if month_start > day_number:
        month_start = _nm_day(k, tz)
    a11 = _month11(yy, tz)
    b11 = a11
    if a11 >= month_start:
        ly = yy
        a11 = _month11(yy - 1, tz)
    else:
        ly = yy + 1
        b11 = _month11(yy + 1, tz)
    ld = day_number - month_start + 1
    diff = (month_start - a11) // 29
    leap = 0
    lm = diff + 11
    if b11 - a11 > 365:
        lo = _leap_offset(a11, tz)
        if diff >= lo:
            lm = diff + 10
            if diff == lo:
                leap = 1
    if lm > 12:
        lm -= 12
    if lm >= 11 and diff < 4:
        ly -= 1
    return ld, lm, ly, leap


CAN = ["Giáp", "Ất", "Bính", "Đinh", "Mậu", "Kỷ", "Canh", "Tân", "Nhâm", "Quý"]
CHI = ["Tý", "Sửu", "Dần", "Mão", "Thìn", "Tỵ", "Ngọ", "Mùi", "Thân", "Dậu", "Tuất", "Hợi"]


def lunar_info(d):
    ld, lm, ly, leap = lunar(d.day, d.month, d.year)
    return {"d": ld, "m": lm, "leap": bool(leap), "year": "%s %s" % (CAN[(ly + 6) % 10], CHI[(ly + 8) % 12]),
            "text": "%d/%d%s âm lịch" % (ld, lm, " nhuận" if leap else "")}


MOON_NAMES = ["Trăng non", "Trăng lưỡi liềm", "Bán nguyệt đầu", "Trăng khuyết đầu",
              "Trăng tròn", "Trăng khuyết cuối", "Bán nguyệt cuối", "Trăng lưỡi liềm cuối"]


def moon_info(d):
    jd = _jd(d.day, d.month, d.year) - 0.5
    age = (jd - 2451550.1) % 29.530588853
    phase = age / 29.530588853
    return {"phase": phase, "age": round(age, 1), "name": MOON_NAMES[int(phase * 8 + 0.5) % 8],
            "illum": round((1 - math.cos(2 * math.pi * phase)) / 2 * 100)}

# ------------------------------------------------------------- weather ----

WMO_LABEL = {
    0: "Trời quang", 1: "Ít mây", 2: "Có mây", 3: "Nhiều mây",
    45: "Sương mù", 48: "Sương mù",
    51: "Mưa phùn", 53: "Mưa phùn", 55: "Mưa phùn",
    61: "Mưa nhỏ", 63: "Mưa vừa", 65: "Mưa to",
    66: "Mưa lạnh", 67: "Mưa lạnh",
    71: "Tuyết nhẹ", 73: "Tuyết", 75: "Tuyết dày", 77: "Tuyết hạt",
    80: "Mưa rào", 81: "Mưa rào", 82: "Mưa rào lớn",
    85: "Tuyết rào", 86: "Tuyết rào",
    95: "Dông", 96: "Dông kèm mưa đá", 99: "Dông kèm mưa đá",
}

_weather = {"at": 0.0, "key": None, "data": None}


def aqi_label(v):
    for limit, label in ((50, "Tốt"), (100, "Vừa"), (150, "Kém"), (200, "Xấu"), (300, "Rất xấu")):
        if v <= limit:
            return label
    return "Nguy hại"


def fetch_weather():
    lat, lon = SETTINGS["latitude"], SETTINGS["longitude"]
    q = urllib.parse.urlencode({
        "latitude": lat, "longitude": lon,
        "current": "temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m",
        "hourly": "temperature_2m,weather_code,precipitation_probability",
        "daily": "weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset,uv_index_max",
        "timezone": "auto", "forecast_days": 5,
    })
    with urllib.request.urlopen("https://api.open-meteo.com/v1/forecast?" + q, timeout=10) as r:
        j = json.load(r)
    aqi = None
    try:
        aq = urllib.parse.urlencode({"latitude": lat, "longitude": lon, "current": "us_aqi", "timezone": "auto"})
        with urllib.request.urlopen("https://air-quality-api.open-meteo.com/v1/air-quality?" + aq, timeout=10) as r:
            aqi = round(json.load(r)["current"]["us_aqi"])
    except Exception as e:
        print("aqi error:", e)
    cur, hr, dy = j["current"], j["hourly"], j["daily"]
    stamp = cur["time"][:13] + ":00"
    start = hr["time"].index(stamp) if stamp in hr["time"] else 0
    hours = []
    for i in range(start + 3, start + 3 + 6 * 3, 3):
        if i < len(hr["time"]):
            hours.append({"h": hr["time"][i][11:13] + "h", "t": hr["temperature_2m"][i],
                          "code": hr["weather_code"][i], "pop": hr["precipitation_probability"][i] or 0})
    days = []
    for i in range(1, len(dy["time"])):
        d = datetime.strptime(dy["time"][i], "%Y-%m-%d")
        days.append({"d": "CN" if d.weekday() == 6 else "T%d" % (d.weekday() + 2),
                     "code": dy["weather_code"][i], "hi": dy["temperature_2m_max"][i], "lo": dy["temperature_2m_min"][i]})
    return {
        "temp": cur["temperature_2m"], "feels": cur["apparent_temperature"],
        "hum": round(cur["relative_humidity_2m"]), "wind": cur["wind_speed_10m"],
        "code": cur["weather_code"], "label": WMO_LABEL.get(cur["weather_code"], "—"),
        "hi": dy["temperature_2m_max"][0], "lo": dy["temperature_2m_min"][0],
        "sunrise": dy["sunrise"][0][11:16], "sunset": dy["sunset"][0][11:16],
        "uv": round(dy["uv_index_max"][0]), "aqi": aqi, "aqi_label": aqi_label(aqi) if aqi is not None else "",
        "hours": hours, "days": days,
    }


def weather_view():
    """Cached weather converted to the configured units."""
    key = (SETTINGS["latitude"], SETTINGS["longitude"])
    if _weather["key"] != key or time.time() - _weather["at"] > 900:
        try:
            _weather["data"], _weather["key"] = fetch_weather(), key
            _weather["at"] = time.time()
        except Exception as e:  # keep showing the last good reading, retry in a minute
            print("weather error:", e)
            _weather["at"] = time.time() - 840
            if _weather["key"] != key:
                _weather["data"] = None
    w = _weather["data"]
    if not w:
        return None
    f = SETTINGS["units"] == "F"
    t = (lambda c: round(c * 9 / 5 + 32)) if f else (lambda c: round(c))
    out = dict(w)
    for k in ("temp", "feels", "hi", "lo"):
        out[k] = t(w[k])
    out["wind"] = round(w["wind"] * (0.621 if f else 1))
    out["wind_unit"] = "mph" if f else "km/h"
    out["hours"] = [dict(h, t=t(h["t"])) for h in w["hours"]]
    out["days"] = [dict(d, hi=t(d["hi"]), lo=t(d["lo"])) for d in w["days"]]
    return out

# ----------------------------------------------------------- flashcards ----

card = {"count": 0, "t0": time.time(), "flip_slot": None, "mode": None}


def slot(now):
    return card["count"] + int((now - card["t0"]) // SETTINGS["card_seconds"])


def card_index(s):
    n = len(CARDS)
    if not SETTINGS["shuffle"] or n < 2:
        return s % n
    day = now_local().date().toordinal()
    order = list(range(n))
    random.Random(day).shuffle(order)
    return order[s % n]


def card_state(now):
    if card["mode"] is None:
        card["mode"] = SETTINGS["mode"]
    s = slot(now)
    phase = (now - card["t0"]) % SETTINGS["card_seconds"]
    flipped = card["flip_slot"] == s or phase >= SETTINGS["flip_after_seconds"]
    idx = card_index(s)
    c = CARDS[idx]
    mode = card["mode"]
    direction = mode if mode != "mixed" else ("en2vi" if s % 2 == 0 else "vi2en")
    if direction == "en2vi":
        front, sub, back, back_sub = c["en"], c["ipa"], c["vi"], c["ex_en"]
    else:
        front, sub, back, back_sub = c["vi"], "", c["en"], (c["ex_vi"] + "  ·  " + c["ipa"]).strip(" ·")
    return {"mode": mode, "direction": direction, "idx": idx + 1, "total": len(CARDS),
            "front": front, "sub": sub, "back": back, "back_sub": back_sub, "flipped": flipped,
            "en": c["en"], "vi": c["vi"], "ipa": c["ipa"], "ex_en": c["ex_en"], "ex_vi": c["ex_vi"]}

def card_next(now):
    """Seconds until the card flips or changes, so the client can refresh exactly then."""
    period, flip_after = SETTINGS["card_seconds"], SETTINGS["flip_after_seconds"]
    phase = (now - card["t0"]) % period
    cands = [period - phase]
    if card["flip_slot"] != slot(now) and phase < flip_after:
        cands.append(flip_after - phase)
    return max(1, int(min(cands)) + 1)


# --------------------------------------------------------------- pomodoro ----

pomo = {"state": "idle", "end": 0.0}


def pomo_view(now):
    st = pomo["state"]
    if st in ("focus", "break") and now >= pomo["end"]:
        st = "done_" + st
    total = {"focus": SETTINGS["pomo_focus"], "break": SETTINGS["pomo_break"]}.get(st.replace("done_", ""), SETTINGS["pomo_focus"])
    remain = max(0, pomo["end"] - now) if st in ("focus", "break") else 0
    frac = 1 - remain / (total * 60) if st in ("focus", "break") else (1 if st.startswith("done") else 0)
    label = {"idle": "Sẵn sàng", "focus": "Đang tập trung", "break": "Nghỉ ngơi",
             "done_focus": "Hết giờ tập trung!", "done_break": "Hết giờ nghỉ!"}[st]
    hint = {"idle": "Chạm để bắt đầu", "focus": "Chạm để huỷ", "break": "Chạm để dừng",
            "done_focus": "Chạm để nghỉ", "done_break": "Chạm để tập trung tiếp"}[st]
    return {"state": st, "remain_min": math.ceil(remain / 60), "total_min": total, "frac": frac,
            "label": label, "hint": hint}


def pomo_toggle(now):
    st = pomo_view(now)["state"]
    if st == "idle" or st == "done_break":
        pomo.update(state="focus", end=now + SETTINGS["pomo_focus"] * 60)
    elif st == "done_focus":
        pomo.update(state="break", end=now + SETTINGS["pomo_break"] * 60)
    else:
        pomo.update(state="idle", end=0.0)

# ------------------------------------------------------------ quote, events ----

quote_state = {"offset": 0}


def quote_view(d):
    quotes = QUOTES + SETTINGS["quotes"]
    q = quotes[(d.toordinal() + quote_state["offset"]) % len(quotes)]
    return q


def events_view(d):
    out = []
    for e in SETTINGS["events"]:
        try:
            ed = date.fromisoformat(e["date"])
        except ValueError:
            continue
        out.append({"name": e["name"], "date": ed.strftime("%d/%m/%Y"), "days": (ed - d).days, "iso": e["date"]})
    out.sort(key=lambda e: (e["days"] < 0, abs(e["days"])))
    return out


def calendar_view(d):
    first = d.replace(day=1)
    nxt = (first + timedelta(days=32)).replace(day=1)
    n = (nxt - first).days
    cells = []
    event_days = {e["date"] for e in SETTINGS["events"]}
    for day in range(1, n + 1):
        cur = first.replace(day=day)
        ld, lm, _, _ = lunar(cur.day, cur.month, cur.year)
        label = ("%d/%d" % (ld, lm)) if ld == 1 else str(ld)
        cells.append({"d": day, "lunar": label, "ev": cur.isoformat() in event_days})
    return {"year": d.year, "month": d.month, "start": first.weekday(), "days": cells, "today": d.day}

# ------------------------------------------------------------- render ----

def state(page_index, w, h, bat=None, sleep=False, menu=None):
    n = now_local()
    pages = SETTINGS["pages"]
    page = "menu" if menu else "sleep" if sleep else pages[page_index % len(pages)]
    hh = n.hour % 12 or 12 if SETTINGS["hour12"] else n.hour
    st = {
        "page": page, "p": page_index % len(pages), "pages": len(pages),
        "w": w, "h": h, "land": w > h, "theme": SETTINGS["theme"],
        "hhmm": "%02d:%02d" % (hh, n.minute) if not SETTINGS["hour12"] else "%d:%02d" % (hh, n.minute),
        "ampm": ("SA" if n.hour < 12 else "CH") if SETTINGS["hour12"] else "",
        "weekday": WEEKDAYS[n.weekday()], "date": "%d tháng %d, %d" % (n.day, n.month, n.year),
        "lunar": lunar_info(n.date()) if SETTINGS["show_lunar"] else None,
        "city": SETTINGS["city"], "unit": SETTINGS["units"],
        "weather": weather_view(), "card": card_state(time.time()),
    }
    if page == "menu":
        st["menu"] = menu
    elif page == "calendar":
        st.update(cal=calendar_view(n.date()), events=events_view(n.date()), moon=moon_info(n.date()))
    elif page == "focus":
        st["pomo"] = pomo_view(time.time())
    elif page == "quote":
        st["quote"] = quote_view(n.date())
    elif page == "sleep":
        tomorrow = (n + timedelta(days=1)).date()
        st.update(sleep={"at": n.strftime("%H:%M"), "bat": bat}, quote=quote_view(n.date()),
                  events=events_view(n.date())[:2], moon=moon_info(n.date()),
                  lunar_tomorrow=lunar_info(tomorrow) if SETTINGS["show_lunar"] else None,
                  tomorrow_label=WEEKDAYS[tomorrow.weekday()])
    return st


def data_view():
    """Everything the Kindle needs to draw the desk itself; it only has to fetch this every few minutes."""
    n = now_local()
    d = n.date()
    return {
        "v": 1, "date": d.isoformat(), "tz": (n.utcoffset() or timedelta(0)).total_seconds() / 3600,
        "city": SETTINGS["city"], "unit": SETTINGS["units"], "hour12": SETTINGS["hour12"],
        "show_lunar": SETTINGS["show_lunar"], "pages": SETTINGS["pages"],
        "weather": weather_view(), "weekdays": WEEKDAYS, "weekdays_upper": [w.upper() for w in WEEKDAYS],
        "cards": CARDS, "mode": SETTINGS["mode"], "shuffle": SETTINGS["shuffle"],
        "card_seconds": SETTINGS["card_seconds"], "flip_after_seconds": SETTINGS["flip_after_seconds"],
        "events": [{"name": e["name"], "date": e["date"]} for e in SETTINGS["events"]],
        "quotes": QUOTES + SETTINGS["quotes"],
        "pomo_focus": SETTINGS["pomo_focus"], "pomo_break": SETTINGS["pomo_break"],
        "lunar": lunar_info(d) if SETTINGS["show_lunar"] else None,
        "cal": calendar_view(d), "moon": moon_info(d),
    }


def html(st, refresh=None):
    page = PAGE.replace("/*STATE*/null", json.dumps(st, ensure_ascii=False))
    if refresh:
        page = page.replace("<head>", '<head><meta http-equiv="refresh" content="%d">' % refresh, 1)
    return page


_browser = {}
_png = {"at": 0.0, "key": None, "data": b"", "zones": "[]"}
_DRAW = """async s => {
    window.draw(s);
    await Promise.all([document.fonts.load("700 1em 'Desk Serif'"), document.fonts.load("400 1em 'Desk Sans'"),
                       document.fonts.load("700 1em 'Desk Sans'"), document.fonts.load("italic 400 1em 'Desk Sans'")]);
    await document.fonts.ready;
    return window.__ZONES;
}"""


def _reset_browser():
    try:
        _browser["browser"].close()
        _browser["pw"].stop()
    except Exception:
        pass
    _browser.clear()


def _page(w, h):
    """One long-lived page: fonts are decoded once and each render only swaps the DOM."""
    if "pg" not in _browser:
        from playwright.sync_api import sync_playwright
        _browser["pw"] = sync_playwright().start()
        _browser["browser"] = _browser["pw"].chromium.launch(args=["--no-sandbox"])  # root in Docker
        _browser["pg"] = _browser["browser"].new_page(viewport={"width": w, "height": h})
        _browser["pg"].set_content(PAGE)  # state is null here, draw() fills it in
        _browser["size"] = (w, h)
    if _browser["size"] != (w, h):
        _browser["pg"].set_viewport_size({"width": w, "height": h})
        _browser["size"] = (w, h)
    return _browser["pg"]


def rotate_zones(zones, rot):
    """Zones are fractions of the logical page; map them onto the image rotated clockwise by rot degrees."""
    out = []
    for z in zones:
        x, y, w, h = z["x"], z["y"], z["w"], z["h"]
        if rot == 90:
            x, y, w, h = 1 - (y + h), x, h, w
        elif rot == 270:
            x, y, w, h = y, 1 - (x + w), h, w
        elif rot == 180:
            x, y = 1 - (x + w), 1 - (y + h)
        out.append({"a": z["a"], "x": round(x, 4), "y": round(y, 4), "w": round(w, 4), "h": round(h, 4)})
    return out


def render_png(st, rot=0):
    key = json.dumps([st, rot], sort_keys=True)
    if key == _png["key"] and time.time() - _png["at"] < 600:
        return _png["data"], _png["zones"]
    t0 = time.time()
    for attempt in (1, 2):
        try:
            pg = _page(st["w"], st["h"])
            zones = pg.evaluate(_DRAW, st)
            t1 = time.time()
            raw = pg.screenshot(type="png")
            t2 = time.time()
            break
        except Exception:
            if attempt == 2:
                raise
            _reset_browser()  # the page died; start over once
    try:
        from PIL import Image, ImageOps
        im = Image.open(io.BytesIO(raw)).convert("L")
        if st["theme"] == "dark":
            im = ImageOps.invert(im)
        if rot:
            im = im.transpose({90: Image.ROTATE_270, 180: Image.ROTATE_180, 270: Image.ROTATE_90}[rot])
        buf = io.BytesIO()
        im.save(buf, "PNG", compress_level=6)
        raw = buf.getvalue()
    except ImportError:
        pass
    zones = json.dumps(rotate_zones(zones, rot), separators=(",", ":"))
    print("render %s: draw %.0fms, shot %.0fms, encode %.0fms" % (st["page"], (t1 - t0) * 1000, (t2 - t1) * 1000, (time.time() - t2) * 1000), flush=True)
    _png.update(at=time.time(), key=key, data=raw, zones=zones)
    return raw, zones


def card_action(path, query):
    now = time.time()
    s = slot(now)
    if path in ("/next", "/prev"):
        card["count"] = s + (1 if path == "/next" else -1)
        card["t0"] = now
        card["flip_slot"] = None
    elif path == "/flip":
        card["flip_slot"] = s
    elif path == "/mode":
        m = query.get("m", [""])[0]
        if m in MODES:
            card["mode"] = m
    elif path == "/pomo":
        pomo_toggle(now)
    elif path == "/quote":
        quote_state["offset"] += int(query.get("d", ["1"])[0] or 1)


def dims(q):
    """Returns (logical w, logical h, page, rot). w/h in the query are the physical screen size."""
    def num(name, default, lo, hi):
        try:
            return max(lo, min(hi, int(q.get(name, [default])[0])))
        except ValueError:
            return default
    w, h, p = num("w", BASE["width"], 480, 2400), num("h", BASE["height"], 480, 2400), num("p", 0, 0, 99)
    rot = num("rot", 0, 0, 359)
    rot = rot if rot in (90, 180, 270) else 0
    return (h, w, p, rot) if rot in (90, 270) else (w, h, p, rot)


def menu_state(q):
    if q.get("menu", ["0"])[0] != "1":
        return None

    def g(k, d):
        return q.get(k, [d])[0]
    return {"rot": g("m_rot", "portrait"), "ref": int(g("m_ref", "1") or 1), "sleep_screen": g("m_sl", "1") == "1",
            "night": g("m_ng", "0") == "1", "night_from": int(g("m_nf", "23") or 23), "night_to": int(g("m_nt", "6") or 6)}


class Handler(BaseHTTPRequestHandler):
    def _send(self, code, ctype, body, headers=None):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        for k, v in (headers or {}).items():
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(body)

    def _auth(self, q):
        return not TOKEN or q.get("k", [""])[0] == TOKEN

    def do_GET(self):
        u = urllib.parse.urlparse(self.path)
        q = urllib.parse.parse_qs(u.query)
        if u.path == "/healthz":
            return self._send(200, "text/plain", b"ok")
        if not self._auth(q):
            return self._send(403, "text/plain", b"forbidden")
        try:
            w, h, p, rot = dims(q)
            if u.path == "/screen.png":
                spec = q.get("do", [""])[0]  # optional action, applied in the same request
                if spec:
                    path, _, qs = spec.partition("?")
                    card_action("/" + path.lstrip("/"), urllib.parse.parse_qs(qs))
                st = state(p, w, h, menu=menu_state(q))
                raw, zones = render_png(st, rot)
                self._send(200, "image/png", raw, {"X-Desk-Zones": zones, "X-Desk-Page": st["page"],
                                                  "X-Desk-Pages": str(st["pages"]),
                                                  "X-Desk-Next": str(card_next(time.time()))})
            elif u.path == "/sleep.png":
                bat = q.get("bat", [""])[0]
                raw, _ = render_png(state(0, w, h, bat=int(bat) if bat.isdigit() else None, sleep=True), rot)
                self._send(200, "image/png", raw)
            elif u.path in ("/next", "/prev", "/flip", "/mode", "/pomo", "/quote"):
                card_action(u.path, q)
                self._send(200, "text/plain", b"ok")
            elif u.path == "/data.json":
                self._send(200, "application/json; charset=utf-8", json.dumps(data_view(), ensure_ascii=False).encode("utf-8"))
            elif u.path == "/settings":
                self._send(200, "text/html; charset=utf-8", SETTINGS_HTML.encode("utf-8"))
            elif u.path == "/api/settings":
                body = json.dumps({"settings": SETTINGS, "cards": CARDS, "defaults": DEFAULTS, "pages": ALL_PAGES},
                                  ensure_ascii=False)
                self._send(200, "application/json; charset=utf-8", body.encode("utf-8"))
            elif u.path == "/":
                self._send(200, "text/html; charset=utf-8", html(state(p, w, h), refresh=30).encode("utf-8"))
            else:
                self._send(404, "text/plain", b"not found")
        except Exception as e:
            import traceback
            traceback.print_exc()
            self._send(500, "text/plain", repr(e).encode())

    def do_POST(self):
        u = urllib.parse.urlparse(self.path)
        q = urllib.parse.parse_qs(u.query)
        if not self._auth(q):
            return self._send(403, "text/plain", b"forbidden")
        if u.path != "/api/settings":
            return self._send(404, "text/plain", b"not found")
        try:
            n = int(self.headers.get("Content-Length", "0"))
            body = json.loads(self.rfile.read(min(n, 1_000_000)))
            save_settings(body.get("settings", {}), body.get("cards", []))
            self._send(200, "application/json", b'{"ok":true}')
        except Exception as e:
            self._send(400, "application/json", json.dumps({"ok": False, "error": repr(e)}).encode())

    def log_message(self, fmt, *args):
        print("%s %s" % (self.address_string(), (fmt % args).split("k=")[0]))


if __name__ == "__main__":
    # Single-threaded on purpose: Playwright's sync API is bound to one thread.
    try:  # pay the Chromium start-up cost now instead of on the first tap
        render_png(state(0, BASE["width"], BASE["height"]))
    except Exception as e:
        print("warm-up failed:", e)
    print("Kindle Desk on http://0.0.0.0:%d  (preview: /, settings: /settings)" % PORT)
    HTTPServer(("0.0.0.0", PORT), Handler).serve_forever()
