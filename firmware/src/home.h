#pragma once
// Home V3: left, a big clock (a little animal pads along its top) with the date, the lunar date, a line that takes
// turns between the next alarm and the nearest countdown, and a dock of three apps (Settings > Home screen);
// right, this month's calendar and the weather with the next hours. Weather effects drift over the background.
// Tap the clock for the Clock app, the month for the Calendar, the weather card for the forecast.
// Included by main.cpp after dragon.h / statusbar.h / ambient.h.
#include "lunar.h"
#include "quotes.h"
#include "almanac.h"
#include "pets_img.h"

static const char DEG[] = {(char)0xC2, (char)0xB0, 0};  // UTF-8 degree sign
static const char *const DAYS_VI[] = {"Chủ Nhật", "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy"};
static const char *const DAYS_EN[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char *const MONTHS_EN[] = {"January", "February", "March",     "April",   "May",      "June",
                                        "July",    "August",   "September", "October", "November", "December"};
static void pageWeather();  // weather_page.h
static void pageCalendar();  // calendar_app.h
static void pageClock();     // clock_app.h
static bool homeAlarmText(char *out, size_t n);  // clock_app.h: "06:30 - in 7 h", false when no alarm is on
static int cdHomeList(int *out);                 // countdown.h: events for the home screen, nearest first
static void cdHomeText(int i, char *out, size_t n);  // countdown.h: "Tết - còn 89 ngày"
static long cdDaysLeft(int i);
static const char *cdName(int i);
static const lv_img_dsc_t *appIcon(int i);       // main.cpp: launcher apps
static const char *appName(int i);
static void appOpen(int i);

static void startAnim(lv_obj_t *o, lv_anim_exec_xcb_t cb, int32_t from, int32_t to, uint32_t t, uint32_t playback,
                      uint32_t repeatDelay, lv_anim_path_cb_t path = lv_anim_path_ease_in_out, uint32_t delay = 0) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, o);
  lv_anim_set_exec_cb(&a, cb);
  lv_anim_set_values(&a, from, to);
  lv_anim_set_time(&a, t);
  lv_anim_set_playback_time(&a, playback);
  lv_anim_set_repeat_delay(&a, repeatDelay);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&a, path);
  lv_anim_set_delay(&a, delay);
  lv_anim_start(&a);
}
static void animY(void *o, int32_t v) { lv_obj_set_y((lv_obj_t *)o, v); }
static void animX(void *o, int32_t v) { lv_obj_set_x((lv_obj_t *)o, v); }
// bg_opa, not style opa: a widget with opa < 100% is rendered through a temporary layer buffer every frame
static void animOpa(void *o, int32_t v) { lv_obj_set_style_bg_opa((lv_obj_t *)o, v, 0); }
static void animH(void *o, int32_t v) { lv_obj_set_height((lv_obj_t *)o, v); }

