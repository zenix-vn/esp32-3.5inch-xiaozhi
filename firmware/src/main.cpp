// Zin100 - ESP32-2432S035 (3.5" 480x320 landscape)
// Main screen: tiles (home, payment QR, app launcher) swiped left/right. Apps open as full pages with a back
// button. V2 shell: status bar + control center on the top layer, time-of-day themes, ambient screen.
#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <time.h>
#include <SPI.h>
#include <SD.h>
#include <Preferences.h>
#include "wifi_setup.h"
#include "i18n.h"
#include "vi_fonts.h"
#include "config.h"
#include "theme.h"
#include "sound.h"
#include "touch.h"
#include "xiaozhi_activation.h"
#include "weather.h"
#include "icons_img.h"

#define FW_VERSION "1.0.0"
static const int PIN_BL = 27;

TFT_eSPI tft;

// lv_conf.h: LVGL asserts (almost always an allocation that failed) land here instead of looping forever.
extern "C" void lv_assert_fail(void) {
  ets_printf("LVGL ASSERT at %p: free heap %u, largest block %u - restarting\n", __builtin_return_address(0), (unsigned)ESP.getFreeHeap(),
             (unsigned)ESP.getMaxAllocHeap());
  delay(200);
  ESP.restart();
}
static SPIClass sdSPI(VSPI);  // TF slot: CS5 SCK18 MISO19 MOSI23 (VSPI's native pins; HSPI drives the display)
static bool sdOk = false;
static lv_color_t *buf1, *buf2;  // draw buffers, allocated at boot (not .bss: keeps the big RAM region free for TLS)

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
#define F_CAPTION  (&vi_font_12)
#define F_BODY     (&vi_font_14)
#define F_LABEL    (&vi_font_16)
#define F_TITLE    (&vi_font_20)
#define F_HEADING  (&vi_font_28)
#define F_DISPLAY  (&lv_font_montserrat_48)

// ---------- display / backlight ----------
// DMA flush: a strip is sent to the panel while LVGL renders the next one into the other buffer, instead
// of the CPU waiting ~1.5 ms per strip on SPI. Build with -DTFT_USE_DMA=0 for the old blocking transfer.
#ifndef TFT_USE_DMA
#define TFT_USE_DMA 1
#endif
static volatile uint32_t gFlushCount = 0;  // photos.h redraws its picture when LVGL paints over it
static volatile uint32_t gFlushTop = 0;    // flushes that touched y < 268: wallpaper.h's preview picture area
static bool tftBusOpen = false;            // a DMA strip may still be in flight; the display holds the SPI bus

// Call before any other use of the shared SPI bus (touch, photos, screenshot): finishes the last DMA
// strip and releases the bus.
static void tftIdle() {
  if (!tftBusOpen) return;
  tft.dmaWait();
  tft.endWrite();
  tftBusOpen = false;
  // The IDF driver behind the DMA sets a MISO sampling delay tuned for 80 MHz; the slow CPU reads that follow
  // (touch at 2.5 MHz, screen readback at 16 MHz) need Arduino's default of no delay or they read shifted bits.
  WRITE_PERI_REG(SPI_CTRL2_REG(2), 0);  // SPI2 = HSPI, the display bus
}

static void dispFlush(lv_disp_drv_t *drv, const lv_area_t *a, lv_color_t *c) {
  gFlushCount++;
  if (a->y1 < 268) gFlushTop++;
  uint32_t w = a->x2 - a->x1 + 1, h = a->y2 - a->y1 + 1, n = w * h;
#if TFT_USE_DMA
  // LVGL renders little-endian RGB565, the panel wants big-endian: swap in place, two pixels per word
  uint32_t *q = (uint32_t *)&c->full;
  for (uint32_t i = 0; i < n / 2; i++) q[i] = ((q[i] & 0x00FF00FFu) << 8) | ((q[i] >> 8) & 0x00FF00FFu);
  if (n & 1) c[n - 1].full = __builtin_bswap16(c[n - 1].full);
  if (tftBusOpen) {
    tft.dmaWait();  // the previous strip must be out before the address window changes
  } else {
    tft.startWrite();
    tftBusOpen = true;
  }
  tft.setAddrWindow(a->x1, a->y1, w, h);
  tft.pushPixelsDMA((uint16_t *)&c->full, n);
#else
  tft.startWrite();
  tft.setAddrWindow(a->x1, a->y1, w, h);
  tft.pushColors((uint16_t *)&c->full, n, true);
  tft.endWrite();
#endif
  lv_disp_flush_ready(drv);  // double-buffered: LVGL draws into the other buffer while this one is sent
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
static const int SB_H = 22;  // status bar height (statusbar.h); pages start below it
static lv_obj_t *tv, *homeTile, *qrTile, *appsTile, *dots[3], *tileAt[3], *startTileObj;
static int tileCount = 3;

static lv_obj_t *makeLabel(lv_obj_t *parent, const char *text, const lv_font_t *font, lv_color_t color) {
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, tr(text));
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, color, 0);
  return l;
}

