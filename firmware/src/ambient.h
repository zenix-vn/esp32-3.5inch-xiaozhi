#pragma once
// Ambient screen: a calm, minimal clock face shown after the idle time, on the nightly schedule or from the control
// center. Black background; the date and the lunar date on top, a big thin clock (Outfit ExtraLight, ambient_font.c)
// in the middle with the home screen pet asleep across it, like the depth effect of a phone lock screen: it faces
// left with its head behind the hours, and its tail over the minutes, and three rounded chips underneath: weather, the next alarm (or a running timer) and the nearest
// countdown. At night everything turns a warm amber. Redraws only when the minute changes; the whole face drifts a
// few pixels every minute (no burn-in) and the backlight is dimmed (more at night). Any touch goes back.
// Included by main.cpp after home.h and clock_app.h.

static lv_obj_t *ambScr = nullptr, *ambPrev = nullptr, *ambBox;
// Layers, bottom to top: minutes, pet, hours, colon (separate objects, placed in ambLayout()).
static lv_obj_t *ambHH, *ambMid, *ambMM, *ambAmPm, *ambDate, *ambLunar, *ambPet;  // ambMid: the colon
static const int AMB_CLOCK_Y = 90, AMB_PET_W = 184;  // clock top; the sleeping pet's drawn width
// LVGL cannot zoom an indexed image, so while the ambient screen is up the pet lives in RAM as true colour + alpha
// (RGB565 low byte, high byte, alpha: ~23 KB), mirrored to face left, freed on exit.
static uint8_t *ambPetBuf = nullptr;
static lv_img_dsc_t ambPetImg;

static const lv_img_dsc_t *ambPetTrueColor(const lv_img_dsc_t *src) {
  int w = src->header.w, h = src->header.h;
  ambPetBuf = (uint8_t *)malloc(w * h * 3);
  if (!ambPetBuf) return nullptr;
  const uint8_t *pal = src->data, *px = src->data + 1024;  // palette: blue, green, red, alpha
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      const uint8_t *c = pal + px[y * w + (w - 1 - x)] * 4;
      uint16_t v = ((c[2] >> 3) << 11) | ((c[1] >> 2) << 5) | (c[0] >> 3);
      uint8_t *o = ambPetBuf + (y * w + x) * 3;
      o[0] = v & 255;
      o[1] = v >> 8;
      o[2] = c[3];
    }
  ambPetImg = *src;
  ambPetImg.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
  ambPetImg.data_size = w * h * 3;
  ambPetImg.data = ambPetBuf;
  return &ambPetImg;
}
static lv_obj_t *ambChip[3], *ambChipIcon[3], *ambChipVal[3], *ambChipCap[3];
static int ambMinute = -1, ambWxKey = -1, ambNight = -1;

struct AmbTint {
  uint32_t text, sub, accent, chipBg, chipLine;
};
static const AmbTint AMB_DAY = {0xF2F5FF, 0x8A93B8, 0x38E1FF, 0x0E1120, 0x1F2538};
static const AmbTint AMB_NIGHT = {0xFFB27A, 0x9A6B4A, 0xFF7A3D, 0x140C07, 0x2C1B10};  // easy on the eyes in the dark
static const AmbTint &ambTint() { return ambNight == 1 ? AMB_NIGHT : AMB_DAY; }

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
  if (ambPetBuf) {
    lv_img_cache_invalidate_src(&ambPetImg);
    free(ambPetBuf);
    ambPetBuf = nullptr;
  }
  lv_disp_trig_activity(nullptr);
}

