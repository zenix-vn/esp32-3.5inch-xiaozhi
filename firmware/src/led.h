#pragma once
// Front RGB LED of the ESP32-2432S035 (common anode: R = IO4, G = IO16, B = IO17, LOW = on), driven by LEDC so it
// can fade. ledTick() (from loop) picks what to show, highest priority first:
//   test pattern (Settings) > alarm / timer ringing > one-shot flash > Wi-Fi setup portal > listening (mic)
//   > radio > sleep sounds > charging > low battery > dragon needs care > screen off / ambient (standby breath)
// One-shot flashes come from events elsewhere: a touch, a toast (in the toast's colour), interface sounds (ok,
// error, coin, evolve...), the charger, Wi-Fi connecting or dropping.
// Modes (Settings > LED light): off, alerts only (alarm, charger, battery, setup portal, errors) or everything.
// At night (inside the ambient schedule, or in do-not-disturb) "quiet at night" keeps only
// the alarm and a nearly empty battery, at a third of the brightness.
// Included by main.cpp after settings_page.h; sound.h, touch.h and statusbar.h forward-declare what they call.

#ifndef LED_PIN_R
#define LED_PIN_R 4
#endif
#ifndef LED_PIN_G
#define LED_PIN_G 16
#endif
#ifndef LED_PIN_B
#define LED_PIN_B 17
#endif
#ifndef LED_ACTIVE_LOW
#define LED_ACTIVE_LOW 1  // common anode: the pin sinks the current
#endif

static const int LED_CH_R = 4, LED_CH_G = 5, LED_CH_B = 6;  // 0 = backlight, 2 = speaker (each pair shares a timer)
static const int LED_MAX = 1023;                             // 10-bit duty
static const uint32_t LED_RAINBOW = 0x1000000;               // ledFlash colour: cycle through the hues

static uint32_t ledFlCol = 0;  // one-shot flash: colour, blinks left to show, length of one blink
static int ledFlN = 0, ledFlMs = 0;
static bool ledFlAlert = false;  // shown at night too
static uint32_t ledFlStart = 0;
static uint32_t ledTestUntil = 0;  // Settings > Test: rainbow + the status colours until then
static int ledLast[3] = {-1, -1, -1};

static void ledWriteRaw(int r, int g, int b) {
  int v[3] = {r, g, b};
  const int ch[3] = {LED_CH_R, LED_CH_G, LED_CH_B};
  for (int i = 0; i < 3; i++) {
    if (v[i] == ledLast[i]) continue;
    ledLast[i] = v[i];
    ledcWrite(ch[i], LED_ACTIVE_LOW ? LED_MAX - v[i] : v[i]);
  }
}

static void ledBegin() {
  const int pin[3] = {LED_PIN_R, LED_PIN_G, LED_PIN_B}, ch[3] = {LED_CH_R, LED_CH_G, LED_CH_B};
  for (int i = 0; i < 3; i++) {
    ledcSetup(ch[i], 25000, 10);  // above hearing (5 kHz whined in the speaker)
    ledcAttachPin(pin[i], ch[i]);
  }
  ledWriteRaw(0, 0, 0);
}

// colour 0xRRGGBB at level 0..255 -> duty, with gamma (the eye is far more sensitive to low levels) and the
// brightness setting. The green die is much brighter than the red and blue ones: scaled down to match.
static void ledShow(uint32_t rgb, int level, bool night) {
  if (cfg.ledMode == 0 || level <= 0 || !rgb) {
    ledWriteRaw(0, 0, 0);
    return;
  }
  static const float W[3] = {1.0f, 0.55f, 0.85f};
  float k = level / 255.0f * cfg.ledBri / 100.0f * (night ? 0.33f : 1.0f);
  int out[3];
  for (int i = 0; i < 3; i++) {
    float c = ((rgb >> (16 - 8 * i)) & 0xFF) / 255.0f;
    float d = c * c * k * W[i];  // gamma 2
    out[i] = d <= 0 ? 0 : max(1, (int)(d * LED_MAX));
  }
  ledWriteRaw(out[0], out[1], out[2]);
}

// hue 0..359 -> 0xRRGGBB, full saturation
static uint32_t ledHue(int h) {
  h %= 360;
  int x = (h % 60) * 255 / 60, r, g, b;
  switch (h / 60) {
    case 0: r = 255, g = x, b = 0; break;
    case 1: r = 255 - x, g = 255, b = 0; break;
    case 2: r = 0, g = 255, b = x; break;
    case 3: r = 0, g = 255 - x, b = 255; break;
    case 4: r = x, g = 0, b = 255; break;
    default: r = 255, g = 0, b = 255 - x; break;
  }
  return (r << 16) | (g << 8) | b;
}

