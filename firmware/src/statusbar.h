#pragma once
// V2 shell: status bar on every screen (top layer), toasts, and the pull-down control center.
// Included by main.cpp after dragon.h (reads the dragon's needs for the alert chip).

static lv_obj_t *sbBar, *sbTime, *sbDragon, *sbAi, *sbWifi[4];
static lv_obj_t *ccOv = nullptr, *ccPanel = nullptr, *ccDndBtn = nullptr;
static lv_obj_t *toastObj = nullptr;
static bool sbVisible = true;

static void ambientShow();      // ambient.h
static bool ambOn = false;      // ambient screen showing (ambient.h)
static void screenSleepNow();   // main.cpp
static bool homeIsActive();     // main.cpp: main screen showing the home tile
static void soundPlay(const Note *seq, int len, int repeat, bool evenInDnd);

// ---------- dragon alert ----------
static const char *dgAlert() {
  if (!dgOk || cfg.dnd) return nullptr;
  if (dg.stage == ST_EGG) return dg.energy < 30 ? DL("Trứng lạnh", "Egg is cold") : nullptr;
  if (dg.sick) return DL("Rồng ốm", "Dragon is sick");
  if (dg.sleeping) return nullptr;
  if (dg.satiety < 25) return DL("Rồng đói", "Dragon is hungry");
  if (dg.clean < 25) return DL("Rồng bẩn", "Dragon is dirty");
  if (dg.energy < 25) return DL("Rồng mệt", "Dragon is tired");
  if (dg.mood < 25) return DL("Rồng buồn", "Dragon is sad");
  return nullptr;
}

