#pragma once
// Ambient screen: a calm, information-rich clock face shown after the idle time, on the nightly schedule or
// from the control center. Left: big clock, date, lunar date. Right: weather now, today's high/low and the
// next hours. Bottom: the sleeping dragon and its state, alarm / timer, offline warning, quote of the day.
// Redraws only when the minute changes; the whole face drifts a few pixels every minute (no burn-in) and the
// backlight is dimmed (more at night). Any touch goes back to the previous screen.
// Included by main.cpp after home.h and clock_app.h.

static lv_obj_t *ambScr = nullptr, *ambPrev = nullptr, *ambBox;
static lv_obj_t *ambTime, *ambAmPm, *ambDate, *ambLunar;
static lv_obj_t *ambWxIcon, *ambTemp, *ambDesc, *ambHiLo, *ambHr[3], *ambHt[3];
static lv_obj_t *ambPet, *ambPetLbl, *ambInfo, *ambQuote;
static int ambMinute = -1, ambWxKey = -1, ambPetSig = -1;

static void ambDim() {
  int h = themeSlotNow() == 3 ? 8 : 15;  // percent of the normal brightness; darker at night
  int v = blLevel * h / 100;
  ledcWrite(0, v < 6 ? 6 : v);
}

static void ambientExit() {
  if (!ambOn) return;
  ambOn = false;
  ledcWrite(0, blLevel);
  statusbarShow(true);
  lv_obj_t *old = ambScr;
  ambScr = nullptr;
  lv_scr_load(ambPrev && lv_obj_is_valid(ambPrev) ? ambPrev : mainScr);
  lv_obj_del(old);
  lv_disp_trig_activity(nullptr);
}

static void ambRefreshPet() {
  if (!dgOk) return;
  int sig = dg.stage | (dg.sick << 4) | ((dg.stage == ST_EGG && dg.energy < 30) << 5);
  if (sig != ambPetSig) {
    ambPetSig = sig;
    lv_obj_clean(ambPet);
    if (dg.stage == ST_EGG) dgDrawEgg(ambPet, 120, 92, 2, false);
    else dgDrawDragon(ambPet, dg.stage, true, dg.sick, 1, 120, 92, 2, false);
  }
  const char *al = dgAlert();
  String n = dgName();
  if (al) {
    lv_label_set_text(ambPetLbl, al);
    lv_obj_set_style_text_color(ambPetLbl, lv_color_hex(0xFFB84A), 0);
  } else {
    if (dg.stage == ST_EGG) lv_label_set_text(ambPetLbl, DL("Quả trứng đang được ấp ấm", "The egg is keeping warm"));
    else lv_label_set_text_fmt(ambPetLbl, isVi() ? "%s đang ngủ ngon" : "%s is fast asleep", n.c_str());
    lv_obj_set_style_text_color(ambPetLbl, lv_color_hex(0xB7C3EE), 0);
  }
}

static void ambRefreshInfo(const struct tm *t) {
  String s;
  if (cfg.alarmOn) {
    char b[48];
    snprintf(b, sizeof(b), isVi() ? LV_SYMBOL_BELL "  Báo thức %02d:%02d" : LV_SYMBOL_BELL "  Alarm %02d:%02d", cfg.alarmH, cfg.alarmM);
    s += b;
  }
  if (tmrEnd && t) {  // running timer: when it ends
    time_t end = time(nullptr) + (tmrEnd - millis()) / 1000;
    struct tm e;
    localtime_r(&end, &e);
    char b[48];
    snprintf(b, sizeof(b), isVi() ? LV_SYMBOL_LOOP "  Hẹn giờ xong lúc %02d:%02d" : LV_SYMBOL_LOOP "  Timer ends %02d:%02d", e.tm_hour, e.tm_min);
    if (s.length()) s += "     ";
    s += b;
  }
  if (!wifiConnected()) {
    if (s.length()) s += "     ";
    s += LV_SYMBOL_WARNING;
    s += DL("  Mất Wi-Fi", "  Wi-Fi offline");
  }
  if (!s.length()) s = DL("Chạm vào màn hình để quay lại", "Touch the screen to go back");
  lv_label_set_text(ambInfo, s.c_str());
}