// smooth 0..255..0 over period ms
static int ledBreath(uint32_t now, uint32_t period, int lo = 0) {
  float t = (float)(now % period) / period;
  return lo + (int)((255 - lo) * (0.5f - 0.5f * cosf(t * 2 * PI)));
}
// n short blinks of onMs (gap the same) at the start of each period: 255 or 0
static int ledBlinks(uint32_t now, uint32_t period, int n, uint32_t onMs) {
  uint32_t t = now % period;
  if (t >= (uint32_t)n * onMs * 2) return 0;
  return (t / onMs) % 2 == 0 ? 255 : 0;
}

// ---------- events ----------
// blinks of ms each (with a gap as long); a newer flash replaces the one showing
static void ledFlash(uint32_t rgb, int blinks, int ms) {
  if (cfg.ledMode != 2) return;
  ledFlAlert = false;
  ledFlCol = rgb;
  ledFlN = blinks;
  ledFlMs = ms;
  ledFlStart = millis();
}
// alerts are shown in the "alerts only" mode too
static void ledAlert(uint32_t rgb, int blinks, int ms) {
  if (cfg.ledMode == 0) return;
  ledFlAlert = true;
  ledFlCol = rgb;
  ledFlN = blinks;
  ledFlMs = ms;
  ledFlStart = millis();
}

static void ledForSound(const Note *seq) {
  if (seq == SND_OK) ledFlash(0x20FF60, 1, 180);
  else if (seq == SND_ERROR) ledAlert(0xFF1010, 2, 140);
  else if (seq == SND_NOTIFY) ledFlash(0x30D0FF, 2, 110);
  else if (seq == SND_COIN) ledFlash(0xFFB000, 1, 140);
  else if (seq == SND_CHIRP) ledFlash(0x40FFE0, 1, 120);
  else if (seq == SND_EVOLVE) ledFlash(LED_RAINBOW, 1, 1500);
}

static void ledForToast(lv_color_t col) { ledFlash(lv_color_to32(col) & 0xFFFFFF, 1, 220); }

static void ledTest() { ledTestUntil = millis() + 7000; }

// ---------- what to show ----------
static bool ledIsNight() {
  if (cfg.dnd) return true;
  if (cfg.ambStart < 0) return false;
  struct tm t;
  if (time(nullptr) < 1700000000 || !getLocalTime(&t, 0)) return false;
  int h = t.tm_hour, s = cfg.ambStart, e = cfg.ambEnd;
  return s <= e ? (h >= s && h < e) : (h >= s || h < e);
}

static uint32_t ledSleepCol() {
  const char *now = audioBgPlaying();
  if (!now) return 0;
  for (int i = 0; i < SLEEP_N; i++)
    if (!strcmp(now, SLEEP_SND[i].file)) return SLEEP_SND[i].col;
  return 0x8E98B8;
}

