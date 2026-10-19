// Zin100 - ESP32-2432S035 (3.5" 480x320 landscape)
// Screen 1: showcase home (clock, mascot face, Xiaozhi status)   Screen 2: app launcher
// Swipe left/right between them. Apps open as full pages with a back button.
#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <time.h>
#include <SPI.h>
#include <SD.h>
#include <Preferences.h>
#include "wifi_setup.h"
#include "touch.h"
#include "xiaozhi_activation.h"
#include "weather.h"

#define FW_VERSION "1.0.0"
static const int PIN_BL = 27;

TFT_eSPI tft;
static SPIClass sdSPI(HSPI);  // TF slot: CS5 SCK18 MISO19 MOSI23
static bool sdOk = false;
static lv_color_t buf1[480 * 8], buf2[480 * 8];

// ---------- design tokens ----------
#define C_BG_TOP   lv_color_hex(0x070B1A)
#define C_BG_BOT   lv_color_hex(0x1D1146)
#define C_CARD     lv_color_hex(0x151B33)
#define C_BORDER   lv_color_hex(0x2A3359)
#define C_ACCENT   lv_color_hex(0x38E1FF)
#define C_ACCENT2  lv_color_hex(0xA16BFF)
#define C_TEXT     lv_color_hex(0xF2F5FF)
#define C_MUTED    lv_color_hex(0x8A93B8)
#define C_OK       lv_color_hex(0x3DEBA0)
#define C_WARN     lv_color_hex(0xFFB84A)
#define C_DANGER   lv_color_hex(0xFF5C7A)
#define F_CAPTION  (&lv_font_montserrat_12)
#define F_BODY     (&lv_font_montserrat_14)
#define F_LABEL    (&lv_font_montserrat_16)
#define F_TITLE    (&lv_font_montserrat_20)
#define F_HEADING  (&lv_font_montserrat_28)
#define F_DISPLAY  (&lv_font_montserrat_48)

// ---------- display / backlight ----------
static void dispFlush(lv_disp_drv_t *drv, const lv_area_t *a, lv_color_t *c) {
  uint32_t w = a->x2 - a->x1 + 1, h = a->y2 - a->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(a->x1, a->y1, w, h);
  tft.pushColors((uint16_t *)&c->full, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(drv);
}

static uint8_t blLevel = 255;
static void setBacklight(uint8_t v) {
  blLevel = v;
  ledcWrite(0, v);
}
static void backlightInit() {
  Preferences p;
  p.begin("ui", true);
  blLevel = p.getUChar("bl", 255);
  p.end();
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PIN_BL, 0);
  ledcWrite(0, blLevel);
}

// ---------- shared UI helpers ----------
static lv_obj_t *mainScr, *tv, *homeTile, *qrTile, *appsTile, *dots[3];

