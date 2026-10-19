#pragma once
// Ambient screen (V2.3): big clock, date, temperature and the dragon asleep, at low backlight.
// Opens after the idle time (when "idle mode" is ambient), on the nightly schedule, or from the
// control center. Any touch goes back to the previous screen. Redraws once every 20 s.

static lv_obj_t *ambScr = nullptr, *ambPrev = nullptr, *ambBox, *ambTime, *ambDate, *ambWx;
static bool ambOn = false;

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

static void ambientRefresh(lv_timer_t *) {
  if (!ambScr) return;
  static const char *daysVi[] = {"Chủ Nhật", "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy"};
  static const char *daysEn[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  struct tm t;
  if (time(nullptr) > 1700000000 && getLocalTime(&t, 0)) {
    if (cfg.clock24) lv_label_set_text_fmt(ambTime, "%02d:%02d", t.tm_hour, t.tm_min);
    else lv_label_set_text_fmt(ambTime, "%d:%02d", t.tm_hour % 12 ? t.tm_hour % 12 : 12, t.tm_min);
    lv_label_set_text_fmt(ambDate, "%s, %02d/%02d", isVi() ? daysVi[t.tm_wday] : daysEn[t.tm_wday], t.tm_mday, t.tm_mon + 1);
    // drift a few pixels every minute so nothing stays in one place all night
    int m = t.tm_hour * 60 + t.tm_min;
    lv_obj_set_pos(ambBox, 40 + (m * 7) % 40, 20 + (m * 11) % 30);
  } else {
    lv_label_set_text(ambTime, "--:--");
    lv_label_set_text(ambDate, "");
  }
  WeatherData w = weatherGet();
  if (w.valid) lv_label_set_text_fmt(ambWx, "%d%s  %s", (int)lroundf(cfg.tempF ? w.temp * 9 / 5 + 32 : w.temp), cfg.tempF ? "°F" : "°C", tr(weatherText(w.code)));
  else lv_label_set_text(ambWx, "");
}

static void ambientShow() {
  if (ambOn) return;
  ambOn = true;
  ambPrev = lv_scr_act();
  ambScr = lv_obj_create(nullptr);
  lv_obj_remove_style_all(ambScr);
  lv_obj_set_style_bg_color(ambScr, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(ambScr, LV_OPA_COVER, 0);
  lv_obj_add_flag(ambScr, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(ambScr, [](lv_event_t *) { ambientExit(); }, LV_EVENT_CLICKED, nullptr);

  ambBox = lv_obj_create(ambScr);
  lv_obj_remove_style_all(ambBox);
  lv_obj_set_size(ambBox, 400, 270);
  lv_obj_clear_flag(ambBox, LV_OBJ_FLAG_CLICKABLE);
  ambTime = makeLabel(ambBox, "--:--", F_DISPLAY, lv_color_hex(0xC8D0F0));
  lv_obj_set_style_transform_zoom(ambTime, 400, 0);  // ~75 px digits, rendered once a minute
  lv_obj_set_style_transform_pivot_x(ambTime, 0, 0);
  lv_obj_set_style_transform_pivot_y(ambTime, 0, 0);
  lv_obj_set_pos(ambTime, 0, 0);
  ambDate = makeLabel(ambBox, "", F_TITLE, lv_color_hex(0x7A86B0));
  lv_obj_set_pos(ambDate, 4, 86);
  ambWx = makeLabel(ambBox, "", F_LABEL, lv_color_hex(0x5E6890));
  lv_obj_set_pos(ambWx, 4, 116);
  if (dgOk) {  // the dragon (or egg) asleep under the clock
    lv_obj_t *pet = lv_obj_create(ambBox);
    lv_obj_remove_style_all(pet);
    lv_obj_set_size(pet, 220, 130);
    lv_obj_set_pos(pet, 150, 136);
    lv_obj_clear_flag(pet, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_opa(pet, LV_OPA_60, 0);
    if (dg.stage == ST_EGG) dgDrawEgg(pet, 220, 130);
    else dgDrawDragon(pet, dg.stage, true, false, 1, 220, 130);
  }
  pageTimer(ambScr, ambientRefresh, 20000);
  ambientRefresh(nullptr);
  statusbarShow(false);
  if (ccOv) ccClose();
  lv_scr_load(ambScr);
  ledcWrite(0, blLevel * 15 / 100 < 8 ? 8 : blLevel * 15 / 100);
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