static void ledTick() {
  static uint32_t at = 0;
  uint32_t now = millis();
  if (now - at < 15) return;
  at = now;

  // events found by watching state: touches, Wi-Fi up / down
  static uint32_t touches = 0;
  if (gTouchPresses != touches) {
    touches = gTouchPresses;
    if (cfg.ledTouch && !(ledFlN && ledFlMs > 100)) ledFlash(0x9AB0FF, 1, 60);  // a real flash is not cut short
  }
  static int wifiWas = -1;
  int wifiNow = wifiConnected();
  if (wifiWas >= 0 && wifiNow != wifiWas) {
    if (wifiNow) ledFlash(0x2060FF, 2, 150);
    else ledAlert(0xFF2000, 3, 150);
  }
  wifiWas = wifiNow;

  if (cfg.ledMode == 0) {
    ledFlN = 0;
    ledWriteRaw(0, 0, 0);
    return;
  }
  bool night = cfg.ledNight && ledIsNight();
  bool full = cfg.ledMode == 2;

  if ((int32_t)(ledTestUntil - now) > 0) {  // 3 s of rainbow, then the status colours one after another
    uint32_t el = 7000 - (ledTestUntil - now);
    static const uint32_t STEPS[] = {0xFF2000, 0xFF9000, 0x20FF60, 0x30D0FF, 0x2060FF, 0xB040FF, 0xFFFFFF, 0xFF1030};
    if (el < 3000) ledShow(ledHue(el * 360 / 1000), 255, false);
    else ledShow(STEPS[((el - 3000) / 500) % 8], 255, false);
    return;
  }

  // alarm or timer ringing: fast red / orange
  if (ringOv) {
    bool a = (now / 250) % 2;
    ledShow(a ? 0xFF1000 : 0xFF7000, 255, false);
    return;
  }

  if (ledFlN > 0 && night && !ledFlAlert) ledFlN = 0;
  if (ledFlN > 0) {
    uint32_t el = now - ledFlStart;
    if (el < (uint32_t)(ledFlN * 2 - 1) * ledFlMs) {
      if (ledFlCol == LED_RAINBOW) ledShow(ledHue(el * 720 / ledFlMs), 255 - (int)(el * 200 / ledFlMs), night);
      else ledShow(ledFlCol, (el / ledFlMs) % 2 == 0 ? 255 : 0, night);
      return;
    }
    ledFlN = 0;
  }

  if (!night && wifiPortalActive()) {  // Wi-Fi setup hotspot waiting for a phone: purple double blink
    ledShow(0xB040FF, ledBlinks(now, 2000, 2, 120), night);
    return;
  }

  bool lowBat = batPresent && !batUsb && batPct >= 0 && batPct <= 15;
  bool critBat = lowBat && batPct <= 5;
  if (night) {  // quiet: only an almost empty battery
    ledShow(critBat ? 0xFF0000 : 0, ledBlinks(now, 6000, 1, 100), true);
    return;
  }

  if (full && micUp) {  // listening: quick cyan breath
    ledShow(0x00E0FF, ledBreath(now, 1200, 40), night);
    return;
  }
  if (full && radioCh >= 0 && radioState != RS_OFF) {
    if (radioState == RS_PLAY) ledShow(RADIO_CH[radioCh].col, ledBreath(now, 4000, 30) / 2, night);
    else if (radioState == RS_ERROR) ledShow(0xFF2000, ledBlinks(now, 1500, 2, 100), night);
    else ledShow(0x2060FF, ledBreath(now, 600), night);  // connecting / buffering
    return;
  }
  uint32_t slc = full ? ledSleepCol() : 0;
  if (slc) {  // sleep sound: very slow, dim breath in the sound's colour
    ledShow(slc, ledBreath(now, 7000) / 4, night);
    return;
  }

  if (batPresent && batUsb) {  // charging: amber breath; full: steady soft green
    if (batFull) ledShow(0x20FF40, 70, night);
    else ledShow(0xFF8000, ledBreath(now, 2600, 20) * 2 / 3, night);
    return;
  }
  if (lowBat) {
    ledShow(0xFF0000, critBat ? ledBlinks(now, 1500, 1, 120) : ledBlinks(now, 4000, 1, 100), night);
    return;
  }
  if (full && dgAlert()) {  // the dragon needs care: yellow heartbeat
    ledShow(0xFFC000, ledBlinks(now, 5000, 2, 110) / 2, night);
    return;
  }
  if (full && (screenOff || ambOn)) {  // standby: a faint, slow breath so the device does not look dead
    ledShow(0x4060FF, ledBreath(now, 8000) / 6, night);
    return;
  }
  ledWriteRaw(0, 0, 0);
}