// lv_label_set_text() re-lays out and repaints a label even when the text is unchanged. The refresh timers
// (home, status bar, game, clock...) rewrite every label each tick, so they go through these instead and
// only labels whose text really changed are redrawn.
static void setText(lv_obj_t *l, const char *t) {
  if (strcmp(lv_label_get_text(l), t) != 0) lv_label_set_text(l, t);
}
static void setTextFmt(lv_obj_t *l, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void setTextFmt(lv_obj_t *l, const char *fmt, ...) {
  char b[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(b, sizeof(b), fmt, ap);
  va_end(ap);
  setText(l, b);
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

static bool screenOff = false;  // backlight off until the next touch (see screenSleepNow)

#include "audio.h"
#include "pages.h"

#include "dragon.h"
#include "statusbar.h"
#include "home.h"
#include "weather_page.h"
#include "clock_app.h"
#include "calendar_app.h"
#include "sleep_app.h"
#include "photos.h"
#include "qr_page.h"
#include "ambient.h"

// ---------- payment QR (VietQR; details come from the web setup page) ----------

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
  lv_obj_set_style_opa(card, LV_OPA_80, LV_STATE_PRESSED);
  lv_obj_add_event_cb(  // hold the code to open the Payment app (type an amount)
      card,
      [](lv_event_t *) {
        soundPlay(SND(SND_TAP), 0, false);
        pageQr();
      },
      LV_EVENT_LONG_PRESSED, nullptr);
  lv_obj_t *qr = lv_qrcode_create(card, 225, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
  String qrData = cfg.qrPayload.length() ? cfg.qrPayload : String(QR_DEFAULT_PAYLOAD);
  lv_qrcode_update(qr, qrData.c_str(), qrData.length());
  lv_obj_center(qr);

  lv_obj_t *t = makeLabel(tile, cfg.qrTitle.length() ? cfg.qrTitle.c_str() : "Scan to pay", F_HEADING, C_TEXT);
  lv_obj_set_width(t, 184);
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 290, 36);
  lv_obj_t *bank = makeLabel(tile, cfg.qrBank.c_str(), F_BODY, C_ACCENT);
  lv_obj_align(bank, LV_ALIGN_TOP_LEFT, 290, 76);

  lv_obj_t *k1 = makeLabel(tile, "Account name", F_CAPTION, C_MUTED);
  lv_obj_align(k1, LV_ALIGN_TOP_LEFT, 290, 118);
  lv_obj_t *v1 = makeLabel(tile, cfg.qrName.c_str(), F_BODY, C_TEXT);
  lv_obj_set_width(v1, 184);
  lv_obj_align(v1, LV_ALIGN_TOP_LEFT, 290, 136);

  lv_obj_t *k2 = makeLabel(tile, "Account number", F_CAPTION, C_MUTED);
  lv_obj_align(k2, LV_ALIGN_TOP_LEFT, 290, 172);
  lv_obj_t *v2 = makeLabel(tile, cfg.qrAcct.c_str(), F_TITLE, C_TEXT);
  lv_obj_set_style_text_letter_space(v2, 1, 0);
  lv_obj_align(v2, LV_ALIGN_TOP_LEFT, 290, 190);

  lv_obj_t *h = makeLabel(tile, "Open your banking app and scan the code", F_CAPTION, C_MUTED);
  lv_obj_set_width(h, 184);
  lv_obj_align(h, LV_ALIGN_TOP_LEFT, 290, 238);
}

// The selected payment account changed (web page or Settings): rebuild the QR tile in place.
static void qrRebuild() {
  if (!qrTile) return;
  lv_obj_clean(qrTile);
  buildQr(qrTile);
}

#include "wallpaper.h"
#include "settings_page.h"
#include "webconfig.h"

// ---------- launcher ----------
struct App {
  const lv_img_dsc_t *icon;
  const char *name;
  void (*open)();
};

static void goAssistant() {
  lv_obj_set_tile(tv, homeTile, LV_ANIM_ON);
  if (xiaozhiState() == XZ_ACTIVATED) {
    listening = true;
    refreshHome();
  }
}

static const App apps[] = {
    {&ic_assistant, "Assistant", goAssistant}, {&ic_weather, "Weather", pageWeather},  {&ic_dragon, "Dragon", pageDragon},
    {&ic_photos, "Photos", pagePhotos},        {&ic_wifi, "Wi-Fi", pageWifi},          {&ic_storage, "Storage", pageStorage},
    {&ic_settings, "Settings", pageSettings},  {&ic_about, "About", pageAbout},        {&ic_clock, "Clock", pageClock},
    {&ic_qr, "Payment", pageQr},
    {&ic_calendar, "Calendar", pageCalendar},
    {&ic_sleep, "Sleep sounds", pageSleep},
};
static_assert(sizeof(apps) / sizeof(apps[0]) == APP_COUNT, "APP_COUNT must match apps[]");
static const char *appName(int i) { return apps[i].name; }
static lv_obj_t *dragonBadge = nullptr;

static void appClicked(lv_event_t *e) {
  const App *a = (const App *)lv_event_get_user_data(e);
  soundPlay(SND(SND_TAP), 0, false);
  a->open();
}

static void buildLauncher(lv_obj_t *tile) {
  lv_obj_t *grid = lv_obj_create(tile);
  lv_obj_remove_style_all(grid);
  lv_obj_set_size(grid, 472, 320 - SB_H - 30);
  lv_obj_set_pos(grid, 4, SB_H + 4);
  lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_column(grid, 8, 0);
  lv_obj_set_style_pad_row(grid, 2, 0);
  lv_obj_set_scroll_dir(grid, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);

  int from = 0;
  while (from <= (int)cfg.apps.length()) {  // visible apps, in the order chosen on the web page
    int to = cfg.apps.indexOf(',', from);
    if (to < 0) to = cfg.apps.length();
    int idx = cfg.apps.substring(from, to).toInt();
    from = to + 1;
    if (idx < 0 || idx >= APP_COUNT || !appInLauncher(idx)) continue;
    const App &a = apps[idx];
    lv_obj_t *b = lv_obj_create(grid);
    lv_obj_remove_style_all(b);
    lv_obj_set_size(b, 112, 88);
    lv_obj_set_style_radius(b, 18, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_10, LV_STATE_PRESSED);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(b, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_t *ic = lv_img_create(b);
    lv_img_set_src(ic, a.icon);
    lv_obj_align(ic, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_img_opa(ic, LV_OPA_70, LV_STATE_PRESSED);
    lv_obj_add_flag(ic, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_t *nm = makeLabel(b, a.name, F_BODY, C_TEXT);
    lv_obj_align(nm, LV_ALIGN_BOTTOM_MID, 0, -4);
    if (idx == 2) {  // red dot while the dragon needs care
      dragonBadge = shape(b, 76, 0, 16, 16, 0xFF4D6D);
      lv_obj_set_style_border_width(dragonBadge, 2, 0);
      lv_obj_set_style_border_color(dragonBadge, lv_color_hex(0xFFFFFF), 0);
      lv_obj_add_flag(dragonBadge, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_add_event_cb(b, appClicked, LV_EVENT_CLICKED, (void *)&a);
  }
  lv_timer_create(
      [](lv_timer_t *) {
        if (!dragonBadge) return;
        if (dgAlert()) lv_obj_clear_flag(dragonBadge, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(dragonBadge, LV_OBJ_FLAG_HIDDEN);
      },
      2000, nullptr);
}

// ---------- page dots ----------
static void updateDots(int active) {
  for (int i = 0; i < tileCount; i++) {
    bool on = (i == active);
    lv_obj_set_size(dots[i], on ? 22 : 8, 8);
    lv_obj_set_style_bg_color(dots[i], on ? C_ACCENT : C_MUTED, 0);
    lv_obj_set_style_bg_opa(dots[i], on ? LV_OPA_COVER : LV_OPA_50, 0);
  }
}
static int activeTileIndex() {
  lv_obj_t *act = lv_tileview_get_tile_act(tv);
  for (int i = 0; i < tileCount; i++)
    if (tileAt[i] == act) return i;
  return 0;
}
static void tvEvent(lv_event_t *) { updateDots(activeTileIndex()); }
// Endless swiping: past the last screen comes the first one, and before the first comes the last. The tileview does
// not scroll there, so the swipe arrives as a gesture.
static void tvGesture(lv_event_t *) {
  if (tileCount < 2) return;
  lv_indev_t *in = lv_indev_get_act();
  if (!in) return;
  lv_dir_t d = lv_indev_get_gesture_dir(in);
  int a = activeTileIndex(), to = -1;
  if (d == LV_DIR_LEFT && a == tileCount - 1) to = 0;
  else if (d == LV_DIR_RIGHT && a == 0) to = tileCount - 1;
  if (to < 0) return;
  lv_indev_wait_release(in);  // LVGL would still send a click to the icon / widget where the swipe began
  lv_obj_set_tile(tv, tileAt[to], LV_ANIM_ON);
  updateDots(to);
}

static void buildUI() {
  mainScr = lv_scr_act();
  styleScreen(mainScr);

  tv = lv_tileview_create(mainScr);
  lv_obj_set_size(tv, 480, 320);
  lv_obj_set_style_bg_opa(tv, LV_OPA_TRANSP, 0);
  lv_obj_set_scrollbar_mode(tv, LV_SCROLLBAR_MODE_OFF);
  tileCount = cfg.qrOn ? 3 : 2;
  homeTile = lv_tileview_add_tile(tv, 0, 0, LV_DIR_RIGHT);
  qrTile = cfg.qrOn ? lv_tileview_add_tile(tv, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT) : nullptr;
  appsTile = lv_tileview_add_tile(tv, tileCount - 1, 0, LV_DIR_LEFT);
  tileAt[0] = homeTile;
  if (qrTile) tileAt[1] = qrTile;
  tileAt[tileCount - 1] = appsTile;
  buildHome(homeTile);
  if (qrTile) buildQr(qrTile);
  buildLauncher(appsTile);
  lv_obj_add_event_cb(tv, tvEvent, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_clear_flag(tv, LV_OBJ_FLAG_GESTURE_BUBBLE);  // LVGL delivers a swipe to the first ancestor WITHOUT this flag: make that the tileview
  lv_obj_add_event_cb(tv, tvGesture, LV_EVENT_GESTURE, nullptr);

  lv_obj_t *dotRow = lv_obj_create(mainScr);
  lv_obj_remove_style_all(dotRow);
  lv_obj_set_size(dotRow, LV_SIZE_CONTENT, 8);
  lv_obj_set_flex_flow(dotRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(dotRow, 6, 0);
  lv_obj_clear_flag(dotRow, LV_OBJ_FLAG_CLICKABLE);
  for (int i = 0; i < tileCount; i++) {
    dots[i] = lv_obj_create(dotRow);
    lv_obj_remove_style_all(dots[i]);
    lv_obj_set_style_radius(dots[i], 4, 0);
    lv_obj_clear_flag(dots[i], LV_OBJ_FLAG_CLICKABLE);
  }
  lv_obj_align(dotRow, LV_ALIGN_BOTTOM_MID, 0, -12);
  startTileObj = cfg.startTile == 2 ? appsTile : (cfg.startTile == 1 && qrTile) ? qrTile : homeTile;
  lv_obj_set_tile(tv, startTileObj, LV_ANIM_OFF);
  updateDots(activeTileIndex());

  lv_timer_create(homeTick, 1000, nullptr);
  // homeTick skips the home screen while a page covers it; bring it up to date as the slide back starts
  lv_obj_add_event_cb(mainScr, [](lv_event_t *) { refreshHome(); }, LV_EVENT_SCREEN_LOAD_START, nullptr);
  refreshHome();
}

void setup() {
  Serial.begin(115200);
  i18nLoad();
  cfgLoad();
  qrLoad();
  clockBegin();  // alarms ring whether or not the Clock app is ever opened
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
#if TFT_USE_DMA
  tft.initDMA();
#endif
  backlightInit();
  {
    Preferences p;
    p.begin("ui", true);
    sleepMin = p.getUChar("sleep", 0);
    p.end();
  }

  lv_init();
  static lv_disp_draw_buf_t db;
  // smallest-first so the draw buffers land in the small RAM regions and the large one stays free for TLS
  buf1 = (lv_color_t *)heap_caps_malloc(480 * 8 * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
  buf2 = (lv_color_t *)heap_caps_malloc(480 * 8 * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
  lv_disp_draw_buf_init(&db, buf1, buf2, 480 * 8);
  static lv_disp_drv_t dd;
  lv_disp_drv_init(&dd);
  dd.hor_res = 480;
  dd.ver_res = 320;
  dd.flush_cb = dispFlush;
  dd.draw_buf = &db;
  lv_disp_t *disp = lv_disp_drv_register(&dd);
  lv_disp_set_theme(disp, lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, &vi_font_14));

  xptInit();
  static lv_indev_drv_t id;
  lv_indev_drv_init(&id);
  id.type = LV_INDEV_TYPE_POINTER;
  id.read_cb = touchRead;
  id.scroll_limit = 24;  // px of travel before a press counts as a swipe (resistive jitter)
  lv_indev_drv_register(&id);

  sdSPI.begin(18, 19, 23, 5);
  sdOk = SD.begin(5, sdSPI, 10000000);  // 10 MHz: long writes (web uploads) stay reliable on this board

  pinMode(0, INPUT_PULLUP);  // hold BOOT while powering up to redo touch calibration
  if (digitalRead(0) == LOW || !touchLoadCal()) touchCalibrate();

  soundInit();
  buildUI();
  if (cfg.wallOn && !wallpaperOn()) cfg.wallOn = false;
  statusbarBegin();
  dragonBegin();
  wifiBegin();
  xiaozhiBegin();
  weatherBegin();
}

static bool homeIsActive() { return lv_scr_act() == mainScr && lv_tileview_get_tile_act(tv) == homeTile; }

// Screen off: backlight off until the next touch, which only wakes the screen (touch.h swallows it).
static void screenSleepNow() {
  screenOff = true;
  gTouchWake = false;
  gTouchSwallow = true;
  ledcWrite(0, 0);
}

// After the configured idle time: ambient screen or screen off (Settings + web page).
static void sleepTick() {
  if (screenOff) {
    if (gTouchWake) {
      gTouchWake = false;
      screenOff = false;
      ledcWrite(0, blLevel);
      lv_disp_trig_activity(nullptr);
    }
    return;
  }
  if (ambOn || !sleepMin) return;
  if (lv_disp_get_inactive_time(nullptr) > (uint32_t)sleepMin * 60000UL) {
    if (cfg.idleMode == 1) ambientShow();
    else screenSleepNow();
  }
}

// Go back to the chosen start screen after the configured idle time.
static void idleReturnTick() {
  if (!cfg.idleReturn || lv_scr_act() != mainScr) return;
  if (lv_disp_get_inactive_time(nullptr) > (uint32_t)cfg.idleReturn * 1000UL && lv_tileview_get_tile_act(tv) != startTileObj) {
    lv_obj_set_tile(tv, startTileObj, LV_ANIM_ON);
    updateDots(activeTileIndex());
  }
}

void loop() {
  wifiLoop();
  webTick();
  uint32_t idle = lv_timer_handler();  // ms until LVGL's next timer is due
  photosLoop();
  wpPreviewLoop();
  sleepTick();
  idleReturnTick();
  soundTick();
  static uint32_t clockAt = 0;
  if (millis() - clockAt > 250) {
    clockAt = millis();
    clockTick();
  }
  static uint32_t slowAt = 0;
  if (millis() - slowAt > 5000) {
    slowAt = millis();
    themeTick();
    ambientTick();
  }
  static uint32_t memAt = 0;
  if (millis() - memAt > 30000) {  // heap health on the serial log: TLS needs ~40 KB in one block
    memAt = millis();
    Serial.printf("MEM free=%u min=%u largest=%u wifi=%d %s\n", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
                  (unsigned)ESP.getMaxAllocHeap(), (int)WiFi.status(), wifiStatus());
  }
  // sleep only until LVGL needs to run again (was a fixed 5 ms, which delayed every frame and touch read)
  delay(idle < 1 ? 1 : idle > 5 ? 5 : idle);
}