static void styleScreen(lv_obj_t *s) {
  lv_obj_set_style_bg_color(s, C_BG_TOP, 0);
  lv_obj_set_style_bg_grad_color(s, C_BG_BOT, 0);
  lv_obj_set_style_bg_grad_dir(s, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
  lv_obj_clear_flag(s, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *makeLabel(lv_obj_t *parent, const char *text, const lv_font_t *font, lv_color_t color) {
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, text);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, color, 0);
  return l;
}

static lv_obj_t *makeCard(lv_obj_t *parent, int w, int h) {
  lv_obj_t *c = lv_obj_create(parent);
  lv_obj_remove_style_all(c);
  lv_obj_set_size(c, w, h);
  lv_obj_set_style_radius(c, 14, 0);
  lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(c, C_CARD, 0);
  lv_obj_set_style_border_width(c, 1, 0);
  lv_obj_set_style_border_color(c, C_BORDER, 0);
  lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  return c;
}

#include "pages.h"

// ---------- home (showcase) ----------
static lv_obj_t *eyeL, *eyeR, *ring, *spinner;
static lv_obj_t *lblWifi, *lblXz, *lblGreet, *lblTime, *lblDate, *lblStatus;
static lv_obj_t *clockBox, *codeBox, *lblCode;
static bool listening = false;
static uint32_t bannerUntil = 0;

static void animEyeH(void *o, int32_t v) { lv_obj_set_height((lv_obj_t *)o, v); }
static void animRingOpa(void *o, int32_t v) { lv_obj_set_style_border_opa((lv_obj_t *)o, v, 0); }

static void startAnim(lv_obj_t *o, lv_anim_exec_xcb_t cb, int32_t from, int32_t to, uint32_t t,
                      uint32_t playback, uint32_t repeatDelay,
                      lv_anim_path_cb_t path = lv_anim_path_ease_in_out) {
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
  lv_anim_start(&a);
}

static lv_obj_t *makeEye(lv_obj_t *parent, int dx) {
  lv_obj_t *e = lv_obj_create(parent);
  lv_obj_remove_style_all(e);
  lv_obj_set_size(e, 38, 62);
  lv_obj_set_style_radius(e, 19, 0);
  lv_obj_set_style_bg_opa(e, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(e, C_ACCENT, 0);
  lv_obj_set_style_bg_grad_color(e, C_ACCENT2, 0);
  lv_obj_set_style_bg_grad_dir(e, LV_GRAD_DIR_VER, 0);
  lv_obj_align(e, LV_ALIGN_CENTER, dx, 0);
  lv_obj_clear_flag(e, LV_OBJ_FLAG_CLICKABLE);
  return e;
}

static void setEyeColor(lv_color_t col) {
  lv_obj_t *eyes[] = {eyeL, eyeR};
  for (lv_obj_t *e : eyes) lv_obj_set_style_bg_color(e, col, 0);
}

// ---------- weather card ----------
static const char DEG[] = {(char)0xC2, (char)0xB0, 0};  // UTF-8 degree sign
enum { WK_NONE, WK_CLEAR, WK_MOON, WK_PARTLY, WK_CLOUD, WK_FOG, WK_RAIN, WK_SNOW, WK_STORM };
static lv_obj_t *wCard, *wIcon, *wTemp, *wDesc;
static int wIconKey = -1;

static lv_obj_t *shape(lv_obj_t *p, int x, int y, int w, int h, uint32_t col, int radius = LV_RADIUS_CIRCLE,
                       int opa = LV_OPA_COVER) {
  lv_obj_t *o = lv_obj_create(p);
  lv_obj_remove_style_all(o);
  lv_obj_set_size(o, w, h);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_style_radius(o, radius, 0);
  lv_obj_set_style_bg_opa(o, opa, 0);
  lv_obj_set_style_bg_color(o, lv_color_hex(col), 0);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
  return o;
}
static void cloudShape(lv_obj_t *p, int dx, int dy, uint32_t col) {
  shape(p, dx + 2, dy + 12, 16, 16, col);
  shape(p, dx + 10, dy + 4, 22, 22, col);
  shape(p, dx + 24, dy + 11, 17, 17, col);
  shape(p, dx + 9, dy + 16, 24, 12, col, 6);
}
static void drawWeatherIcon(int kind) {
  lv_obj_clean(wIcon);
  switch (kind) {
    case WK_CLEAR:
      shape(wIcon, 2, 2, 40, 40, 0xFFC93C, LV_RADIUS_CIRCLE, LV_OPA_30);
      shape(wIcon, 9, 9, 26, 26, 0xFFC93C);
      break;
    case WK_MOON:
      shape(wIcon, 8, 8, 28, 28, 0xDDE3FF);
      shape(wIcon, 17, 4, 24, 24, 0x151B33);
      break;
    case WK_PARTLY:
      shape(wIcon, 0, 0, 24, 24, 0xFFC93C);
      cloudShape(wIcon, 3, 8, 0xDDE3FF);
      break;
    case WK_FOG:
      shape(wIcon, 4, 8, 36, 5, 0x9AA3C7, 3);
      shape(wIcon, 10, 19, 30, 5, 0x9AA3C7, 3);
      shape(wIcon, 4, 30, 36, 5, 0x9AA3C7, 3);
      break;
    case WK_RAIN:
      cloudShape(wIcon, 0, 0, 0x9AA3C7);
      shape(wIcon, 12, 31, 3, 9, 0x5AA9FF, 2);
      shape(wIcon, 21, 33, 3, 9, 0x5AA9FF, 2);
      shape(wIcon, 30, 31, 3, 9, 0x5AA9FF, 2);
      break;
    case WK_SNOW:
      cloudShape(wIcon, 0, 0, 0x9AA3C7);
      shape(wIcon, 11, 33, 5, 5, 0xFFFFFF);
      shape(wIcon, 21, 36, 5, 5, 0xFFFFFF);
      shape(wIcon, 30, 33, 5, 5, 0xFFFFFF);
      break;
    case WK_STORM: {
      cloudShape(wIcon, 0, 0, 0x7B84A8);
      lv_obj_t *z = lv_label_create(wIcon);
      lv_label_set_text(z, LV_SYMBOL_CHARGE);
      lv_obj_set_style_text_color(z, lv_color_hex(0xFFC93C), 0);
      lv_obj_set_pos(z, 16, 26);
      break;
    }
    case WK_CLOUD:
      cloudShape(wIcon, 0, 6, 0xB8C1E8);
      break;
    default:  // no data yet: dim cloud outline
      cloudShape(wIcon, 0, 6, 0x2A3359);
      break;
  }
}
static int iconKindFor(const WeatherData &w) {
  int c = w.code;
  if (c <= 1) return w.isDay ? WK_CLEAR : WK_MOON;
  if (c == 2) return WK_PARTLY;
  if (c == 45 || c == 48) return WK_FOG;
  if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82)) return WK_RAIN;
  if ((c >= 71 && c <= 77) || c == 85 || c == 86) return WK_SNOW;
  if (c >= 95) return WK_STORM;
  return WK_CLOUD;
}
static void refreshWeather() {
  WeatherData w = weatherGet();
  int key = WK_NONE;
  if (!w.valid) {
    lv_label_set_text(wTemp, "--");
    lv_label_set_text(wDesc, w.failed ? "Weather unavailable" : (wifiConnected() ? "Loading weather..." : "Waiting for network"));
  } else {
    key = iconKindFor(w);
    lv_label_set_text_fmt(wTemp, "%d%sC", (int)lroundf(w.temp), DEG);
    lv_label_set_text_fmt(wDesc, "%s - %s - %d%%", weatherText(w.code), w.city, w.humidity);
  }
  if (key != wIconKey) {
    drawWeatherIcon(key);
    wIconKey = key;
  }
}
static void weatherTapped(lv_event_t *) {
  weatherRefresh();
  lv_label_set_text(wDesc, "Updating...");
}