static void ambApplyTint() {
  const AmbTint &c = ambTint();
  lv_obj_set_style_text_color(ambHH, lv_color_hex(c.text), 0);
  lv_obj_set_style_text_color(ambMM, lv_color_hex(c.text), 0);
  lv_obj_set_style_text_color(ambMid, lv_color_hex(c.accent), 0);
  if (ambPet) lv_obj_set_style_img_opa(ambPet, ambNight == 1 ? LV_OPA_70 : LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(ambAmPm, lv_color_hex(c.accent), 0);
  lv_obj_set_style_text_color(ambDate, lv_color_hex(c.text), 0);
  lv_obj_set_style_text_color(ambLunar, lv_color_hex(c.sub), 0);
  for (int i = 0; i < 3; i++) {
    lv_obj_set_style_bg_color(ambChip[i], lv_color_hex(c.chipBg), 0);
    lv_obj_set_style_border_color(ambChip[i], lv_color_hex(c.chipLine), 0);
    lv_obj_set_style_text_color(ambChipVal[i], lv_color_hex(c.text), 0);
    lv_obj_set_style_text_color(ambChipCap[i], lv_color_hex(c.sub), 0);
    if (i) lv_obj_set_style_text_color(ambChipIcon[i], lv_color_hex(c.accent), 0);
  }
}

// chip i: value (one line, bold) over a caption
static void ambChipSet(int i, const char *val, const char *cap) {
  setText(ambChipVal[i], val);
  setText(ambChipCap[i], cap);
}

static void ambRefreshWeather() {
  WeatherData w = weatherGet();
  if (!w.valid) {
    ambChipSet(0, "--", tr(wifiConnected() ? "Loading weather..." : "Waiting for network"));
    return;
  }
  int key = iconKindFor(w.code, w.isDay);
  if (key != ambWxKey) {
    drawWxIcon(ambChipIcon[0], key);
    ambWxKey = key;
  }
  char v[16], c[40];
  snprintf(v, sizeof(v), "%d%s", tempOut(w.temp), DEG);
  if (w.nDays) snprintf(c, sizeof(c), "%d%s / %d%s", tempOut(w.days[0].tmax), DEG, tempOut(w.days[0].tmin), DEG);
  else strlcpy(c, tr(weatherText(w.code)), sizeof(c));
  ambChipSet(0, v, c);
}

static void ambRefreshAlarm() {
  char v[24], c[40];
  if (tmrEnd) {  // a running timer comes first: when it ends
    time_t end = time(nullptr) + (tmrEnd - millis()) / 1000;
    struct tm e;
    localtime_r(&end, &e);
    snprintf(v, sizeof(v), "%02d:%02d", e.tm_hour, e.tm_min);
    setText(ambChipIcon[1], LV_SYMBOL_LOOP);
    ambChipSet(1, v, DL("Hẹn giờ xong", "Timer ends"));
    return;
  }
  setText(ambChipIcon[1], LV_SYMBOL_BELL);
  int away = 0, next = alarmNext(&away);
  if (next < 0) {
    ambChipSet(1, "--:--", DL("Không báo thức", "No alarm"));
    return;
  }
  alarmTimeText(alarms[next].h, alarms[next].m, v, sizeof(v));
  alarmAwayText(away, c, sizeof(c));
  ambChipSet(1, v, c);
}

static void ambRefreshCountdown() {
  int ev[12];
  int n = cdHomeList(ev);
  if (!wifiConnected()) {  // being offline matters more: the time and weather stop updating
    setText(ambChipIcon[2], LV_SYMBOL_WARNING);
    ambChipSet(2, "Wi-Fi", DL("Mất kết nối", "Offline"));
    return;
  }
  if (!n) {
    setText(ambChipIcon[2], LV_SYMBOL_WIFI);
    ambChipSet(2, "Wi-Fi", DL("Đã kết nối", "Connected"));
    return;
  }
  long days = cdDaysLeft(ev[0]);
  char v[24];
  if (days == 0) strlcpy(v, DL("Hôm nay", "Today"), sizeof(v));
  else snprintf(v, sizeof(v), isVi() ? "%ld ngày" : "%ld days", days);
  setText(ambChipIcon[2], LV_SYMBOL_LIST);
  lv_obj_set_style_text_color(ambChipIcon[2], lv_color_hex(ambNight == 1 ? 0xFF9A6A : 0xFF9BC8), 0);  // pink, as on home
  ambChipSet(2, v, cdName(ev[0]));
}

// Hours, colon and minutes centred in a row; the pet lies across the colon, its paws a little below the digits' feet.
static void ambLayout() {
  lv_obj_update_layout(ambBox);
  int wh = lv_obj_get_width(ambHH), wc = lv_obj_get_width(ambMid), wm = lv_obj_get_width(ambMM), gap = 4;
  int x = (480 - (wh + wc + wm + 2 * gap)) / 2, lh = lv_font_get_line_height(&ambient_font);
  lv_obj_set_pos(ambHH, x, AMB_CLOCK_Y);
  lv_obj_set_pos(ambMid, x + wh + gap, AMB_CLOCK_Y);
  lv_obj_set_pos(ambMM, x + wh + gap + wc + gap, AMB_CLOCK_Y);
  lv_obj_align_to(ambAmPm, ambMM, LV_ALIGN_OUT_RIGHT_TOP, 6, 8);
  if (ambPet) {
    const lv_img_dsc_t *d = (const lv_img_dsc_t *)lv_img_get_src(ambPet);
    int zh = d->header.h * AMB_PET_W / d->header.w;  // drawn height (zoomed around the centre)
    int cx = x + wh + gap + wc / 2, cy = AMB_CLOCK_Y + lh + 6 - zh / 2;
    lv_obj_set_pos(ambPet, cx - d->header.w / 2, cy - d->header.h / 2);
  }
}

static void ambientRefresh(lv_timer_t *) {
  if (!ambScr) return;
  struct tm t;
  bool ok = time(nullptr) > 1700000000 && getLocalTime(&t, 0);
  int minute = ok ? t.tm_hour * 60 + t.tm_min : -2;
  if (minute == ambMinute) return;  // nothing visible changes within a minute
  ambMinute = minute;
  int night = themeSlotNow() == 3 ? 1 : 0;
  if (night != ambNight) {
    ambNight = night;
    ambApplyTint();
  }
  if (ok) {
    int h = cfg.clock24 ? t.tm_hour : (t.tm_hour % 12 ? t.tm_hour % 12 : 12);
    setTextFmt(ambHH, cfg.clock24 ? "%02d" : "%d", h);
    setTextFmt(ambMM, "%02d", t.tm_min);
    setText(ambAmPm, cfg.clock24 ? "" : isVi() ? (t.tm_hour < 12 ? "SA" : "CH") : (t.tm_hour < 12 ? "AM" : "PM"));
    if (isVi()) setTextFmt(ambDate, "%s, %d tháng %d", DAYS_VI[t.tm_wday], t.tm_mday, t.tm_mon + 1);
    else setTextFmt(ambDate, "%s, %d %s", DAYS_EN[t.tm_wday], t.tm_mday, MONTHS_EN[t.tm_mon]);
    setText(ambLunar, cfg.lunarOn ? lunarText(t.tm_mday, t.tm_mon + 1, t.tm_year + 1900).c_str() : "");
    int dx = (minute * 7) % 13 - 6, dy = (minute * 5) % 11 - 5;  // drift
    lv_obj_set_pos(ambBox, dx, dy);
  } else {
    setText(ambHH, "--");
    setText(ambMM, "--");
    setText(ambAmPm, "");
    setText(ambDate, tr("Syncing time..."));
    setText(ambLunar, "");
  }
  ambLayout();
  ambRefreshWeather();
  ambRefreshAlarm();
  ambRefreshCountdown();
  ambDim();
}

static void ambientShow() {
  if (ambOn) return;
  ambOn = true;
  ambPrev = lv_scr_act();
  ambMinute = ambWxKey = ambNight = -1;
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

  // top: date and lunar date, centred
  ambDate = makeLabel(ambBox, "", F_TITLE, lv_color_hex(0xF2F5FF));
  lv_obj_set_width(ambDate, 440);
  lv_obj_set_style_text_align(ambDate, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_letter_space(ambDate, 1, 0);
  lv_obj_align(ambDate, LV_ALIGN_TOP_MID, 0, 14);
  ambLunar = makeLabel(ambBox, "", F_BODY, lv_color_hex(0x8A93B8));
  lv_obj_set_width(ambLunar, 440);
  lv_obj_set_style_text_align(ambLunar, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(ambLunar, LV_ALIGN_TOP_MID, 0, 40);

  // middle: the clock
  // middle: minutes, then the pet, then hours and colon on top (creation order is the drawing order)
  ambMM = makeLabel(ambBox, "--", &ambient_font, lv_color_hex(0xF2F5FF));
  ambPet = nullptr;
  const lv_img_dsc_t *nap = cfg.pet >= 1 && cfg.pet <= PET_N ? ambPetTrueColor(PET_NAP[cfg.pet - 1]) : nullptr;
  if (nap) {
    ambPet = lv_img_create(ambBox);
    lv_img_set_src(ambPet, nap);
    lv_img_set_zoom(ambPet, AMB_PET_W * 256 / nap->header.w);
    lv_img_set_antialias(ambPet, true);
    lv_obj_clear_flag(ambPet, LV_OBJ_FLAG_CLICKABLE);
  }
  ambHH = makeLabel(ambBox, "--", &ambient_font, lv_color_hex(0xF2F5FF));
  ambMid = makeLabel(ambBox, ":", &ambient_font, lv_color_hex(0x38E1FF));
  ambAmPm = makeLabel(ambBox, "", F_TITLE, lv_color_hex(0x38E1FF));

  // bottom: three chips
  for (int i = 0; i < 3; i++) {
    lv_obj_t *c = lv_obj_create(ambBox);
    lv_obj_remove_style_all(c);
    lv_obj_set_size(c, 142, 58);
    lv_obj_set_pos(c, 18 + i * 150, 238);
    lv_obj_set_style_radius(c, 20, 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    ambChip[i] = c;
    if (i == 0) {
      ambChipIcon[0] = wxIconBox(c, WK_NONE);
      lv_obj_set_pos(ambChipIcon[0], 6, 7);
    } else {
      ambChipIcon[i] = makeLabel(c, LV_SYMBOL_BELL, F_TITLE, lv_color_hex(0x38E1FF));
      lv_obj_set_width(ambChipIcon[i], 44);
      lv_obj_set_style_text_align(ambChipIcon[i], LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_align(ambChipIcon[i], LV_ALIGN_LEFT_MID, 6, 0);
    }
    ambChipVal[i] = makeLabel(c, "", F_LABEL, lv_color_hex(0xF2F5FF));
    lv_obj_set_size(ambChipVal[i], 86, 20);
    lv_label_set_long_mode(ambChipVal[i], LV_LABEL_LONG_DOT);
    lv_obj_set_pos(ambChipVal[i], 52, 9);
    ambChipCap[i] = makeLabel(c, "", F_CAPTION, lv_color_hex(0x8A93B8));
    lv_obj_set_size(ambChipCap[i], 86, 16);
    lv_label_set_long_mode(ambChipCap[i], LV_LABEL_LONG_DOT);
    lv_obj_set_pos(ambChipCap[i], 52, 32);
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