// ---------- weather icons (drawn with shapes into a 44x44 box) ----------
enum { WK_NONE, WK_CLEAR, WK_MOON, WK_PARTLY, WK_CLOUD, WK_FOG, WK_RAIN, WK_SNOW, WK_STORM };
static void cloudShape(lv_obj_t *p, int dx, int dy, uint32_t col) {
  shape(p, dx + 2, dy + 12, 16, 16, col);
  shape(p, dx + 10, dy + 4, 22, 22, col);
  shape(p, dx + 24, dy + 11, 17, 17, col);
  shape(p, dx + 9, dy + 16, 24, 12, col, 6);
}
static int iconKindFor(int c, bool isDay) {
  if (c <= 1) return isDay ? WK_CLEAR : WK_MOON;
  if (c == 2) return WK_PARTLY;
  if (c == 45 || c == 48) return WK_FOG;
  if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82)) return WK_RAIN;
  if ((c >= 71 && c <= 77) || c == 85 || c == 86) return WK_SNOW;
  if (c >= 95) return WK_STORM;
  return WK_CLOUD;
}
static void drawWxIcon(lv_obj_t *box, int kind) {
  lv_obj_clean(box);
  switch (kind) {
    case WK_CLEAR:
      shape(box, 2, 2, 40, 40, 0xFFC93C, LV_RADIUS_CIRCLE, LV_OPA_30);
      shape(box, 9, 9, 26, 26, 0xFFC93C);
      break;
    case WK_MOON:
      shape(box, 8, 8, 28, 28, 0xDDE3FF);
      shape(box, 17, 4, 24, 24, 0x151B33);
      break;
    case WK_PARTLY:
      shape(box, 0, 0, 24, 24, 0xFFC93C);
      cloudShape(box, 3, 8, 0xDDE3FF);
      break;
    case WK_FOG:
      shape(box, 4, 8, 36, 5, 0x9AA3C7, 3);
      shape(box, 10, 19, 30, 5, 0x9AA3C7, 3);
      shape(box, 4, 30, 36, 5, 0x9AA3C7, 3);
      break;
    case WK_RAIN:
      cloudShape(box, 0, 0, 0x9AA3C7);
      shape(box, 12, 31, 3, 9, 0x5AA9FF, 2);
      shape(box, 21, 33, 3, 9, 0x5AA9FF, 2);
      shape(box, 30, 31, 3, 9, 0x5AA9FF, 2);
      break;
    case WK_SNOW:
      cloudShape(box, 0, 0, 0x9AA3C7);
      shape(box, 11, 33, 5, 5, 0xFFFFFF);
      shape(box, 21, 36, 5, 5, 0xFFFFFF);
      shape(box, 30, 33, 5, 5, 0xFFFFFF);
      break;
    case WK_STORM: {
      cloudShape(box, 0, 0, 0x7B84A8);
      lv_obj_t *z = lv_label_create(box);
      setText(z, LV_SYMBOL_CHARGE);
      lv_obj_set_style_text_color(z, lv_color_hex(0xFFC93C), 0);
      lv_obj_set_pos(z, 16, 26);
      break;
    }
    case WK_CLOUD:
      cloudShape(box, 0, 6, 0xB8C1E8);
      break;
    default:
      cloudShape(box, 0, 6, 0x2A3359);
      break;
  }
}
static lv_obj_t *wxIconBox(lv_obj_t *parent, int kind) {
  lv_obj_t *b = lv_obj_create(parent);
  lv_obj_remove_style_all(b);
  lv_obj_set_size(b, 44, 44);
  lv_obj_clear_flag(b, LV_OBJ_FLAG_CLICKABLE);
  drawWxIcon(b, kind);
  return b;
}
static int tempOut(float c) { return (int)lroundf(cfg.tempF ? c * 9.0f / 5.0f + 32.0f : c); }

// ---------- state ----------
static lv_obj_t *hFx, *hClock, *lblGreet, *lblAmPm, *lblDate, *lblLunar, *lblAlarm, *hDock;
static lv_obj_t *hCal, *wCard, *wIcon, *wTemp, *wDesc, *wRange, *wHr[4], *wHt[4];
static int wIconKey = -1, fxKind = -1;

// ---------- weather effects ----------
enum { FX_NONE, FX_RAIN, FX_SNOW, FX_STARS, FX_SUN, FX_FOG };
static void fxStart(int kind) {
  lv_obj_clean(hFx);
  fxKind = kind;
  switch (kind) {
    case FX_RAIN:
      for (int i = 0; i < 14; i++) {
        lv_obj_t *o = shape(hFx, random(0, 480), -20, 2, 14, 0x9CC7FF, 1, LV_OPA_40);
        startAnim(o, animY, -20, 320, 700 + random(0, 400), 0, 0, lv_anim_path_linear, random(0, 1200));
      }
      break;
    case FX_SNOW:
      for (int i = 0; i < 12; i++) {
        lv_obj_t *o = shape(hFx, random(0, 470), -10, 5, 5, 0xFFFFFF, LV_RADIUS_CIRCLE, LV_OPA_70);
        startAnim(o, animY, -10, 320, 4000 + random(0, 2500), 0, 0, lv_anim_path_linear, random(0, 4000));
      }
      break;
    case FX_STARS:
      for (int i = 0; i < 14; i++) {
        int s = random(2, 4);
        lv_obj_t *o = shape(hFx, random(0, 480), random(SB_H + 4, 220), s, s, 0xFFFFFF);
        startAnim(o, animOpa, 40, 255, 900 + random(0, 1400), 900 + random(0, 1400), random(0, 1500), lv_anim_path_ease_in_out, random(0, 2000));
      }
      break;
    case FX_SUN:
      shape(hFx, 340, -120, 240, 240, 0xFFD27A, LV_RADIUS_CIRCLE, LV_OPA_10);
      shape(hFx, 390, -70, 140, 140, 0xFFE7A0, LV_RADIUS_CIRCLE, LV_OPA_10);
      for (int i = 0; i < 6; i++) {
        lv_obj_t *o = shape(hFx, random(200, 470), random(SB_H + 4, 140), 3, 3, 0xFFF3C4);
        startAnim(o, animOpa, 0, 220, 1200 + random(0, 1200), 1200, random(0, 2000), lv_anim_path_ease_in_out, random(0, 2000));
      }
      break;
    case FX_FOG:
      for (int i = 0; i < 3; i++) {
        lv_obj_t *o = shape(hFx, -320, 90 + i * 70, 320, 16, 0xFFFFFF, 8, LV_OPA_10);
        startAnim(o, animX, -320, 480, 14000 + i * 3000, 0, 0, lv_anim_path_linear, i * 4000);
      }
      break;
  }
}
static int fxWanted() {
  if (!cfg.fxOn || !homeIsActive() || screenOff || ambOn) return FX_NONE;
  WeatherData w = weatherGet();
  if (!w.valid) return themeIsNight() ? FX_STARS : FX_NONE;
  int c = w.code;
  if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82) || c >= 95) return FX_RAIN;
  if ((c >= 71 && c <= 77) || c == 85 || c == 86) return FX_SNOW;
  if (c == 45 || c == 48) return FX_FOG;
  if (c <= 2) return w.isDay ? FX_SUN : FX_STARS;
  return FX_NONE;
}

