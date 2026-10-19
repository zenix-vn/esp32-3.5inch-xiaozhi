#pragma once
// Home V2: the user's dragon is the companion (tap = talk to the AI, hold = open the game), with a speech
// bubble, the clock, the lunar date, weather with the next hours, the quote of the day and weather effects
// drifting over the background. Included by main.cpp after dragon.h / statusbar.h / ambient.h.
#include "lunar.h"
#include "quotes.h"

static const char DEG[] = {(char)0xC2, (char)0xB0, 0};  // UTF-8 degree sign
static const char *const DAYS_VI[] = {"Chủ Nhật", "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy"};
static const char *const DAYS_EN[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char *const MONTHS_EN[] = {"January", "February", "March",     "April",   "May",      "June",
                                        "July",    "August",   "September", "October", "November", "December"};
static void pageWeather();  // weather_page.h

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
static void animOpa(void *o, int32_t v) { lv_obj_set_style_opa((lv_obj_t *)o, v, 0); }
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
      lv_label_set_text(z, LV_SYMBOL_CHARGE);
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
static lv_obj_t *hFx, *hPet, *hPetBob, *hSpinner, *hBubble, *hBubbleLbl;
static lv_obj_t *lblGreet, *lblTime, *lblAmPm, *lblDate, *lblLunar, *clockBox, *codeBox, *lblCode;
static lv_obj_t *wCard, *wIcon, *wTemp, *wDesc, *wHr[3], *wHt[3], *wHp[3];
static lv_obj_t *hQuote;
static bool listening = false;
static uint32_t bannerUntil = 0;
static int hPetSig = -1, wIconKey = -1, fxKind = -1, quoteDay = -1;

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

// ---------- companion ----------
static void homeDrawPet() {
  int moodLv = dg.mood < 30 ? 0 : dg.mood < 65 ? 1 : 2;
  int sig = !dgOk ? 1 << 20 : dg.stage | (dg.sleeping << 4) | (dg.sick << 5) | (moodLv << 6) | ((dg.stage == ST_EGG && dg.energy < 30) << 8);
  if (dgOk && dg.stage == ST_EGG) sig |= ((int)(dg.growth * 100 / dgNeed()) / 40) << 9;
  if (sig == hPetSig) return;
  hPetSig = sig;
  lv_obj_clean(hPetBob);
  if (!dgOk) {  // no card, no dragon: the V1 mascot face
    lv_obj_t *ring = shape(hPetBob, 33, 30, 170, 170, 0xA16BFF, LV_RADIUS_CIRCLE, LV_OPA_10);
    lv_obj_set_style_border_width(ring, 3, 0);
    lv_obj_set_style_border_color(ring, C_ACCENT2, 0);
    for (int i = -1; i <= 1; i += 2) {
      lv_obj_t *e = shape(hPetBob, 118 + 36 * i - 19, 84, 38, 62, 0x38E1FF, 19);
      lv_obj_set_style_bg_grad_color(e, C_ACCENT2, 0);
      lv_obj_set_style_bg_grad_dir(e, LV_GRAD_DIR_VER, 0);
      startAnim(e, animH, 62, 6, 120, 120, 3200, lv_anim_path_linear);
    }
    return;
  }
  if (dg.stage == ST_EGG) {
    dgDrawEgg(hPetBob, 236, 196, 1.3f, false);
  } else {
    float z = 150.0f / DRAGON_SPRITES[dg.stage - 1]->header.h;  // the home companion is always ~150 px tall
    dgDrawDragon(hPetBob, dg.stage, dg.sleeping, dg.sick, moodLv, 236, 196, z > 1.7f ? 1.7f : z, false);
  }
}

static const char *homeBubbleText(XzState st, int hour) {
  if (millis() < bannerUntil) return tr("Device linked to xiaozhi.me");
  if (listening) return tr("Listening...");
  if (st == XZ_CODE) return DL("Nhập mã bên phải ở xiaozhi.me nhé!", "Enter the code on the right at xiaozhi.me!");
  if (dgOk && dg.sleeping) return "Zzz...";
  if (!cfg.dnd && dgOk) {
    if (dg.stage == ST_EGG && dg.energy < 30) return DL("Ấp trứng cho ấm nhé!", "Keep the egg warm!");
    if (dg.sick) return DL("Mình bị ốm rồi...", "I feel sick...");
    if (dg.stage > ST_EGG && dg.satiety < 25) return DL("Mình đói bụng quá!", "I'm so hungry!");
    if (dg.stage > ST_EGG && dg.clean < 25) return DL("Cho mình đi tắm nhé!", "Can I have a bath?");
    if (dg.stage > ST_EGG && dg.energy < 25) return DL("Mình buồn ngủ quá...", "I'm sleepy...");
    if (dg.stage > ST_EGG && dg.mood < 25) return DL("Chơi với mình đi!", "Play with me!");
  }
  bool hint = (millis() / 8000) % 3 == 2;  // every third bubble reminds how to talk
  if (hint) {
    if (st == XZ_ACTIVATED) return DL("Chạm vào mình để trò chuyện", "Tap me to talk");
    if (dgOk) return DL("Giữ vào mình để chơi cùng", "Hold me to play");
  }
  WeatherData w = weatherGet();
  if (w.valid) {
    int c = w.code;
    if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82) || c >= 95) return DL("Trời mưa, nhớ mang ô nhé!", "It's raining - take an umbrella!");
    if (w.temp >= 34) return DL("Nóng quá, uống nhiều nước nhé!", "It's hot - drink plenty of water!");
    if (w.temp <= 15) return DL("Trời lạnh, mặc ấm vào nhé!", "It's cold - dress warmly!");
  }
  if (hour < 0) return DL("Xin chào!", "Hello!");
  if (hour >= 5 && hour < 10) return DL("Chúc bạn buổi sáng tốt lành!", "Have a lovely morning!");
  if (hour >= 11 && hour < 13) return DL("Đến giờ ăn trưa rồi!", "Lunch time!");
  if (hour >= 17 && hour < 21) return DL("Một ngày vất vả rồi, nghỉ ngơi nhé!", "Long day - time to relax!");
  if (hour >= 22 || hour < 5) return DL("Khuya rồi, đi ngủ thôi!", "It's late - time for bed!");
  return DL("Hôm nay bạn thế nào?", "How are you today?");
}