static void ambRefreshWeather() {
  WeatherData w = weatherGet();
  if (!w.valid) {
    lv_label_set_text(ambTemp, "--");
    lv_label_set_text(ambDesc, tr(wifiConnected() ? "Loading weather..." : "Waiting for network"));
    lv_label_set_text(ambHiLo, "");
    for (int i = 0; i < 3; i++) {
      lv_label_set_text(ambHr[i], "");
      lv_label_set_text(ambHt[i], "");
    }
    return;
  }
  int key = iconKindFor(w.code, w.isDay);
  if (key != ambWxKey) {
    drawWxIcon(ambWxIcon, key);
    ambWxKey = key;
  }
  lv_label_set_text_fmt(ambTemp, "%d%s", tempOut(w.temp), DEG);
  lv_label_set_text_fmt(ambDesc, "%s  -  %s", tr(weatherText(w.code)), w.city);
  if (w.nDays) {
    const WxDay &d = w.days[0];
    char rain[24] = "";
    if (d.pop >= 30) snprintf(rain, sizeof(rain), isVi() ? "   mưa %d%%" : "   rain %d%%", d.pop);
    lv_label_set_text_fmt(ambHiLo, LV_SYMBOL_UP " %d%s   " LV_SYMBOL_DOWN " %d%s%s", tempOut(d.tmax), DEG, tempOut(d.tmin), DEG, rain);
  }
  for (int i = 0; i < 3; i++) {
    int idx = 2 + i * 2;
    if (idx < w.nHours) {
      lv_label_set_text_fmt(ambHr[i], "%dh", w.hours[idx].hour);
      lv_label_set_text_fmt(ambHt[i], "%d%s", tempOut(w.hours[idx].temp), DEG);
    }
  }
}

static void ambientRefresh(lv_timer_t *) {
  if (!ambScr) return;
  struct tm t;
  bool ok = time(nullptr) > 1700000000 && getLocalTime(&t, 0);
  int minute = ok ? t.tm_hour * 60 + t.tm_min : -2;
  if (minute == ambMinute) return;  // nothing visible changes within a minute
  ambMinute = minute;
  if (ok) {
    if (cfg.clock24) {
      lv_label_set_text_fmt(ambTime, "%02d:%02d", t.tm_hour, t.tm_min);
      lv_label_set_text(ambAmPm, "");
    } else {
      lv_label_set_text_fmt(ambTime, "%d:%02d", t.tm_hour % 12 ? t.tm_hour % 12 : 12, t.tm_min);
      lv_label_set_text(ambAmPm, isVi() ? (t.tm_hour < 12 ? "SA" : "CH") : (t.tm_hour < 12 ? "AM" : "PM"));
      lv_obj_align_to(ambAmPm, ambTime, LV_ALIGN_OUT_RIGHT_BOTTOM, 6, -18);
    }
    if (isVi()) lv_label_set_text_fmt(ambDate, "%s, %d tháng %d", DAYS_VI[t.tm_wday], t.tm_mday, t.tm_mon + 1);
    else lv_label_set_text_fmt(ambDate, "%s, %d %s", DAYS_EN[t.tm_wday], t.tm_mday, MONTHS_EN[t.tm_mon]);
    lv_label_set_text(ambLunar, lunarText(t.tm_mday, t.tm_mon + 1, t.tm_year + 1900).c_str());
    lv_label_set_text(ambQuote, quoteOfDay(t.tm_yday));
    int dx = (minute * 7) % 13 - 6, dy = (minute * 5) % 11 - 5;  // drift
    lv_obj_set_pos(ambBox, dx, dy);
  } else {
    lv_label_set_text(ambTime, "--:--");
    lv_label_set_text(ambAmPm, "");
    lv_label_set_text(ambDate, tr("Syncing time..."));
    lv_label_set_text(ambLunar, "");
  }
  ambRefreshWeather();
  ambRefreshPet();
  ambRefreshInfo(ok ? &t : nullptr);
  ambDim();
}