// One idempotent pass that makes the home screen reflect the current state.
static void refreshHome() {
  static const char *days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  static const char *months[] = {"January", "February", "March",     "April",   "May",      "June",
                                 "July",    "August",   "September", "October", "November", "December"};
  XzState st = xiaozhiState();

  // top bar
  bool wifiOn = wifiConnected();
  lv_label_set_text_fmt(lblWifi, LV_SYMBOL_WIFI "  %s", wifiOn ? WiFi.SSID().c_str() : "No Wi-Fi");
  lv_obj_set_style_text_color(lblWifi, wifiOn ? C_OK : C_WARN, 0);
  const char *xz = "Offline";
  lv_color_t xzc = C_MUTED;
  switch (st) {
    case XZ_ACTIVATED: xz = "Xiaozhi online"; xzc = C_OK; break;
    case XZ_CODE: xz = "Pairing"; xzc = C_WARN; break;
    case XZ_REQUESTING: xz = "Connecting..."; break;
    case XZ_ERROR: xz = "Retrying"; xzc = C_WARN; break;
    default: break;
  }
  lv_label_set_text(lblXz, xz);
  lv_obj_set_style_text_color(lblXz, xzc, 0);

  // right panel: pairing code or clock
  if (st == XZ_CODE) {
    lv_label_set_text(lblCode, xiaozhiCode());
    lv_obj_add_flag(clockBox, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(codeBox, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(codeBox, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(clockBox, LV_OBJ_FLAG_HIDDEN);
  }

  struct tm tmv;
  bool timeOk = wifiOn && getLocalTime(&tmv, 0) && tmv.tm_year > 120;
  if (timeOk) {
    int h = tmv.tm_hour;
    lv_label_set_text(lblGreet, h >= 5 && h < 12 ? "Good morning" : h < 18 && h >= 12 ? "Good afternoon" : h >= 18 && h < 22 ? "Good evening" : "Good night");
    lv_label_set_text_fmt(lblTime, "%02d:%02d", h, tmv.tm_min);
    lv_label_set_text_fmt(lblDate, "%s, %d %s", days[tmv.tm_wday], tmv.tm_mday, months[tmv.tm_mon]);
  } else {
    lv_label_set_text(lblGreet, "Hello");
    lv_label_set_text(lblTime, "--:--");
    lv_label_set_text(lblDate, wifiOn ? "Syncing time..." : "Waiting for network");
  }

  // status line + mascot state
  bool banner = millis() < bannerUntil;
  if (banner) {
    lv_label_set_text(lblStatus, "Device linked to xiaozhi.me");
    lv_obj_set_style_text_color(lblStatus, C_OK, 0);
  } else if (listening) {
    lv_label_set_text(lblStatus, "Listening...");
    lv_obj_set_style_text_color(lblStatus, C_OK, 0);
  } else if (st == XZ_ACTIVATED) {
    lv_label_set_text(lblStatus, "Tap the face to talk");
    lv_obj_set_style_text_color(lblStatus, C_MUTED, 0);
  } else {
    lv_label_set_text(lblStatus, xiaozhiMessage());
    lv_obj_set_style_text_color(lblStatus, C_MUTED, 0);
  }
  setEyeColor(banner || listening ? C_OK : C_ACCENT);
  if (listening) lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
  refreshWeather();
}

static void faceClicked(lv_event_t *) {
  if (xiaozhiState() != XZ_ACTIVATED) return;
  listening = !listening;
  refreshHome();
}

static void homeTick(lv_timer_t *) {
  static int lastState = -1;
  XzState st = xiaozhiState();
  if (st == XZ_ACTIVATED && lastState != XZ_ACTIVATED && lastState != -1) bannerUntil = millis() + 4000;
  lastState = st;
  refreshHome();
}

static void buildHome(lv_obj_t *tile) {
  lblWifi = makeLabel(tile, "", F_CAPTION, C_MUTED);
  lv_obj_align(lblWifi, LV_ALIGN_TOP_LEFT, 20, 14);
  lblXz = makeLabel(tile, "", F_CAPTION, C_MUTED);
  lv_obj_align(lblXz, LV_ALIGN_TOP_RIGHT, -20, 14);

  // mascot face
  lv_obj_t *face = lv_obj_create(tile);
  lv_obj_remove_style_all(face);
  lv_obj_set_size(face, 190, 190);
  lv_obj_align(face, LV_ALIGN_LEFT_MID, 30, -6);
  lv_obj_add_flag(face, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(face, faceClicked, LV_EVENT_CLICKED, nullptr);

  ring = lv_obj_create(face);
  lv_obj_remove_style_all(ring);
  lv_obj_set_size(ring, 170, 170);
  lv_obj_center(ring);
  lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(ring, 3, 0);
  lv_obj_set_style_border_color(ring, C_ACCENT2, 0);
  lv_obj_set_style_bg_opa(ring, LV_OPA_10, 0);
  lv_obj_set_style_bg_color(ring, C_ACCENT2, 0);
  lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE);
  startAnim(ring, animRingOpa, 60, 255, 2200, 2200, 0);

  spinner = lv_spinner_create(face, 1000, 70);
  lv_obj_set_size(spinner, 186, 186);
  lv_obj_center(spinner);
  lv_obj_set_style_arc_color(spinner, C_OK, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(spinner, lv_color_hex(0x1C2442), LV_PART_MAIN);
  lv_obj_set_style_arc_width(spinner, 6, LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(spinner, 6, LV_PART_MAIN);
  lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(spinner, LV_OBJ_FLAG_CLICKABLE);

  eyeL = makeEye(face, -36);
  eyeR = makeEye(face, 36);
  startAnim(eyeL, animEyeH, 62, 6, 120, 120, 3200, lv_anim_path_linear);  // blink
  startAnim(eyeR, animEyeH, 62, 6, 120, 120, 3200, lv_anim_path_linear);

  // right panel: clock
  clockBox = lv_obj_create(tile);
  lv_obj_remove_style_all(clockBox);
  lv_obj_set_size(clockBox, 214, 170);
  lv_obj_align(clockBox, LV_ALIGN_TOP_LEFT, 246, 62);
  lblGreet = makeLabel(clockBox, "", F_LABEL, C_MUTED);
  lv_obj_align(lblGreet, LV_ALIGN_TOP_LEFT, 0, 0);
  lblTime = makeLabel(clockBox, "--:--", F_DISPLAY, C_TEXT);
  lv_obj_align(lblTime, LV_ALIGN_TOP_LEFT, 0, 26);
  lblDate = makeLabel(clockBox, "", F_LABEL, C_ACCENT);
  lv_obj_align(lblDate, LV_ALIGN_TOP_LEFT, 0, 90);
  lblStatus = makeLabel(clockBox, "", F_BODY, C_MUTED);
  lv_obj_set_width(lblStatus, 214);
  lv_label_set_long_mode(lblStatus, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblStatus, LV_ALIGN_TOP_LEFT, 0, 126);

  // right panel: pairing code (shown only while pairing)
  codeBox = lv_obj_create(tile);
  lv_obj_remove_style_all(codeBox);
  lv_obj_set_size(codeBox, 214, 170);
  lv_obj_align(codeBox, LV_ALIGN_TOP_LEFT, 246, 62);
  lv_obj_t *ct = makeLabel(codeBox, "Pair this device", F_LABEL, C_MUTED);
  lv_obj_align(ct, LV_ALIGN_TOP_LEFT, 0, 0);
  lblCode = makeLabel(codeBox, "------", F_DISPLAY, C_ACCENT);
  lv_obj_set_style_text_letter_space(lblCode, 3, 0);
  lv_obj_align(lblCode, LV_ALIGN_TOP_LEFT, 0, 30);
  lv_obj_t *ch = makeLabel(codeBox, "Enter this code at xiaozhi.me > Add device", F_BODY, C_MUTED);
  lv_obj_set_width(ch, 214);
  lv_label_set_long_mode(ch, LV_LABEL_LONG_WRAP);
  lv_obj_align(ch, LV_ALIGN_TOP_LEFT, 0, 100);
  lv_obj_add_flag(codeBox, LV_OBJ_FLAG_HIDDEN);

  wCard = makeCard(tile, 214, 62);
  lv_obj_align(wCard, LV_ALIGN_TOP_LEFT, 246, 218);
  lv_obj_add_flag(wCard, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(wCard, weatherTapped, LV_EVENT_CLICKED, nullptr);
  wIcon = lv_obj_create(wCard);
  lv_obj_remove_style_all(wIcon);
  lv_obj_set_size(wIcon, 44, 44);
  lv_obj_align(wIcon, LV_ALIGN_LEFT_MID, 12, 0);
  lv_obj_clear_flag(wIcon, LV_OBJ_FLAG_CLICKABLE);
  wTemp = makeLabel(wCard, "--", F_HEADING, C_TEXT);
  lv_obj_align(wTemp, LV_ALIGN_TOP_LEFT, 66, 5);
  wDesc = makeLabel(wCard, "", F_CAPTION, C_MUTED);
  lv_obj_set_width(wDesc, 140);
  lv_label_set_long_mode(wDesc, LV_LABEL_LONG_DOT);
  lv_obj_align(wDesc, LV_ALIGN_TOP_LEFT, 66, 40);
}

// ---------- payment QR (VietQR, MB Bank) ----------
static const char *PAY_QR =
    "00020101021138570010A000000727012700069704220113VQRQAHIZT41140208QRIBFTTA53037045802VN62150107NPS686908006304C8A9";
static const char *PAY_NAME = "HO KINH DOANH ZPLUS";
static const char *PAY_ACCOUNT = "1122100288";

static void buildQr(lv_obj_t *tile) {
  // white card = quiet zone around the code; 225 px = 45 modules x 5 px, so every module is crisp
  lv_obj_t *card = lv_obj_create(tile);
  lv_obj_remove_style_all(card);
  lv_obj_set_size(card, 249, 249);
  lv_obj_align(card, LV_ALIGN_LEFT_MID, 24, -6);
  lv_obj_set_style_radius(card, 18, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *qr = lv_qrcode_create(card, 225, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
  lv_qrcode_update(qr, PAY_QR, strlen(PAY_QR));
  lv_obj_center(qr);

  lv_obj_t *t = makeLabel(tile, "Scan to pay", F_HEADING, C_TEXT);
  lv_obj_set_width(t, 184);
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 290, 36);
  lv_obj_t *bank = makeLabel(tile, "VietQR  |  MB Bank", F_BODY, C_ACCENT);
  lv_obj_align(bank, LV_ALIGN_TOP_LEFT, 290, 76);

  lv_obj_t *k1 = makeLabel(tile, "Account name", F_CAPTION, C_MUTED);
  lv_obj_align(k1, LV_ALIGN_TOP_LEFT, 290, 118);
  lv_obj_t *v1 = makeLabel(tile, PAY_NAME, F_BODY, C_TEXT);
  lv_obj_set_width(v1, 184);
  lv_obj_align(v1, LV_ALIGN_TOP_LEFT, 290, 136);

  lv_obj_t *k2 = makeLabel(tile, "Account number", F_CAPTION, C_MUTED);
  lv_obj_align(k2, LV_ALIGN_TOP_LEFT, 290, 172);
  lv_obj_t *v2 = makeLabel(tile, PAY_ACCOUNT, F_TITLE, C_TEXT);
  lv_obj_set_style_text_letter_space(v2, 1, 0);
  lv_obj_align(v2, LV_ALIGN_TOP_LEFT, 290, 190);

  lv_obj_t *h = makeLabel(tile, "Open your banking app and scan the code", F_CAPTION, C_MUTED);
  lv_obj_set_width(h, 184);
  lv_obj_align(h, LV_ALIGN_TOP_LEFT, 290, 238);
}

// ---------- launcher ----------
struct App {
  const char *icon, *name;
  uint32_t c1, c2;
  void (*open)();
};

static void goAssistant() {
  lv_obj_set_tile(tv, homeTile, LV_ANIM_ON);
  if (xiaozhiState() == XZ_ACTIVATED) {
    listening = true;
    refreshHome();
  }
}
static void soonMusic() { pageSoon("Music", LV_SYMBOL_AUDIO); }
static void soonGames() { pageSoon("Games", LV_SYMBOL_PLAY); }
static void soonPhotos() { pageSoon("Photos", LV_SYMBOL_IMAGE); }

static const App apps[] = {
    {LV_SYMBOL_VOLUME_MAX, "Assistant", 0x38E1FF, 0x2B6BFF, goAssistant},
    {LV_SYMBOL_AUDIO, "Music", 0xFF6B9A, 0xC2336B, soonMusic},
    {LV_SYMBOL_PLAY, "Games", 0xA16BFF, 0x5B2BD6, soonGames},
    {LV_SYMBOL_IMAGE, "Photos", 0xFFB84A, 0xE8702A, soonPhotos},
    {LV_SYMBOL_WIFI, "Wi-Fi", 0x3DEBA0, 0x16A085, pageWifi},
    {LV_SYMBOL_SD_CARD, "Storage", 0x5AA9FF, 0x3B6FD6, pageStorage},
    {LV_SYMBOL_SETTINGS, "Settings", 0x9AA3C7, 0x4E5878, pageSettings},
    {LV_SYMBOL_LIST, "About", 0x7BDCB5, 0x2E8B8B, pageAbout},
};

static const char *dbgClick = "-";
static void appClicked(lv_event_t *e) {
  const App *a = (const App *)lv_event_get_user_data(e);
  dbgClick = a->name;
  a->open();
}

// TEMP diagnostic strip on the top layer: last touch position, press count, last app clicked.
static void buildDebugStrip() {
  lv_obj_t *l = makeLabel(lv_layer_top(), "", F_CAPTION, lv_color_hex(0xFFD54A));
  lv_obj_align(l, LV_ALIGN_BOTTOM_LEFT, 6, -2);
  lv_timer_create(
      [](lv_timer_t *t) {
        lv_obj_t *lab = (lv_obj_t *)t->user_data;
        lv_label_set_text_fmt(lab, "raw %d,%d  pos %d,%d  n=%u  click=%s", dbgRawX, dbgRawY, dbgX, dbgY, (unsigned)dbgPresses, dbgClick);
      },
      150, l);
}

static void buildLauncher(lv_obj_t *tile) {
  lv_obj_t *t = makeLabel(tile, "Apps", F_HEADING, C_TEXT);
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 20, 10);

  lv_obj_t *grid = lv_obj_create(tile);
  lv_obj_remove_style_all(grid);
  lv_obj_set_size(grid, 448, 214);
  lv_obj_align(grid, LV_ALIGN_TOP_LEFT, 16, 52);
  lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_column(grid, 16, 0);
  lv_obj_set_style_pad_row(grid, 14, 0);
  lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

  for (const App &a : apps) {
    lv_obj_t *b = lv_obj_create(grid);
    lv_obj_remove_style_all(b);
    lv_obj_set_size(b, 100, 100);
    lv_obj_set_style_radius(b, 22, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(a.c1), 0);
    lv_obj_set_style_bg_grad_color(b, lv_color_hex(a.c2), 0);
    lv_obj_set_style_bg_grad_dir(b, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_opa(b, LV_OPA_70, LV_STATE_PRESSED);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(b, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_t *ic = makeLabel(b, a.icon, F_DISPLAY, C_TEXT);
    lv_obj_align(ic, LV_ALIGN_CENTER, 0, -12);
    lv_obj_t *nm = makeLabel(b, a.name, F_BODY, C_TEXT);
    lv_obj_align(nm, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(b, appClicked, LV_EVENT_CLICKED, (void *)&a);
  }
}

// ---------- page dots ----------
static void updateDots(int active) {
  for (int i = 0; i < 3; i++) {
    bool on = (i == active);
    lv_obj_set_size(dots[i], on ? 22 : 8, 8);
    lv_obj_set_style_bg_color(dots[i], on ? C_ACCENT : C_MUTED, 0);
    lv_obj_set_style_bg_opa(dots[i], on ? LV_OPA_COVER : LV_OPA_50, 0);
  }
}
static void tvEvent(lv_event_t *) {
  lv_obj_t *act = lv_tileview_get_tile_act(tv);
  updateDots(act == homeTile ? 0 : act == qrTile ? 1 : 2);
}

static void buildUI() {
  mainScr = lv_scr_act();
  styleScreen(mainScr);

  tv = lv_tileview_create(mainScr);
  lv_obj_set_size(tv, 480, 320);
  lv_obj_set_style_bg_opa(tv, LV_OPA_TRANSP, 0);
  lv_obj_set_scrollbar_mode(tv, LV_SCROLLBAR_MODE_OFF);
  homeTile = lv_tileview_add_tile(tv, 0, 0, LV_DIR_RIGHT);
  qrTile = lv_tileview_add_tile(tv, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
  appsTile = lv_tileview_add_tile(tv, 2, 0, LV_DIR_LEFT);
  buildHome(homeTile);
  buildQr(qrTile);
  buildLauncher(appsTile);
  lv_obj_add_event_cb(tv, tvEvent, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_t *dotRow = lv_obj_create(mainScr);
  lv_obj_remove_style_all(dotRow);
  lv_obj_set_size(dotRow, LV_SIZE_CONTENT, 8);
  lv_obj_set_flex_flow(dotRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(dotRow, 6, 0);
  lv_obj_clear_flag(dotRow, LV_OBJ_FLAG_CLICKABLE);
  for (int i = 0; i < 3; i++) {
    dots[i] = lv_obj_create(dotRow);
    lv_obj_remove_style_all(dots[i]);
    lv_obj_set_style_radius(dots[i], 4, 0);
    lv_obj_clear_flag(dots[i], LV_OBJ_FLAG_CLICKABLE);
  }
  lv_obj_align(dotRow, LV_ALIGN_BOTTOM_MID, 0, -12);
  updateDots(0);

  lv_timer_create(homeTick, 1000, nullptr);
  refreshHome();
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  backlightInit();
  {
    Preferences p;
    p.begin("ui", true);
    sleepMin = p.getUChar("sleep", 0);
    p.end();
  }

  lv_init();
  static lv_disp_draw_buf_t db;
  lv_disp_draw_buf_init(&db, buf1, buf2, 480 * 8);
  static lv_disp_drv_t dd;
  lv_disp_drv_init(&dd);
  dd.hor_res = 480;
  dd.ver_res = 320;
  dd.flush_cb = dispFlush;
  dd.draw_buf = &db;
  lv_disp_drv_register(&dd);

  xptInit();
  static lv_indev_drv_t id;
  lv_indev_drv_init(&id);
  id.type = LV_INDEV_TYPE_POINTER;
  id.read_cb = touchRead;
  id.scroll_limit = 24;  // px of travel before a press counts as a swipe (resistive jitter)
  lv_indev_drv_register(&id);

  sdSPI.begin(18, 19, 23, 5);
  sdOk = SD.begin(5, sdSPI, 20000000);

  pinMode(0, INPUT_PULLUP);  // hold BOOT while powering up to redo touch calibration
  if (digitalRead(0) == LOW || !touchLoadCal()) touchCalibrate();

  buildUI();
  buildDebugStrip();
  wifiBegin();
  xiaozhiBegin();
  weatherBegin();
}

// Turn the backlight off after the configured idle time; the first touch wakes it.
static void sleepTick() {
  static bool asleep = false;
  uint32_t idle = lv_disp_get_inactive_time(nullptr);
  if (!asleep && sleepMin && idle > (uint32_t)sleepMin * 60000UL) {
    asleep = true;
    ledcWrite(0, 0);
  } else if (asleep && idle < 1000) {
    asleep = false;
    ledcWrite(0, blLevel);
  }
}

void loop() {
  wifiLoop();
  lv_timer_handler();
  sleepTick();
  delay(5);
}