static void refreshWeather() {
  WeatherData w = weatherGet();
  int key = WK_NONE;
  if (!w.valid) {
    lv_label_set_text(wTemp, "--");
    lv_label_set_text(wDesc, tr(w.failed ? "Weather unavailable" : (wifiConnected() ? "Loading weather..." : "Waiting for network")));
    for (int i = 0; i < 3; i++) {
      lv_label_set_text(wHr[i], "");
      lv_label_set_text(wHt[i], "");
      lv_label_set_text(wHp[i], "");
    }
  } else {
    key = iconKindFor(w.code, w.isDay);
    lv_label_set_text_fmt(wTemp, "%d%s", tempOut(w.temp), DEG);
    lv_label_set_text_fmt(wDesc, "%s - %s", tr(weatherText(w.code)), w.city);
    for (int i = 0; i < 3; i++) {
      int idx = 2 + i * 2;  // +2h, +4h, +6h
      if (idx < w.nHours) {
        const WxHour &h = w.hours[idx];
        lv_label_set_text_fmt(wHr[i], "%dh", h.hour);
        lv_label_set_text_fmt(wHt[i], "%d%s", tempOut(h.temp), DEG);
        if (h.pop >= 20) lv_label_set_text_fmt(wHp[i], "%d%%", h.pop);
        else lv_label_set_text(wHp[i], "");
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
  XzState st = xiaozhiState();
  bool wifiOn = wifiConnected();
  if (st == XZ_CODE) {
    lv_label_set_text(lblCode, xiaozhiCode());
    lv_obj_add_flag(clockBox, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(codeBox, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(codeBox, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(clockBox, LV_OBJ_FLAG_HIDDEN);
  }

  struct tm t;
  bool timeOk = time(nullptr) > 1700000000 && getLocalTime(&t, 0);
  int hour = timeOk ? t.tm_hour : -1;
  if (timeOk) {
    int h = t.tm_hour;
    const char *g = h >= 5 && h < 12 ? "Good morning" : h < 18 && h >= 12 ? "Good afternoon" : h >= 18 && h < 22 ? "Good evening" : "Good night";
    if (!cfg.greetOn) lv_label_set_text(lblGreet, "");
    else if (cfg.userName.length()) lv_label_set_text_fmt(lblGreet, "%s, %s", tr(g), cfg.userName.c_str());
    else lv_label_set_text(lblGreet, tr(g));
    if (cfg.clock24) {
      lv_label_set_text_fmt(lblTime, "%02d:%02d", h, t.tm_min);
      lv_label_set_text(lblAmPm, "");
    } else {
      lv_label_set_text_fmt(lblTime, "%d:%02d", h % 12 ? h % 12 : 12, t.tm_min);
      lv_label_set_text(lblAmPm, isVi() ? (h < 12 ? "SA" : "CH") : (h < 12 ? "AM" : "PM"));
      lv_obj_align_to(lblAmPm, lblTime, LV_ALIGN_OUT_RIGHT_BOTTOM, 4, -8);
    }
    if (cfg.dateFmt == 2) lv_label_set_text(lblDate, "");
    else if (cfg.dateFmt == 1) lv_label_set_text_fmt(lblDate, "%02d/%02d/%d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
    else if (isVi()) lv_label_set_text_fmt(lblDate, "%s, %d tháng %d", DAYS_VI[t.tm_wday], t.tm_mday, t.tm_mon + 1);
    else lv_label_set_text_fmt(lblDate, "%s, %d %s", DAYS_EN[t.tm_wday], t.tm_mday, MONTHS_EN[t.tm_mon]);
    lv_label_set_text(lblLunar, cfg.lunarOn ? lunarText(t.tm_mday, t.tm_mon + 1, t.tm_year + 1900).c_str() : "");
    if (t.tm_yday != quoteDay || !cfg.quoteOn) {
      quoteDay = cfg.quoteOn ? t.tm_yday : -1;
      lv_label_set_text(hQuote, cfg.quoteOn ? quoteOfDay(t.tm_yday) : "");
    }
  } else {
    lv_label_set_text(lblGreet, cfg.greetOn ? tr("Hello") : "");
    lv_label_set_text(lblTime, "--:--");
    lv_label_set_text(lblAmPm, "");
    lv_label_set_text(lblDate, tr(wifiOn ? "Syncing time..." : "Waiting for network"));
    lv_label_set_text(lblLunar, "");
  }

  homeDrawPet();
  if (listening) lv_obj_clear_flag(hSpinner, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(hSpinner, LV_OBJ_FLAG_HIDDEN);
  const char *bt = homeBubbleText(st, hour);
  if (strcmp(bt, lv_label_get_text(hBubbleLbl)) != 0) {
    lv_point_t sz;  // a content-sized label never wraps, so cap its width when the text is long
    lv_txt_get_size(&sz, bt, F_LABEL, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_obj_set_width(hBubbleLbl, sz.x > 224 ? 224 : LV_SIZE_CONTENT);
    lv_label_set_text(hBubbleLbl, bt);
  }
  refreshWeather();
  int fx = fxWanted();
  if (fx != fxKind) fxStart(fx);
}

static void petTap(lv_event_t *) {
  if (xiaozhiState() == XZ_ACTIVATED) {
    listening = !listening;
    soundPlay(SND(SND_CHIRP), 0, false);
    refreshHome();
  } else if (dgOk) {
    pageDragon();
  }
}
static void petHold(lv_event_t *) {
  if (!dgOk) return;
  soundPlay(SND(SND_CHIRP), 0, false);
  pageDragon();
}

static void homeTick(lv_timer_t *) {
  static int lastState = -1;
  XzState st = xiaozhiState();
  if (st == XZ_ACTIVATED && lastState != XZ_ACTIVATED && lastState != -1) bannerUntil = millis() + 4000;
  lastState = st;
  refreshHome();
}

static void buildHome(lv_obj_t *tile) {
  hFx = lv_obj_create(tile);  // weather effects, behind everything
  lv_obj_remove_style_all(hFx);
  lv_obj_set_size(hFx, 480, 320);
  lv_obj_clear_flag(hFx, LV_OBJ_FLAG_CLICKABLE);

  // companion
  hPet = lv_obj_create(tile);
  lv_obj_remove_style_all(hPet);
  lv_obj_set_size(hPet, 236, 232);
  lv_obj_set_pos(hPet, 6, SB_H + 2);
  lv_obj_add_flag(hPet, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(hPet, petTap, LV_EVENT_SHORT_CLICKED, nullptr);
  lv_obj_add_event_cb(hPet, petHold, LV_EVENT_LONG_PRESSED, nullptr);
  hSpinner = lv_spinner_create(hPet, 1000, 70);
  lv_obj_set_size(hSpinner, 200, 200);
  lv_obj_set_pos(hSpinner, 18, 30);
  lv_obj_set_style_arc_color(hSpinner, C_OK, LV_PART_INDICATOR);
  lv_obj_set_style_arc_opa(hSpinner, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_arc_width(hSpinner, 5, LV_PART_INDICATOR);
  lv_obj_add_flag(hSpinner, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(hSpinner, LV_OBJ_FLAG_CLICKABLE);
  hPetBob = lv_obj_create(hPet);
  lv_obj_remove_style_all(hPetBob);
  lv_obj_set_size(hPetBob, 236, 196);
  lv_obj_set_pos(hPetBob, 0, 26);
  lv_obj_clear_flag(hPetBob, LV_OBJ_FLAG_CLICKABLE);
  lv_timer_create(  // breathing: two positions, so the zoomed sprite is redrawn only twice per breath
      [](lv_timer_t *) {
        static bool up = false;
        up = !up;
        if (homeIsActive()) lv_obj_set_y(hPetBob, up ? 23 : 26);
      },
      1300, nullptr);

  // the dragon's line: white text on one line, scrolling left when it is too long
  hBubble = lv_obj_create(hPet);
  lv_obj_remove_style_all(hBubble);
  lv_obj_set_size(hBubble, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_pos(hBubble, 8, 4);
  lv_obj_clear_flag(hBubble, LV_OBJ_FLAG_CLICKABLE);
  hBubbleLbl = makeLabel(hBubble, "", F_LABEL, C_TEXT);
  lv_label_set_long_mode(hBubbleLbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_style_anim_speed(hBubbleLbl, 30, 0);

  // right column: clock
  clockBox = lv_obj_create(tile);
  lv_obj_remove_style_all(clockBox);
  lv_obj_set_size(clockBox, 228, 120);
  lv_obj_set_pos(clockBox, 248, SB_H + 2);
  lv_obj_clear_flag(clockBox, LV_OBJ_FLAG_CLICKABLE);
  lblGreet = makeLabel(clockBox, "", F_LABEL, lv_color_hex(0xC9D2F2));
  lblTime = makeLabel(clockBox, "--:--", F_DISPLAY, C_TEXT);
  lv_obj_set_pos(lblTime, 0, 18);
  lblAmPm = makeLabel(clockBox, "", F_LABEL, C_ACCENT);
  lblDate = makeLabel(clockBox, "", F_LABEL, C_ACCENT);
  lv_obj_set_pos(lblDate, 0, 74);
  lblLunar = makeLabel(clockBox, "", F_BODY, lv_color_hex(0xD9C8FF));
  lv_obj_set_pos(lblLunar, 0, 96);

  // right column: pairing code (only while pairing)
  codeBox = lv_obj_create(tile);
  lv_obj_remove_style_all(codeBox);
  lv_obj_set_size(codeBox, 228, 120);
  lv_obj_set_pos(codeBox, 248, SB_H + 2);
  lv_obj_t *ct = makeLabel(codeBox, "Pair this device", F_LABEL, C_MUTED);
  (void)ct;
  lblCode = makeLabel(codeBox, "------", F_DISPLAY, C_ACCENT);
  lv_obj_set_style_text_letter_space(lblCode, 3, 0);
  lv_obj_set_pos(lblCode, 0, 22);
  lv_obj_t *ch = makeLabel(codeBox, "Enter this code at xiaozhi.me > Add device", F_CAPTION, C_MUTED);
  lv_obj_set_width(ch, 228);
  lv_label_set_long_mode(ch, LV_LABEL_LONG_WRAP);
  lv_obj_set_pos(ch, 0, 80);
  lv_obj_add_flag(codeBox, LV_OBJ_FLAG_HIDDEN);

  // weather card: now + the next hours
  wCard = makeCard(tile, 226, 104);
  lv_obj_set_pos(wCard, 248, SB_H + 126);
  lv_obj_set_style_bg_opa(wCard, LV_OPA_80, 0);
  lv_obj_add_flag(wCard, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_color(wCard, C_ACCENT, LV_STATE_PRESSED);
  lv_obj_add_event_cb(wCard, [](lv_event_t *) { pageWeather(); }, LV_EVENT_CLICKED, nullptr);
  wIcon = wxIconBox(wCard, WK_NONE);
  lv_obj_set_pos(wIcon, 8, 6);
  wTemp = makeLabel(wCard, "--", F_HEADING, C_TEXT);
  lv_obj_set_pos(wTemp, 60, 4);
  wDesc = makeLabel(wCard, "", F_CAPTION, C_MUTED);
  lv_obj_set_width(wDesc, 158);
  lv_label_set_long_mode(wDesc, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(wDesc, 60, 36);
  shape(wCard, 10, 58, 206, 1, 0x2A3359, 0);
  for (int i = 0; i < 3; i++) {
    int x = 10 + i * 72;
    wHr[i] = makeLabel(wCard, "", F_CAPTION, C_MUTED);
    lv_obj_set_pos(wHr[i], x, 64);
    wHt[i] = makeLabel(wCard, "", F_LABEL, C_TEXT);
    lv_obj_set_pos(wHt[i], x, 80);
    wHp[i] = makeLabel(wCard, "", F_CAPTION, lv_color_hex(0x7FB8FF));
    lv_obj_set_pos(wHp[i], x + 34, 64);
  }

  // quote of the day
  hQuote = makeLabel(tile, "", F_BODY, lv_color_hex(0xD4DAF2));
  lv_obj_set_width(hQuote, 456);
  lv_label_set_long_mode(hQuote, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_style_anim_speed(hQuote, 25, 0);
  lv_obj_set_style_text_align(hQuote, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(hQuote, 12, 262);
}