static void ambientShow() {
  if (ambOn) return;
  ambOn = true;
  ambPrev = lv_scr_act();
  ambMinute = ambWxKey = ambPetSig = -1;
  ambScr = lv_obj_create(nullptr);
  lv_obj_remove_style_all(ambScr);
  lv_obj_set_style_bg_color(ambScr, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(ambScr, LV_OPA_COVER, 0);
  lv_obj_add_flag(ambScr, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(ambScr, [](lv_event_t *) { ambientExit(); }, LV_EVENT_CLICKED, nullptr);

  ambBox = lv_obj_create(ambScr);
  lv_obj_remove_style_all(ambBox);
  lv_obj_set_size(ambBox, 480, 320);
  lv_obj_clear_flag(ambBox, LV_OBJ_FLAG_CLICKABLE);

  // left: clock, date, lunar date
  ambTime = makeLabel(ambBox, "--:--", &vi_clock_84, lv_color_hex(0xEEF2FF));
  lv_obj_set_pos(ambTime, 18, 34);
  ambAmPm = makeLabel(ambBox, "", F_TITLE, lv_color_hex(0x8FB4FF));
  lv_obj_set_pos(ambAmPm, 250, 70);
  ambDate = makeLabel(ambBox, "", F_TITLE, lv_color_hex(0x8FB4FF));
  lv_obj_set_pos(ambDate, 26, 112);
  ambLunar = makeLabel(ambBox, "", F_BODY, lv_color_hex(0xC2B2F5));
  lv_obj_set_pos(ambLunar, 26, 140);

  // right: weather
  ambWxIcon = wxIconBox(ambBox, WK_NONE);
  lv_obj_set_pos(ambWxIcon, 306, 22);
  ambTemp = makeLabel(ambBox, "--", F_HEADING, lv_color_hex(0xEEF2FF));
  lv_obj_set_pos(ambTemp, 358, 24);
  ambDesc = makeLabel(ambBox, "", F_BODY, lv_color_hex(0xB7C3EE));
  lv_obj_set_width(ambDesc, 166);
  lv_label_set_long_mode(ambDesc, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(ambDesc, 306, 74);
  ambHiLo = makeLabel(ambBox, "", F_CAPTION, lv_color_hex(0x8C96C0));
  lv_obj_set_width(ambHiLo, 166);
  lv_label_set_long_mode(ambHiLo, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(ambHiLo, 306, 96);
  for (int i = 0; i < 3; i++) {
    ambHr[i] = makeLabel(ambBox, "", F_CAPTION, lv_color_hex(0x8C96C0));
    lv_obj_set_pos(ambHr[i], 306 + i * 56, 130);
    ambHt[i] = makeLabel(ambBox, "", F_LABEL, lv_color_hex(0xDDE3FF));
    lv_obj_set_pos(ambHt[i], 306 + i * 56, 146);
  }

  shape(ambBox, 24, 180, 432, 1, 0x252C4A, 0);  // divider

  // bottom: dragon, its state, alarm / timer, quote
  ambPet = lv_obj_create(ambBox);
  lv_obj_remove_style_all(ambPet);
  lv_obj_set_size(ambPet, 120, 92);
  lv_obj_set_pos(ambPet, 14, 196);
  lv_obj_clear_flag(ambPet, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_opa(ambPet, LV_OPA_80, 0);
  ambPetLbl = makeLabel(ambBox, "", F_LABEL, lv_color_hex(0xB7C3EE));
  lv_obj_set_width(ambPetLbl, 320);
  lv_label_set_long_mode(ambPetLbl, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(ambPetLbl, 146, 196);
  ambInfo = makeLabel(ambBox, "", F_BODY, lv_color_hex(0x7FD8C4));
  lv_obj_set_width(ambInfo, 320);
  lv_label_set_long_mode(ambInfo, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(ambInfo, 146, 222);
  ambQuote = makeLabel(ambBox, "", F_BODY, lv_color_hex(0x7F89B5));
  lv_obj_set_width(ambQuote, 320);
  lv_label_set_long_mode(ambQuote, LV_LABEL_LONG_WRAP);
  lv_obj_set_pos(ambQuote, 146, 248);
  if (!dgOk) {  // no dragon: the quote takes the whole bottom row
    lv_obj_add_flag(ambPetLbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(ambInfo, 26, 196);
    lv_obj_set_pos(ambQuote, 26, 228);
    lv_obj_set_width(ambInfo, 430);
    lv_obj_set_width(ambQuote, 430);
  }

  pageTimer(ambScr, ambientRefresh, 2000);
  ambientRefresh(nullptr);
  statusbarShow(false);
  if (ccOv) ccClose();
  lv_scr_load(ambScr);
  ambDim();
}

// Nightly schedule: enter ambient after a minute without touches inside [ambStart, ambEnd).
static void ambientTick() {
  if (ambOn || cfg.ambStart < 0) return;
  struct tm t;
  if (time(nullptr) < 1700000000 || !getLocalTime(&t, 0)) return;
  int h = t.tm_hour, s = cfg.ambStart, e = cfg.ambEnd;
  bool inside = s <= e ? (h >= s && h < e) : (h >= s || h < e);
  if (inside && lv_disp_get_inactive_time(nullptr) > 60000) ambientShow();
}