// ---------- toast ----------
static void toastAnimY(void *o, int32_t v) { lv_obj_set_y((lv_obj_t *)o, v); }
static void uiToast(const char *text, lv_color_t col = C_ACCENT) {
  if (toastObj) lv_obj_del(toastObj);
  toastObj = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(toastObj);
  lv_obj_set_size(toastObj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(toastObj, C_CARD, 0);
  lv_obj_set_style_bg_opa(toastObj, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(toastObj, 14, 0);
  lv_obj_set_style_border_width(toastObj, 2, 0);
  lv_obj_set_style_border_color(toastObj, col, 0);
  lv_obj_set_style_pad_hor(toastObj, 16, 0);
  lv_obj_set_style_pad_ver(toastObj, 9, 0);
  lv_obj_set_style_shadow_width(toastObj, 18, 0);
  lv_obj_set_style_shadow_opa(toastObj, LV_OPA_40, 0);
  lv_obj_clear_flag(toastObj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *l = makeLabel(toastObj, text, F_BODY, C_TEXT);
  lv_obj_set_style_max_width(l, 420, 0);
  lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
  lv_obj_update_layout(toastObj);
  lv_obj_set_x(toastObj, (480 - lv_obj_get_width(toastObj)) / 2);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, toastObj);
  lv_anim_set_exec_cb(&a, toastAnimY);
  lv_anim_set_values(&a, -50, SB_H + 6);
  lv_anim_set_time(&a, 220);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_start(&a);
  lv_obj_t *mine = toastObj;
  lv_timer_t *t = lv_timer_create(
      [](lv_timer_t *t) {
        if (toastObj && toastObj == (lv_obj_t *)t->user_data) {
          lv_obj_del(toastObj);
          toastObj = nullptr;
        }
      },
      3000, mine);
  lv_timer_set_repeat_count(t, 1);
}

// ---------- control center ----------
static void ccClose() {
  if (ccOv) lv_obj_del(ccOv);
  ccOv = ccPanel = ccDndBtn = nullptr;
}
static void ccOpenPage(void (*open)()) {
  ccClose();
  pageReplace = lv_scr_act() != mainScr;
  open();
  pageReplace = false;
}

static lv_obj_t *ccButton(lv_obj_t *parent, const char *sym, const char *text, bool on, lv_event_cb_t cb) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_set_size(b, 82, 62);
  lv_obj_set_style_radius(b, 16, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_bg_color(b, on ? C_ACCENT : C_BORDER, 0);
  lv_obj_set_style_bg_color(b, C_ACCENT2, LV_STATE_PRESSED);
  lv_obj_set_flex_flow(b, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(b, 2, 0);
  lv_obj_set_style_pad_row(b, 3, 0);
  lv_color_t fg = on ? lv_color_hex(0x04101A) : C_TEXT;
  makeLabel(b, sym, F_LABEL, fg);
  lv_obj_t *l = makeLabel(b, text, F_CAPTION, fg);
  lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
  return b;
}

static void ccSetBtnOn(lv_obj_t *b, bool on) {
  lv_obj_set_style_bg_color(b, on ? C_ACCENT : C_BORDER, 0);
  lv_color_t fg = on ? lv_color_hex(0x04101A) : C_TEXT;
  for (uint32_t i = 0; i < lv_obj_get_child_cnt(b); i++) lv_obj_set_style_text_color(lv_obj_get_child(b, i), fg, 0);
}

static lv_obj_t *ccSlider(lv_obj_t *parent, const char *sym, int y, int minV, int maxV, int val, lv_event_cb_t cb) {
  lv_obj_t *ic = makeLabel(parent, sym, F_LABEL, C_TEXT);
  lv_obj_set_pos(ic, 16, y - 2);
  lv_obj_t *s = lv_slider_create(parent);
  lv_slider_set_range(s, minV, maxV);
  lv_slider_set_value(s, val, LV_ANIM_OFF);
  lv_obj_set_size(s, 380, 10);
  lv_obj_set_pos(s, 52, y + 4);
  lv_obj_set_style_bg_color(s, C_BORDER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(s, C_ACCENT, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(s, C_TEXT, LV_PART_KNOB);
  lv_obj_set_ext_click_area(s, 14);
  lv_obj_add_event_cb(s, cb, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(s, cb, LV_EVENT_RELEASED, nullptr);
  return s;
}

static void ccBrightness(lv_event_t *e) {
  lv_obj_t *s = lv_event_get_target(e);
  setBacklight(lv_slider_get_value(s));
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
    Preferences p;
    p.begin("ui", false);
    p.putUChar("bl", blLevel);
    p.end();
  }
}
static void ccVolume(lv_event_t *e) {
  cfg.volume = lv_slider_get_value(lv_event_get_target(e));
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
    cfgSave();
    soundPlay(SND(SND_NOTIFY), 0, true);
  }
}

static void ccPanelY(void *o, int32_t v) { lv_obj_set_y((lv_obj_t *)o, v); }

static void ccOpen() {
  if (ccOv) return;
  ccOv = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(ccOv);
  lv_obj_set_size(ccOv, 480, 320);
  lv_obj_set_style_bg_color(ccOv, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(ccOv, LV_OPA_50, 0);
  lv_obj_add_flag(ccOv, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(
      ccOv, [](lv_event_t *e) {
        if (lv_event_get_target(e) == ccOv) ccClose();
      },
      LV_EVENT_CLICKED, nullptr);

  ccPanel = makeCard(ccOv, 464, 214);
  lv_obj_set_x(ccPanel, 8);
  lv_obj_set_style_radius(ccPanel, 20, 0);
  lv_obj_set_style_shadow_width(ccPanel, 24, 0);
  lv_obj_set_style_shadow_opa(ccPanel, LV_OPA_50, 0);
  lv_obj_add_flag(ccPanel, LV_OBJ_FLAG_CLICKABLE);  // taps on the panel do not close it

  ccSlider(ccPanel, LV_SYMBOL_IMAGE, 18, 20, 255, blLevel, ccBrightness);
  ccSlider(ccPanel, LV_SYMBOL_VOLUME_MAX, 54, 0, 100, cfg.volume, ccVolume);

  lv_obj_t *row = lv_obj_create(ccPanel);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, 440, 66);
  lv_obj_set_pos(row, 12, 88);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(row, 7, 0);
  ccButton(row, LV_SYMBOL_WIFI, "Wi-Fi", wifiConnected(), [](lv_event_t *) { ccOpenPage(pageWifi); });
  ccDndBtn = ccButton(row, LV_SYMBOL_MUTE, DL("Im lặng", "Silent"), cfg.dnd, [](lv_event_t *) {
    cfg.dnd = !cfg.dnd;
    cfgSave();
    ccSetBtnOn(ccDndBtn, cfg.dnd);
    uiToast(cfg.dnd ? DL("Đã bật Không làm phiền", "Do not disturb is on") : DL("Đã tắt Không làm phiền", "Do not disturb is off"));
  });
  ccButton(row, LV_SYMBOL_POWER, DL("Tắt màn", "Screen off"), false, [](lv_event_t *) {
    ccClose();
    screenSleepNow();
  });
  ccButton(row, LV_SYMBOL_EYE_CLOSE, DL("Chờ", "Ambient"), false, [](lv_event_t *) {
    ccClose();
    ambientShow();
  });
  ccButton(row, LV_SYMBOL_SETTINGS, DL("Cài đặt", "Settings"), false, [](lv_event_t *) { ccOpenPage(pageSettings); });

  lv_obj_t *hint = makeLabel(ccPanel, DL("Chạm ra ngoài để đóng", "Tap outside to close"), F_CAPTION, C_MUTED);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, ccPanel);
  lv_anim_set_exec_cb(&a, ccPanelY);
  lv_anim_set_values(&a, -220, SB_H + 2);
  lv_anim_set_time(&a, 220);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_start(&a);
}

// ---------- status bar ----------
static void statusbarShow(bool on) {
  sbVisible = on;
  if (on) lv_obj_clear_flag(sbBar, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(sbBar, LV_OBJ_FLAG_HIDDEN);
}

static void statusbarRefresh(lv_timer_t * = nullptr) {
  if (!sbBar) return;
  struct tm t;
  bool timeOk = time(nullptr) > 1700000000 && getLocalTime(&t, 0);
  if (homeIsActive() || !timeOk) {
    setText(sbTime, "");
  } else if (cfg.clock24) {
    setTextFmt(sbTime, "%02d:%02d", t.tm_hour, t.tm_min);
  } else {
    setTextFmt(sbTime, "%d:%02d %s", t.tm_hour % 12 ? t.tm_hour % 12 : 12, t.tm_min, t.tm_hour < 12 ? "AM" : "PM");
  }
  const char *al = dgAlert();
  if (al) {
    setText(sbDragon, al);
    lv_obj_clear_flag(sbDragon, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(sbDragon, LV_OBJ_FLAG_HIDDEN);
  }
  bool ai = xiaozhiState() == XZ_ACTIVATED;
  lv_obj_set_style_text_color(sbAi, ai ? C_OK : C_MUTED, 0);
  setText(sbAi, cfg.dnd ? LV_SYMBOL_MUTE "  AI" : "AI");
  bool on = wifiConnected();
  int rssi = on ? WiFi.RSSI() : -100;
  int level = !on ? 0 : rssi > -55 ? 4 : rssi > -65 ? 3 : rssi > -75 ? 2 : 1;
  for (int i = 0; i < 4; i++) lv_obj_set_style_bg_color(sbWifi[i], i < level ? C_TEXT : (on ? C_BORDER : C_DANGER), 0);
}

static void statusbarBegin() {
  sbBar = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(sbBar);
  lv_obj_set_size(sbBar, 480, SB_H);
  lv_obj_set_style_bg_color(sbBar, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(sbBar, LV_OPA_20, 0);
  lv_obj_add_flag(sbBar, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_ext_click_area(sbBar, 6);
  lv_obj_add_event_cb(sbBar, [](lv_event_t *) { ccOpen(); }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(
      sbBar,
      [](lv_event_t *) {  // pull down
        lv_point_t v;
        lv_indev_get_vect(lv_indev_get_act(), &v);
        if (v.y > 6) ccOpen();
      },
      LV_EVENT_PRESSING, nullptr);

  sbTime = makeLabel(sbBar, "", F_CAPTION, C_TEXT);
  lv_obj_align(sbTime, LV_ALIGN_LEFT_MID, 12, 0);
  lv_obj_t *grab = shape(sbBar, 224, SB_H - 5, 32, 3, 0xFFFFFF, 2, LV_OPA_40);  // pull handle
  (void)grab;

  lv_obj_t *bars = lv_obj_create(sbBar);
  lv_obj_remove_style_all(bars);
  lv_obj_set_size(bars, 22, 14);
  lv_obj_align(bars, LV_ALIGN_RIGHT_MID, -10, 0);
  for (int i = 0; i < 4; i++) sbWifi[i] = shape(bars, i * 5 + 2, 13 - (4 + i * 3), 3, 4 + i * 3, 0xFFFFFF, 1);
  sbAi = makeLabel(sbBar, "AI", F_CAPTION, C_MUTED);
  lv_obj_align(sbAi, LV_ALIGN_RIGHT_MID, -40, 0);
  sbDragon = makeLabel(sbBar, "", F_CAPTION, C_WARN);
  lv_obj_align_to(sbDragon, sbAi, LV_ALIGN_OUT_LEFT_MID, -14, 0);
  lv_obj_set_style_text_align(sbDragon, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(sbDragon, 140);
  lv_obj_align(sbDragon, LV_ALIGN_RIGHT_MID, -80, 0);

  lv_timer_create(statusbarRefresh, 1000, nullptr);
  statusbarRefresh();
}