// ---------- clock ----------
// Hours and minutes in Bebas Neue (clock_font.c, tools/gen_font.swift); the colon blinks with the seconds.
static const int CLOCK_X = 26, CLOCK_Y = 48;  // in the left column, leaving room for the pet above
static const int CLOCK_W = 194;               // "00:00" in clock_font
static int clockH = 80;                       // the font's line height = the digits' height
static const char *const HOME_WD_VI[7] = {"T2", "T3", "T4", "T5", "T6", "T7", "CN"};  // Monday first
static const char *const HOME_WD_EN[7] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
static lv_obj_t *lblHH, *lblColon, *lblMM;

static void clockSet(const char *hh, const char *mm, bool colon) {
  setText(lblHH, hh);
  setText(lblMM, mm);
  lv_opa_t o = colon ? LV_OPA_COVER : LV_OPA_20;
  if (lv_obj_get_style_text_opa(lblColon, 0) != o) lv_obj_set_style_text_opa(lblColon, o, 0);
}

// ---------- a little animal on the clock ----------
// The pet chosen in Settings > Home screen (or on the web page) pads back and forth on the left half of the clock's
// top edge: sprites made by tools/gen_pets.py from assets/*.png (walk, walk, rest, asleep, facing right; mirrored here
// for walking left). Now and then it
// lies down and blinks; at night it sleeps where it is. cfg.pet: 0 = none, 1..PET_N.
static const int PET_RUN = CLOCK_W / 2 - PET_W + 4;  // path length: from the corner to the middle of the clock
static const int PET_TICK_MS = 60;                  // one pixel per tick: ~17 px/s, smooth steps
static lv_obj_t *hPet;
static int petX = PET_RUN, petDir = -1;  // place on the path (0 = corner), +1 = walking right
static uint32_t petRestUntil = 0;       // millis() until which it lies down
static int petFrame = -1;

// Walking left: the right-facing frame mirrored into RAM (8-bit indexed: a 1 KB palette, then one byte per pixel).
// Two buffers taken in turn, so the image source always changes and LVGL redraws it.
static uint8_t petMirBuf[2][1024 + PET_W * PET_H];
static lv_img_dsc_t petMir[2];
static int petMirNext = 0;

static void petShow(int frame) {  // frame 0..3, facing the way it walks
  int f = frame + (petDir < 0 ? 4 : 0);
  if (f == petFrame) return;
  petFrame = f;
  const lv_img_dsc_t *src = PET_FRAMES[cfg.pet - 1][frame];
  if (petDir > 0) {
    lv_img_set_src(hPet, src);
    return;
  }
  int k = petMirNext;
  petMirNext ^= 1;
  uint8_t *dst = petMirBuf[k];
  memcpy(dst, src->data, 1024);
  for (int y = 0; y < PET_H; y++) {
    const uint8_t *row = src->data + 1024 + y * PET_W;
    uint8_t *out = dst + 1024 + y * PET_W;
    for (int x = 0; x < PET_W; x++) out[x] = row[PET_W - 1 - x];
  }
  petMir[k] = *src;
  petMir[k].data = dst;
  lv_img_cache_invalidate_src(&petMir[k]);
  lv_img_set_src(hPet, &petMir[k]);
}

