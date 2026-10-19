#pragma once
// Clock app (V2.4): daily alarm, countdown timer and stopwatch. The alarm and the timer keep running when the
// page is closed; when they go off a full-screen ring card appears above everything (and wakes the screen).

// ---------- shared state ----------
static uint32_t tmrEnd = 0, tmrLeft = 0, tmrTotal = 5 * 60000;  // timer: end time (running) or time left (paused)
static uint32_t swStart = 0, swAcc = 0;                          // stopwatch: start (running) and accumulated ms
static bool swRun = false;
static uint32_t snoozeAt = 0, ringStarted = 0;
static int alarmFiredDay = -1;
static lv_obj_t *ringOv = nullptr;

static void ringClose() {
  soundStop();
  if (ringOv) lv_obj_del(ringOv);
  ringOv = nullptr;
}

static void ringShow(const char *title, const char *sub, bool canSnooze) {
  ringClose();
  if (screenOff) {  // wake up
    screenOff = false;
    gTouchSwallow = false;
    ledcWrite(0, blLevel);
  }
  if (ambOn) ambientExit();
  lv_disp_trig_activity(nullptr);
  ringOv = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(ringOv);
  lv_obj_set_size(ringOv, 480, 320);
  lv_obj_set_style_bg_color(ringOv, lv_color_hex(0x0B0F24), 0);
  lv_obj_set_style_bg_opa(ringOv, LV_OPA_90, 0);
  lv_obj_add_flag(ringOv, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *ic = lv_img_create(ringOv);
  lv_img_set_src(ic, &ic_clock);
  lv_obj_align(ic, LV_ALIGN_TOP_MID, 0, 34);
  lv_obj_t *t = makeLabel(ringOv, title, F_HEADING, C_TEXT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 104);
  lv_obj_t *s = makeLabel(ringOv, sub, F_DISPLAY, C_ACCENT);
  lv_obj_align(s, LV_ALIGN_TOP_MID, 0, 144);
  lv_obj_t *stop = smallBtn(ringOv, DL("Tắt", "Stop"), C_DANGER, [](lv_event_t *) { ringClose(); }, nullptr, 180);
  lv_obj_set_height(stop, 52);
  if (canSnooze) {
    lv_obj_align(stop, LV_ALIGN_BOTTOM_MID, -100, -22);
    lv_obj_t *sn = smallBtn(ringOv, DL("Báo lại 5 phút", "Snooze 5 min"), C_ACCENT, [](lv_event_t *) {
      snoozeAt = millis() + 5 * 60000UL;
      ringClose();
      uiToast(DL("Sẽ báo lại sau 5 phút", "Snoozed for 5 minutes"));
    }, nullptr, 180);
    lv_obj_set_height(sn, 52);
    lv_obj_align(sn, LV_ALIGN_BOTTOM_MID, 100, -22);
  } else {
    lv_obj_align(stop, LV_ALIGN_BOTTOM_MID, 0, -22);
  }
  ringStarted = millis();
  soundPlay(SND(SND_ALARM), -1, true);
}

// Called from loop() a few times per second.
static void clockTick() {
  if (ringOv && millis() - ringStarted > 120000UL) ringClose();  // give up after 2 minutes
  if (tmrEnd && (int32_t)(millis() - tmrEnd) >= 0) {
    tmrEnd = 0;
    tmrLeft = 0;
    ringShow(DL("Hết giờ!", "Time's up!"), "00:00", false);
  }
  if (snoozeAt && (int32_t)(millis() - snoozeAt) >= 0) {
    snoozeAt = 0;
    char b[8];
    snprintf(b, sizeof(b), "%02d:%02d", cfg.alarmH, cfg.alarmM);
    ringShow(DL("Báo thức", "Alarm"), b, true);
  }
  struct tm t;
  if (!cfg.alarmOn || time(nullptr) < 1700000000 || !getLocalTime(&t, 0)) return;
  if (t.tm_hour == cfg.alarmH && t.tm_min == cfg.alarmM && t.tm_yday != alarmFiredDay) {
    alarmFiredDay = t.tm_yday;
    char b[8];
    snprintf(b, sizeof(b), "%02d:%02d", cfg.alarmH, cfg.alarmM);
    ringShow(DL("Báo thức", "Alarm"), b, true);
  }
}

// ---------- page ----------
static lv_obj_t *ckTabs[3], *ckTmrLbl, *ckTmrBtn, *ckSwLbl, *ckSwBtn, *ckAlarmLbl, *ckRollH, *ckRollM;

static uint32_t tmrRemaining() { return tmrEnd ? (int32_t)(tmrEnd - millis()) > 0 ? tmrEnd - millis() : 0 : tmrLeft; }
static uint32_t swElapsed() { return swAcc + (swRun ? millis() - swStart : 0); }

static void ckAlarmText() {
  if (cfg.alarmOn) lv_label_set_text_fmt(ckAlarmLbl, isVi() ? "Báo thức lúc %02d:%02d mỗi ngày" : "Alarm at %02d:%02d every day", cfg.alarmH, cfg.alarmM);
  else lv_label_set_text(ckAlarmLbl, DL("Báo thức đang tắt", "Alarm is off"));
}
static void ckRefresh(lv_timer_t *) {
  uint32_t r = tmrRemaining();
  if (!tmrEnd && !tmrLeft) r = tmrTotal;
  lv_label_set_text_fmt(ckTmrLbl, "%02lu:%02lu", (unsigned long)(r / 60000), (unsigned long)((r / 1000) % 60));
  lv_label_set_text(lv_obj_get_child(ckTmrBtn, 0), tmrEnd ? DL("Tạm dừng", "Pause") : DL("Bắt đầu", "Start"));
  uint32_t e = swElapsed();
  lv_label_set_text_fmt(ckSwLbl, "%02lu:%02lu.%lu", (unsigned long)(e / 60000), (unsigned long)((e / 1000) % 60), (unsigned long)((e / 100) % 10));
  lv_label_set_text(lv_obj_get_child(ckSwBtn, 0), swRun ? DL("Dừng", "Stop") : DL("Bắt đầu", "Start"));
}
static void ckShowTab(int i) {
  for (int k = 0; k < 3; k++) {
    if (k == i) lv_obj_clear_flag(ckTabs[k], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ckTabs[k], LV_OBJ_FLAG_HIDDEN);
  }
}

static lv_obj_t *ckSegment(lv_obj_t *parent, const char **map, int checked, lv_event_cb_t cb) {
  lv_obj_t *m = lv_btnmatrix_create(parent);
  lv_btnmatrix_set_map(m, map);
  lv_btnmatrix_set_btn_ctrl_all(m, LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(m, true);
  lv_btnmatrix_set_btn_ctrl(m, checked, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_set_style_bg_opa(m, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(m, 0, 0);
  lv_obj_set_style_pad_all(m, 0, 0);
  lv_obj_set_style_pad_column(m, 8, 0);
  lv_obj_set_style_pad_row(m, 8, 0);
  lv_obj_set_style_text_font(m, F_BODY, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(m, C_BORDER, LV_PART_ITEMS);
  lv_obj_set_style_text_color(m, C_TEXT, LV_PART_ITEMS);
  lv_obj_set_style_border_width(m, 0, LV_PART_ITEMS);
  lv_obj_set_style_shadow_width(m, 0, LV_PART_ITEMS);
  lv_obj_set_style_radius(m, 12, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(m, C_ACCENT, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(m, lv_color_hex(0x04101A), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_add_event_cb(m, cb, LV_EVENT_VALUE_CHANGED, nullptr);
  return m;
}

static lv_obj_t *ckRoller(lv_obj_t *parent, int n, int sel) {
  static char opts[2][200];
  static int which = 0;
  char *o = opts[which++ & 1];
  o[0] = 0;
  for (int i = 0; i < n; i++) {
    char b[5];
    snprintf(b, sizeof(b), i ? "\n%02d" : "%02d", i);
    strcat(o, b);
  }
  lv_obj_t *r = lv_roller_create(parent);
  lv_roller_set_options(r, o, LV_ROLLER_MODE_INFINITE);
  lv_roller_set_visible_row_count(r, 3);
  lv_roller_set_selected(r, sel, LV_ANIM_OFF);
  lv_obj_set_width(r, 96);
  lv_obj_set_style_text_font(r, F_HEADING, 0);
  lv_obj_set_style_bg_color(r, C_CARD, 0);
  lv_obj_set_style_text_color(r, C_MUTED, 0);
  lv_obj_set_style_border_color(r, C_BORDER, 0);
  lv_obj_set_style_bg_color(r, C_ACCENT, LV_PART_SELECTED);
  lv_obj_set_style_text_color(r, lv_color_hex(0x04101A), LV_PART_SELECTED);
  lv_obj_add_event_cb(
      r,
      [](lv_event_t *) {
        cfg.alarmH = lv_roller_get_selected(ckRollH);
        cfg.alarmM = lv_roller_get_selected(ckRollM);
        alarmFiredDay = -1;
        cfgSave();
        ckAlarmText();
      },
      LV_EVENT_VALUE_CHANGED, nullptr);
  return r;
}

static void pageClock() {
  lv_obj_t *c;
  lv_obj_t *s = pageBegin("Clock", &c);
  lv_obj_del(c);
  static const char *tabMap[4];
  tabMap[0] = DL("Báo thức", "Alarm");
  tabMap[1] = DL("Hẹn giờ", "Timer");
  tabMap[2] = DL("Bấm giờ", "Stopwatch");
  tabMap[3] = "";
  lv_obj_t *seg = ckSegment(s, tabMap, 0, [](lv_event_t *e) {
    uint32_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
    if (id < 3) ckShowTab(id);
  });
  lv_obj_set_size(seg, 456, 38);
  lv_obj_set_pos(seg, 12, SB_H + 50);

  for (int i = 0; i < 3; i++) {
    ckTabs[i] = lv_obj_create(s);
    lv_obj_remove_style_all(ckTabs[i]);
    lv_obj_set_size(ckTabs[i], 456, 320 - (SB_H + 96));
    lv_obj_set_pos(ckTabs[i], 12, SB_H + 96);
    lv_obj_clear_flag(ckTabs[i], LV_OBJ_FLAG_SCROLLABLE);
  }

  // alarm
  ckRollH = ckRoller(ckTabs[0], 24, cfg.alarmH);
  lv_obj_set_pos(ckRollH, 20, 6);
  lv_obj_t *colon = makeLabel(ckTabs[0], ":", F_HEADING, C_TEXT);
  lv_obj_set_pos(colon, 122, 52);
  ckRollM = ckRoller(ckTabs[0], 60, cfg.alarmM);
  lv_obj_set_pos(ckRollM, 138, 6);
  lv_obj_t *sw = lv_switch_create(ckTabs[0]);
  lv_obj_set_size(sw, 70, 36);
  lv_obj_set_pos(sw, 300, 30);
  lv_obj_set_style_bg_color(sw, C_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
  if (cfg.alarmOn) lv_obj_add_state(sw, LV_STATE_CHECKED);
  lv_obj_add_event_cb(
      sw,
      [](lv_event_t *e) {
        cfg.alarmOn = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
        alarmFiredDay = -1;
        cfgSave();
        ckAlarmText();
      },
      LV_EVENT_VALUE_CHANGED, nullptr);
  ckAlarmLbl = makeLabel(ckTabs[0], "", F_BODY, C_MUTED);
  lv_obj_set_width(ckAlarmLbl, 200);
  lv_label_set_long_mode(ckAlarmLbl, LV_LABEL_LONG_WRAP);
  lv_obj_set_pos(ckAlarmLbl, 250, 80);
  ckAlarmText();

  // timer
  ckTmrLbl = makeLabel(ckTabs[1], "", F_DISPLAY, C_TEXT);
  lv_obj_set_pos(ckTmrLbl, 14, 6);
  static const char *preMap[] = {"1'", "3'", "5'", "\n", "10'", "15'", "30'", ""};
  lv_obj_t *pre = ckSegment(ckTabs[1], preMap, 2, [](lv_event_t *e) {
    static const uint8_t mins[] = {1, 3, 5, 10, 15, 30};
    uint32_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
    if (id >= 6 || tmrEnd) return;
    tmrTotal = mins[id] * 60000UL;
    tmrLeft = 0;
  });
  lv_obj_set_size(pre, 200, 84);
  lv_obj_set_pos(pre, 250, 4);
  ckTmrBtn = smallBtn(ckTabs[1], "", C_ACCENT, [](lv_event_t *) {
    if (tmrEnd) {  // pause
      tmrLeft = tmrRemaining();
      tmrEnd = 0;
    } else {
      tmrEnd = millis() + (tmrLeft ? tmrLeft : tmrTotal);
      tmrLeft = 0;
    }
  }, nullptr, 216);
  lv_obj_set_height(ckTmrBtn, 48);
  lv_obj_set_pos(ckTmrBtn, 4, 140);
  lv_obj_t *rs = smallBtn(ckTabs[1], DL("Đặt lại", "Reset"), C_MUTED, [](lv_event_t *) {
    tmrEnd = 0;
    tmrLeft = 0;
  }, nullptr, 216);
  lv_obj_set_height(rs, 48);
  lv_obj_set_pos(rs, 236, 140);

  // stopwatch
  ckSwLbl = makeLabel(ckTabs[2], "", F_DISPLAY, C_TEXT);
  lv_obj_align(ckSwLbl, LV_ALIGN_TOP_MID, 0, 26);
  ckSwBtn = smallBtn(ckTabs[2], "", C_ACCENT, [](lv_event_t *) {
    if (swRun) swAcc += millis() - swStart;
    else swStart = millis();
    swRun = !swRun;
  }, nullptr, 216);
  lv_obj_set_height(ckSwBtn, 48);
  lv_obj_set_pos(ckSwBtn, 4, 140);
  lv_obj_t *sr = smallBtn(ckTabs[2], DL("Đặt lại", "Reset"), C_MUTED, [](lv_event_t *) {
    swRun = false;
    swAcc = 0;
  }, nullptr, 216);
  lv_obj_set_height(sr, 48);
  lv_obj_set_pos(sr, 236, 140);

  ckShowTab(0);
  ckRefresh(nullptr);
  pageTimer(s, ckRefresh, 100);
  pageShow(s);
}