// ---------- Settings > LED light ----------
static lv_obj_t *ledBriPct;
static lv_obj_t *ledSwitchRow(lv_obj_t *grp, const char *sym, uint32_t col, const char *title, bool on, lv_event_cb_t cb) {
  lv_obj_t *r = stRow(grp, sym, col, title, "", C_MUTED, nullptr, false);
  lv_obj_t *sw = lv_switch_create(r);
  lv_obj_set_size(sw, 52, 28);
  lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -14, 0);
  lv_obj_set_style_bg_color(sw, C_BORDER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(sw, C_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
  if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
  lv_obj_add_event_cb(sw, cb, LV_EVENT_VALUE_CHANGED, nullptr);
  return r;
}

static const char *ledModeName(int m) { return m == 0 ? DL("Tắt", "Off") : m == 1 ? DL("Chỉ cảnh báo", "Alerts only") : DL("Đầy đủ", "Everything"); }

static void pageSettingsLed() {
  lv_obj_t *c;
  lv_obj_t *s = stPage(DL("Đèn LED", "LED light"), &c);

  lv_obj_t *card = makeCard(c, 456, 88);
  lv_obj_t *t = makeLabel(card, "", F_BODY, C_TEXT);
  lv_label_set_text(t, DL("Đèn báo", "The light shows"));
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 16, 10);
  static const char *modeMap[4];
  for (int i = 0; i < 3; i++) modeMap[i] = ledModeName(i);
  modeMap[3] = "";
  lv_obj_t *chips = ckChips(card, modeMap, true, [](lv_event_t *e) {
    uint32_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
    if (id > 2) return;
    cfg.ledMode = id;
    cfgSave();
    if (id) ledTest();
  });
  lv_btnmatrix_set_btn_ctrl(chips, cfg.ledMode, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_set_size(chips, 424, 38);
  lv_obj_align(chips, LV_ALIGN_BOTTOM_MID, 0, -10);

  stSlider(c, LV_SYMBOL_EYE_OPEN, 0xF2A33A, DL("Độ sáng đèn", "Light brightness"), 5, 100, cfg.ledBri, [](lv_event_t *e) {
    cfg.ledBri = lv_slider_get_value(lv_event_get_target(e));
    setTextFmt(ledBriPct, "%d%%", cfg.ledBri);
    ledAlert(0xFFFFFF, 1, 400);  // preview the level
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) cfgSave();
  }, &ledBriPct);
  setTextFmt(ledBriPct, "%d%%", cfg.ledBri);

  lv_obj_t *g = stGroup(c);
  ledSwitchRow(g, LV_SYMBOL_EDIT, 0x4C8BF5, DL("Nháy khi chạm màn hình", "Blink on every touch"), cfg.ledTouch, [](lv_event_t *e) {
    cfg.ledTouch = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    cfgSave();
  });
  ledSwitchRow(g, LV_SYMBOL_EYE_CLOSE, 0x6A5BD6, DL("Yên lặng ban đêm", "Quiet at night"), cfg.ledNight, [](lv_event_t *e) {
    cfg.ledNight = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    cfgSave();
  });
  stRow(g, LV_SYMBOL_PLAY, 0x35B0A8, DL("Thử đèn", "Test the light"), "", C_MUTED, [](lv_event_t *) { ledTest(); }, false);

  stHeader(c, DL("Ý NGHĨA MÀU", "WHAT THE COLOURS MEAN"));
  lv_obj_t *h = makeLabel(c, "", F_CAPTION, C_MUTED);
  lv_obj_set_width(h, 440);
  lv_label_set_long_mode(h, LV_LABEL_LONG_WRAP);
  lv_label_set_text(h, DL("Đỏ/cam nháy nhanh: báo thức, hẹn giờ\n"
                          "Cam thở: đang sạc  -  Xanh lá sáng đều: sạc đầy\n"
                          "Đỏ nháy chậm: pin yếu (nhanh hơn khi gần hết)\n"
                          "Tím nháy đôi: đang chờ cài Wi-Fi bằng điện thoại\n"
                          "Xanh dương nháy 2 lần: có Wi-Fi  -  Đỏ nháy 3 lần: mất Wi-Fi\n"
                          "Xanh lơ thở nhanh: mic đang nghe\n"
                          "Radio: thở theo màu kênh (xanh dương nhấp nháy khi đang kết nối)\n"
                          "Âm thanh ru ngủ: thở rất chậm theo màu âm thanh\n"
                          "Vàng nháy đôi: rồng cần chăm sóc\n"
                          "Xanh mờ thở chậm: màn hình đang tắt\n"
                          "Thông báo trên màn hình, tiếng xu, lên cấp...: nháy theo màu tương ứng\n"
                          "Ban đêm / Im lặng: chỉ còn báo thức và pin sắp hết.",
                          "Fast red/orange: alarm or timer\n"
                          "Amber breathing: charging  -  Steady green: fully charged\n"
                          "Slow red blink: low battery (faster when nearly empty)\n"
                          "Purple double blink: waiting for Wi-Fi setup from a phone\n"
                          "Blue x2: Wi-Fi connected  -  Red x3: Wi-Fi lost\n"
                          "Quick cyan breathing: the mic is listening\n"
                          "Radio: breathes in the channel colour (blue pulse while connecting)\n"
                          "Sleep sounds: very slow breath in the sound's colour\n"
                          "Yellow double blink: the dragon needs care\n"
                          "Faint slow blue: the screen is off\n"
                          "Messages, coins, level-ups...: a blink in a matching colour\n"
                          "At night / do not disturb: only alarms and an empty battery."));
  pageShow(s);
}