static void petApply() {  // after cfg.pet changed (Settings, web page)
  if (!hPet) return;
  petFrame = -1;
  if (cfg.pet < 1 || cfg.pet > PET_N) {
    lv_obj_add_flag(hPet, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(hPet, LV_OBJ_FLAG_HIDDEN);
  petShow(0);
}

static void petTick(lv_timer_t *) {
  if (!hPet || cfg.pet < 1 || cfg.pet > PET_N || !homeIsActive() || screenOff) return;
  uint32_t now = millis();
  if (themeIsNight()) {  // asleep at night
    petShow(3);
    return;
  }
  if ((int32_t)(petRestUntil - now) > 0) {  // lying down: blinks for 150 ms every 2.5 s
    petShow((petRestUntil - now) % 2500 < 150 ? 3 : 2);
    return;
  }
  if (random(0, 300) == 0) {  // now and then (~every 18 s) a little rest
    petRestUntil = now + random(2500, 6000);
    petShow(2);
    return;
  }
  petX += petDir;
  if (petX <= 0 || petX >= PET_RUN) {  // an end of the path: turn round, sometimes after a rest
    petX = petX <= 0 ? 0 : PET_RUN;
    petDir = -petDir;
    if (random(0, 3) == 0) petRestUntil = now + random(1500, 4000);
  }
  lv_obj_set_x(hPet, CLOCK_X - 8 + petX);
  petShow((petX / 4) % 2);  // the two walking frames take turns every 4 px
}

// ---------- month calendar ----------
static const int MC_W = 220, MC_H = 166, MC_PAD = 7, MC_CW = 30, MC_TOP = 44;
struct MiniDay {
  uint8_t d, flags;  // d 0 = empty cell
};
enum { MD_TODAY = 1, MD_SUN = 2, MD_OFF = 4, MD_HOL = 8, MD_LUNAR1 = 16, MD_LUNAR15 = 32 };
static MiniDay mcDays[42];
static int mcRows = 5, mcKey = -1;
static char mcTitle[32];

static void mcBuild(int y, int m, int today) {
  int key = y * 10000 + m * 100 + today + (isVi() ? 0 : 50000000);
  if (key == mcKey) return;
  mcKey = key;
  if (isVi()) snprintf(mcTitle, sizeof(mcTitle), "Tháng %d, %d", m, y);
  else snprintf(mcTitle, sizeof(mcTitle), "%s %d", MONTHS_EN[m - 1], y);
  long first = lunar::jdFromDate(1, m, y);
  int ny = m == 12 ? y + 1 : y, nm = m == 12 ? 1 : m + 1;
  int dim = (int)(lunar::jdFromDate(1, nm, ny) - first), dow = alm::weekday(first);
  mcRows = dow + dim > 35 ? 6 : 5;
  memset(mcDays, 0, sizeof(mcDays));
  LunarDate lc = solarToLunar(1, m, y);
  int lcLen = lunarMonthLength(lc.start);
  for (int d = 1; d <= dim; d++) {
    long jd = first + d - 1;
    if (jd >= lc.start + lcLen) {  // the next lunar month starts inside this solar month
      int dd, mm, yy;
      jdToDate(jd, dd, mm, yy);
      lc = solarToLunar(dd, mm, yy);
      lcLen = lunarMonthLength(lc.start);
    }
    int ld = (int)(jd - lc.start) + 1;
    MiniDay &k = mcDays[dow + d - 1];
    k.d = d;
    if (d == today) k.flags |= MD_TODAY;
    if ((dow + d - 1) % 7 == 6) k.flags |= MD_SUN;
    if (ld == 1) k.flags |= MD_LUNAR1;
    if (ld == 15) k.flags |= MD_LUNAR15;
    const alm::Holiday *h[3];
    int n = alm::holidaysOn(d, m, ld, lc.month, lc.leap, lcLen, h);
    for (int i = 0; i < n; i++) k.flags |= h[i]->off ? (MD_OFF | MD_HOL) : MD_HOL;
  }
  lv_obj_invalidate(hCal);
}

static void mcDraw(lv_event_t *e) {
  lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);
  lv_area_t g;
  lv_obj_get_coords(hCal, &g);
  lv_draw_label_dsc_t ld;
  lv_draw_label_dsc_init(&ld);
  ld.font = F_LABEL;
  ld.color = C_TEXT;
  lv_area_t ta = {(lv_coord_t)(g.x1 + 12), (lv_coord_t)(g.y1 + 8), (lv_coord_t)(g.x2 - 12), (lv_coord_t)(g.y1 + 28)};
  lv_draw_label(ctx, &ld, &ta, mcTitle, nullptr);
  ld.font = F_CAPTION;
  ld.color = C_MUTED;
  ld.align = LV_TEXT_ALIGN_RIGHT;
  lv_draw_label(ctx, &ld, &ta, LV_SYMBOL_RIGHT, nullptr);

  ld.align = LV_TEXT_ALIGN_CENTER;
  for (int c = 0; c < 7; c++) {
    lv_area_t a = {(lv_coord_t)(g.x1 + MC_PAD + c * MC_CW), (lv_coord_t)(g.y1 + 28), (lv_coord_t)(g.x1 + MC_PAD + (c + 1) * MC_CW - 1), (lv_coord_t)(g.y1 + MC_TOP)};
    ld.color = c == 6 ? lv_color_hex(0xFF7A90) : lv_color_hex(0x9AA3C7);
    lv_draw_label(ctx, &ld, &a, isVi() ? HOME_WD_VI[c] : HOME_WD_EN[c], nullptr);
  }
  int rowH = (MC_H - MC_TOP - 4) / mcRows;
  ld.font = F_BODY;
  for (int i = 0; i < mcRows * 7; i++) {
    const MiniDay &k = mcDays[i];
    if (!k.d) continue;
    int r = i / 7, c = i % 7;
    lv_area_t a = {(lv_coord_t)(g.x1 + MC_PAD + c * MC_CW), (lv_coord_t)(g.y1 + MC_TOP + r * rowH), (lv_coord_t)(g.x1 + MC_PAD + (c + 1) * MC_CW - 1),
                   (lv_coord_t)(g.y1 + MC_TOP + (r + 1) * rowH - 1)};
    bool today = k.flags & MD_TODAY;
    if (today) {
      lv_draw_rect_dsc_t rd;
      lv_draw_rect_dsc_init(&rd);
      rd.radius = 8;
      rd.bg_color = C_ACCENT;
      rd.bg_opa = LV_OPA_COVER;
      lv_area_t b = {(lv_coord_t)(a.x1 + 2), (lv_coord_t)(a.y1 + 1), (lv_coord_t)(a.x2 - 2), (lv_coord_t)(a.y2 - 1)};
      lv_draw_rect(ctx, &rd, &b);
    }
    char b[4];
    snprintf(b, sizeof(b), "%d", k.d);
    lv_area_t na = {a.x1, (lv_coord_t)((a.y1 + a.y2) / 2 - 8), a.x2, a.y2};
    ld.color = today ? lv_color_hex(0x04101A) : (k.flags & (MD_SUN | MD_OFF)) ? lv_color_hex(0xFF7A90) : C_TEXT;
    lv_draw_label(ctx, &ld, &na, b, nullptr);
    if (k.flags & (MD_HOL | MD_LUNAR1 | MD_LUNAR15)) {  // dot: holiday (red off / amber), lunar 1st / 15th (violet)
      lv_draw_rect_dsc_t dd;
      lv_draw_rect_dsc_init(&dd);
      dd.radius = LV_RADIUS_CIRCLE;
      dd.bg_opa = LV_OPA_COVER;
      dd.bg_color = today ? lv_color_hex(0x04101A) : (k.flags & MD_OFF) ? lv_color_hex(0xFF5C7A) : (k.flags & MD_HOL) ? lv_color_hex(0xFFB84A) : lv_color_hex(0xC9A8FF);
      lv_area_t da = {(lv_coord_t)(a.x2 - 7), (lv_coord_t)(a.y1 + 3), (lv_coord_t)(a.x2 - 4), (lv_coord_t)(a.y1 + 6)};
      lv_draw_rect(ctx, &dd, &da);
    }
  }
}

// ---------- refresh ----------
static void refreshWeather() {
  WeatherData w = weatherGet();
  int key = WK_NONE;
  if (!w.valid) {
    setText(wTemp, "--");
    setText(wDesc, tr(w.failed ? "Weather unavailable" : (wifiConnected() ? "Loading weather..." : "Waiting for network")));
    setText(wRange, "");
    for (int i = 0; i < 4; i++) {
      setText(wHr[i], "");
      setText(wHt[i], "");
    }
  } else {
    key = iconKindFor(w.code, w.isDay);
    setTextFmt(wTemp, "%d%s", tempOut(w.temp), DEG);
    if (w.city[0]) setTextFmt(wDesc, "%s - %s", tr(weatherText(w.code)), w.city);
    else setText(wDesc, tr(weatherText(w.code)));
    if (w.nDays) setTextFmt(wRange, LV_SYMBOL_UP "%d%s  " LV_SYMBOL_DOWN "%d%s", tempOut(w.days[0].tmax), DEG, tempOut(w.days[0].tmin), DEG);
    else setText(wRange, "");
    for (int i = 0; i < 4; i++) {
      int idx = 1 + i * 2;  // +1h, +3h, +5h, +7h
      if (idx < w.nHours) {
        const WxHour &h = w.hours[idx];
        setTextFmt(wHr[i], "%dh", h.hour);
        setTextFmt(wHt[i], "%d%s", tempOut(h.temp), DEG);
      } else {
        setText(wHr[i], "");
        setText(wHt[i], "");
      }
    }
  }
  if (key != wIconKey) {
    drawWxIcon(wIcon, key);
    wIconKey = key;
  }
}

// One idempotent pass that makes the home screen reflect the current state.
static void refreshHome() {
  bool wifiOn = wifiConnected();
  struct tm t;
  bool timeOk = time(nullptr) > 1700000000 && getLocalTime(&t, 0);
  if (timeOk) {
    int h = t.tm_hour;
    const char *g = h >= 5 && h < 12 ? "Good morning" : h < 18 && h >= 12 ? "Good afternoon" : h >= 18 && h < 22 ? "Good evening" : "Good night";
    if (!cfg.greetOn) setText(lblGreet, "");
    else if (cfg.userName.length()) setTextFmt(lblGreet, "%s, %s", tr(g), cfg.userName.c_str());
    else setText(lblGreet, tr(g));
    int hh = cfg.clock24 ? h : (h % 12 ? h % 12 : 12);
    char hs[4], ms[4];
    snprintf(hs, sizeof(hs), cfg.clock24 ? "%02d" : "%d", hh);
    snprintf(ms, sizeof(ms), "%02d", t.tm_min);
    clockSet(hs, ms, t.tm_sec % 2 == 0);
    setText(lblAmPm, cfg.clock24 ? "" : isVi() ? (h < 12 ? "SA" : "CH") : (h < 12 ? "AM" : "PM"));

    // one line: the weekday in white, the date in the accent colour (label recolouring)
    const char *dow = isVi() ? DAYS_VI[t.tm_wday] : DAYS_EN[t.tm_wday];
    if (cfg.dateFmt == 2) setText(lblDate, "");
    else if (cfg.dateFmt == 1) setTextFmt(lblDate, "%s, #38E1FF %02d/%02d/%d#", dow, t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
    else if (isVi()) setTextFmt(lblDate, "%s, #38E1FF %d tháng %d#", dow, t.tm_mday, t.tm_mon + 1);
    else setTextFmt(lblDate, "%s, #38E1FF %d %s#", dow, t.tm_mday, MONTHS_EN[t.tm_mon]);
    setText(lblLunar, cfg.lunarOn ? lunarText(t.tm_mday, t.tm_mon + 1, t.tm_year + 1900).c_str() : "");
    mcBuild(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
  } else {
    setText(lblGreet, cfg.greetOn ? tr("Hello") : "");
    clockSet("--", "--", true);
    setText(lblAmPm, "");
    setText(lblDate, tr(wifiOn ? "Syncing time..." : "Waiting for network"));
    setText(lblLunar, "");
    static const char MON[] = "JanFebMarAprMayJunJulAugSepOctNovDec";  // not synced: the month of the build date
    char mon[4] = "";
    int y = 2026, d = 1;
    sscanf(__DATE__, "%3s %d %d", mon, &d, &y);
    const char *mp = strstr(MON, mon);
    mcBuild(y, mp ? (int)(mp - MON) / 3 + 1 : 1, 0);
  }
  // one line, taking turns every 5 s: the next alarm, then each countdown switched on for the home screen
  char al[48], cd[80];
  int ev[12];
  bool hasAl = homeAlarmText(al, sizeof(al));
  int nCd = cdHomeList(ev), total = nCd + (hasAl ? 1 : 0);
  int turn = total ? (int)((millis() / 5000) % total) : 0;
  bool showCd = nCd && !(hasAl && turn == 0);
  if (showCd) {
    cdHomeText(ev[hasAl ? turn - 1 : turn], cd, sizeof(cd));
    setTextFmt(lblAlarm, LV_SYMBOL_BELL "  %s", cd);
  } else if (hasAl) {
    setTextFmt(lblAlarm, LV_SYMBOL_BELL "  %s", al);
  } else {
    setText(lblAlarm, "");
  }
  lv_color_t lc = lv_color_hex(showCd ? 0xFF9BC8 : 0xFFD58A);
  if (lv_obj_get_style_text_color(lblAlarm, 0).full != lc.full) lv_obj_set_style_text_color(lblAlarm, lc, 0);
  refreshWeather();
  int fx = fxWanted();
  if (fx != fxKind) fxStart(fx);
}

static void homeTick(lv_timer_t *) {
  if (screenOff) return;  // backlight off: drawing would only cost power; refreshHome() runs again on wake
  if (lv_scr_act() != mainScr) {  // a page covers the home screen: nothing to update until it slides back
    if (fxKind != FX_NONE) fxStart(FX_NONE);
    return;
  }
  refreshHome();
}

// The dock: three app icons in a soft bar under the clock (Settings > Home screen > Dock).
static void homeDockBuild() {
  if (!hDock) return;
  lv_obj_clean(hDock);
  for (int k = 0; k < 3; k++) {
    int id = cfg.dock[k];
    lv_obj_t *it = lv_obj_create(hDock);
    lv_obj_remove_style_all(it);
    lv_obj_set_size(it, 72, 62);
    lv_obj_set_pos(it, 4 + k * 74, 2);
    lv_obj_set_style_radius(it, 14, 0);
    lv_obj_set_style_bg_color(it, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(it, LV_OPA_20, LV_STATE_PRESSED);
    lv_obj_clear_flag(it, LV_OBJ_FLAG_SCROLLABLE);
    if (id < 0) {  // empty slot: a faint plus
      lv_obj_t *p = makeLabel(it, LV_SYMBOL_PLUS, F_TITLE, C_MUTED);
      lv_obj_center(p);
      lv_obj_clear_flag(it, LV_OBJ_FLAG_CLICKABLE);
      continue;
    }
    lv_obj_add_flag(it, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(it, [](lv_event_t *e) { appOpen((int)(intptr_t)lv_event_get_user_data(e)); }, LV_EVENT_SHORT_CLICKED,
                        (void *)(intptr_t)id);
    lv_obj_t *ic = lv_img_create(it);  // icon only, at 70 % of its 56 px
    lv_img_set_src(ic, appIcon(id));
    lv_img_set_zoom(ic, 179);
    lv_img_set_antialias(ic, true);
    lv_obj_center(ic);
    lv_obj_clear_flag(ic, LV_OBJ_FLAG_CLICKABLE);
  }
}

// A plain container that opens a page when tapped.
static lv_obj_t *homeBox(lv_obj_t *parent, int x, int y, int w, int h, lv_event_cb_t cb) {
  lv_obj_t *b = lv_obj_create(parent);
  lv_obj_remove_style_all(b);
  lv_obj_set_size(b, w, h);
  lv_obj_set_pos(b, x, y);
  lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(b, cb, LV_EVENT_SHORT_CLICKED, nullptr);
  return b;
}

static lv_obj_t *homePanel(lv_obj_t *parent, int x, int y, int w, int h) {
  lv_obj_t *c = makeCard(parent, w, h);
  lv_obj_set_pos(c, x, y);
  lv_obj_set_style_bg_color(c, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(c, LV_OPA_30, 0);
  lv_obj_set_style_border_color(c, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_opa(c, LV_OPA_10, 0);
  lv_obj_set_style_radius(c, 18, 0);
  lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_color(c, C_ACCENT, LV_STATE_PRESSED);
  lv_obj_set_style_border_opa(c, LV_OPA_COVER, LV_STATE_PRESSED);
  return c;
}

static void buildHome(lv_obj_t *tile) {
  hFx = lv_obj_create(tile);  // weather effects, behind everything
  lv_obj_remove_style_all(hFx);
  lv_obj_set_size(hFx, 480, 320);
  lv_obj_clear_flag(hFx, LV_OBJ_FLAG_CLICKABLE);

  // left: greeting, clock, date (tap: Clock app)
  lv_obj_t *left = homeBox(tile, 0, SB_H + 4, 252, 268, [](lv_event_t *) { pageClock(); });
  lblGreet = makeLabel(left, "", F_LABEL, lv_color_hex(0xC9D2F2));  // top right: the pet walks on the left
  lv_obj_set_width(lblGreet, 150);
  lv_label_set_long_mode(lblGreet, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(lblGreet, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_align(lblGreet, LV_ALIGN_TOP_RIGHT, -12, 0);
  hClock = lv_obj_create(left);  // HH : MM in a row
  lv_obj_remove_style_all(hClock);
  lv_obj_set_size(hClock, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_pos(hClock, CLOCK_X, CLOCK_Y);
  lv_obj_set_flex_flow(hClock, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(hClock, 3, 0);
  lv_obj_clear_flag(hClock, LV_OBJ_FLAG_CLICKABLE);
  lblHH = makeLabel(hClock, "--", &clock_font, C_TEXT);
  lblColon = makeLabel(hClock, ":", &clock_font, C_ACCENT);
  lv_obj_set_style_translate_y(lblColon, -11, 0);  // Bebas Neue's colon sits low (centre 27 px up vs 38 for digits)
  lblMM = makeLabel(hClock, "--", &clock_font, C_TEXT);
  lblAmPm = makeLabel(hClock, "", F_LABEL, C_ACCENT);  // 12-hour clock: SA / CH right after the minutes, at the top
  clockH = lv_font_get_line_height(&clock_font);
  int y = CLOCK_Y + clockH + 8;  // under the clock: date, lunar date, alarm / countdown, then the dock
  lblDate = makeLabel(left, "", F_TITLE, C_TEXT);
  lv_label_set_recolor(lblDate, true);
  lv_obj_set_width(lblDate, 232);
  lv_label_set_long_mode(lblDate, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(lblDate, 12, y);
  lblLunar = makeLabel(left, "", F_BODY, lv_color_hex(0xD9C8FF));
  lv_obj_set_width(lblLunar, 228);
  lv_label_set_long_mode(lblLunar, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(lblLunar, 12, y + 26);
  lblAlarm = makeLabel(left, "", F_BODY, lv_color_hex(0xFFD58A));
  lv_obj_set_width(lblAlarm, 228);
  lv_label_set_long_mode(lblAlarm, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(lblAlarm, 12, y + 46);
  hDock = homePanel(left, 12, 268 - 66, 228, 66);  // bottom aligned with the weather card on the right
  lv_obj_clear_flag(hDock, LV_OBJ_FLAG_CLICKABLE);
  homeDockBuild();
  hPet = lv_img_create(left);  // last child: walks over the clock, paws on the digits
  lv_obj_clear_flag(hPet, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_pos(hPet, CLOCK_X - 8 + petX, CLOCK_Y - PET_H + 1);
  petApply();
  lv_timer_create(petTick, PET_TICK_MS, nullptr);


  // right: this month (tap: Calendar)
  lv_obj_t *calCard = homePanel(tile, 252, SB_H + 4, MC_W, MC_H);
  lv_obj_add_event_cb(calCard, [](lv_event_t *) { pageCalendar(); }, LV_EVENT_SHORT_CLICKED, nullptr);
  hCal = lv_obj_create(calCard);
  lv_obj_remove_style_all(hCal);
  lv_obj_set_size(hCal, MC_W, MC_H);
  lv_obj_clear_flag(hCal, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(hCal, mcDraw, LV_EVENT_DRAW_MAIN, nullptr);

  // right: weather now, today's high / low and the next hours (tap: forecast)
  wCard = homePanel(tile, 252, SB_H + MC_H + 10, MC_W, 96);
  lv_obj_add_event_cb(wCard, [](lv_event_t *) { pageWeather(); }, LV_EVENT_SHORT_CLICKED, nullptr);
  wIcon = wxIconBox(wCard, WK_NONE);
  lv_obj_set_pos(wIcon, 8, 4);
  wTemp = makeLabel(wCard, "--", F_HEADING, C_TEXT);
  lv_obj_set_pos(wTemp, 58, 2);
  wRange = makeLabel(wCard, "", F_CAPTION, C_MUTED);
  lv_obj_align(wRange, LV_ALIGN_TOP_RIGHT, -10, 10);
  wDesc = makeLabel(wCard, "", F_CAPTION, C_MUTED);
  lv_obj_set_width(wDesc, 156);
  lv_label_set_long_mode(wDesc, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(wDesc, 58, 34);
  shape(wCard, 10, 54, 200, 1, 0xFFFFFF, 0, LV_OPA_10);
  for (int i = 0; i < 4; i++) {
    int x = 10 + i * 52;
    wHr[i] = makeLabel(wCard, "", F_CAPTION, C_MUTED);
    lv_obj_set_pos(wHr[i], x, 58);
    wHt[i] = makeLabel(wCard, "", F_BODY, C_TEXT);
    lv_obj_set_pos(wHt[i], x, 73);
  }

}
