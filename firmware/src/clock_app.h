#pragma once
// Clock app: alarms (up to ALARM_MAX, each once or repeating on chosen weekdays), a countdown timer with a
// progress ring and a stopwatch with laps. Alarms and the timer keep running when the page is closed; when
// they go off a full-screen ring card appears above everything (and wakes the screen).
// Alarms live in NVS namespace "clock" (key "al": packed AlarmItem array); clockBegin() loads them at boot.

static void ambientExit();  // ambient.h

// ---------- alarms ----------
struct AlarmItem {
  uint8_t h, m;
  uint8_t days;  // bit per weekday (bit 0 = Sunday ... bit 6 = Saturday); 0 = once, then it switches off
  uint8_t on;
};
static const int ALARM_MAX = 8;
static AlarmItem alarms[ALARM_MAX];
static int alarmCount = 0;
static int32_t alarmFired[ALARM_MAX];  // minute key (yday * 1440 + minute of day) the alarm last rang in
static bool ckAlarmsDirty = false;     // the open Clock page must rebuild its alarm list

static void alarmSave() {
  Preferences p;
  p.begin("clock", false);
  p.putBytes("al", alarms, sizeof(AlarmItem) * alarmCount);
  p.putUChar("n", alarmCount);
  p.end();
}

static void clockBegin() {
  for (int i = 0; i < ALARM_MAX; i++) alarmFired[i] = -1;
  Preferences p;
  p.begin("clock", false);
  alarmCount = p.getUChar("n", 0xFF);
  if (alarmCount == 0xFF) {  // first boot with this version: take over the single daily alarm of V2
    alarmCount = 0;
    Preferences old;
    old.begin("cfg", false);
    if (old.isKey("alH")) {
      alarms[0] = {(uint8_t)(old.getUChar("alH", 6) % 24), (uint8_t)(old.getUChar("alM", 30) % 60), 0x7F,
                   (uint8_t)old.getBool("alOn", false)};
      alarmCount = 1;
      old.remove("alOn");
      old.remove("alH");
      old.remove("alM");
    }
    old.end();
    p.putBytes("al", alarms, sizeof(AlarmItem) * alarmCount);
    p.putUChar("n", alarmCount);
  } else {
    if (alarmCount > ALARM_MAX) alarmCount = ALARM_MAX;
    if (p.getBytes("al", alarms, sizeof(AlarmItem) * alarmCount) != sizeof(AlarmItem) * alarmCount) alarmCount = 0;
  }
  p.end();
}

static bool clockTimeNow(struct tm &t) { return time(nullptr) > 1700000000 && getLocalTime(&t, 0); }

// Minutes from now until alarm a rings next (0 = this minute), or -1 if it is off.
static int alarmMinutesAway(const AlarmItem &a, const struct tm &t) {
  if (!a.on) return -1;
  int now = t.tm_hour * 60 + t.tm_min, at = a.h * 60 + a.m;
  for (int d = 0; d < 8; d++) {
    int away = d * 1440 + at - now;
    if (away < 0) continue;
    if (!a.days || (a.days & (1 << ((t.tm_wday + d) % 7)))) return away;
  }
  return -1;
}

// The alarm that rings next: index, or -1 (none on, or the time is not synced yet).
static int alarmNext(int *minutesAway = nullptr) {
  struct tm t;
  if (!clockTimeNow(t)) return -1;
  int best = -1, bestAway = 0;
  for (int i = 0; i < alarmCount; i++) {
    int away = alarmMinutesAway(alarms[i], t);
    if (away >= 0 && (best < 0 || away < bestAway)) {
      best = i;
      bestAway = away;
    }
  }
  if (minutesAway) *minutesAway = bestAway;
  return best;
}

static const char *const WD_VI[] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
static const char *const WD_EN[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static void alarmRepeatText(uint8_t days, char *out, size_t n) {
  if (days == 0) strlcpy(out, DL("Một lần", "Once"), n);
  else if (days == 0x7F) strlcpy(out, DL("Hằng ngày", "Every day"), n);
  else if (days == 0x3E) strlcpy(out, DL("Thứ 2 - Thứ 6", "Weekdays"), n);
  else if (days == 0x41) strlcpy(out, DL("Cuối tuần", "Weekends"), n);
  else {
    out[0] = 0;
    for (int k = 1; k <= 7; k++) {  // Monday first, Sunday last
      int d = k % 7;
      if (!(days & (1 << d))) continue;
      if (out[0]) strlcat(out, ", ", n);
      strlcat(out, isVi() ? WD_VI[d] : WD_EN[d], n);
    }
  }
}

static void alarmAwayText(int away, char *out, size_t n) {
  if (away < 1) snprintf(out, n, "%s", DL("trong vòng 1 phút", "within a minute"));
  else if (away < 60) snprintf(out, n, isVi() ? "sau %d phút" : "in %d min", away);
  else if (away % 60 == 0) snprintf(out, n, isVi() ? "sau %d giờ" : "in %d h", away / 60);
  else snprintf(out, n, isVi() ? "sau %d giờ %d phút" : "in %d h %d min", away / 60, away % 60);
}

static void alarmTimeText(int h, int m, char *out, size_t n) {
  if (cfg.clock24) snprintf(out, n, "%02d:%02d", h, m);
  else snprintf(out, n, "%d:%02d %s", h % 12 ? h % 12 : 12, m, h < 12 ? DL("SA", "AM") : DL("CH", "PM"));
}

// ---------- ringing ----------
static uint32_t tmrEnd = 0, tmrLeft = 0, tmrTotal = 5 * 60000;  // timer: end time (running) or time left (paused)
static uint32_t swStart = 0, swAcc = 0;                          // stopwatch: start (running) and accumulated ms
static bool swRun = false;
static uint32_t snoozeAt = 0, ringStarted = 0;
static char snoozeTime[12] = "";
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
  lv_obj_set_style_bg_opa(ringOv, LV_OPA_COVER, 0);
  lv_obj_add_flag(ringOv, LV_OBJ_FLAG_CLICKABLE);
  shape(ringOv, 240 - 70, 26, 140, 140, 0x38E1FF, LV_RADIUS_CIRCLE, LV_OPA_10);  // halo behind the bell
  shape(ringOv, 240 - 52, 44, 104, 104, 0x38E1FF, LV_RADIUS_CIRCLE, LV_OPA_20);
  lv_obj_t *ic = lv_img_create(ringOv);
  lv_img_set_src(ic, &ic_clock);
  lv_obj_align(ic, LV_ALIGN_TOP_MID, 0, 64);
  lv_obj_t *s = makeLabel(ringOv, sub, F_DISPLAY, C_TEXT);
  lv_obj_align(s, LV_ALIGN_TOP_MID, 0, 168);
  lv_obj_t *t = makeLabel(ringOv, title, F_TITLE, C_ACCENT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 222);
  lv_obj_t *stop = smallBtn(ringOv, DL("Tắt", "Stop"), C_DANGER, [](lv_event_t *) { ringClose(); }, nullptr, 180);
  lv_obj_set_height(stop, 48);
  if (canSnooze) {
    lv_obj_align(stop, LV_ALIGN_BOTTOM_MID, -100, -14);
    lv_obj_t *sn = smallBtn(ringOv, DL("Báo lại 5 phút", "Snooze 5 min"), C_ACCENT, [](lv_event_t *) {
      snoozeAt = millis() + 5 * 60000UL;
      ringClose();
      uiToast(DL("Sẽ báo lại sau 5 phút", "Snoozed for 5 minutes"));
    }, nullptr, 180);
    lv_obj_set_height(sn, 48);
    lv_obj_align(sn, LV_ALIGN_BOTTOM_MID, 100, -14);
  } else {
    lv_obj_align(stop, LV_ALIGN_BOTTOM_MID, 0, -14);
  }
  ringStarted = millis();
  soundPlay(SND(SND_ALARM), -1, true);
}

static void alarmCheck() {
  struct tm t;
  if (!clockTimeNow(t)) return;
  int32_t key = t.tm_yday * 1440 + t.tm_hour * 60 + t.tm_min;
  int hit = -1;
  bool changed = false;
  for (int i = 0; i < alarmCount; i++) {
    AlarmItem &a = alarms[i];
    if (!a.on || a.h != t.tm_hour || a.m != t.tm_min || alarmFired[i] == key) continue;
    if (a.days && !(a.days & (1 << t.tm_wday))) continue;
    alarmFired[i] = key;
    if (!a.days) {  // one-time alarm: done
      a.on = 0;
      changed = true;
    }
    if (hit < 0) hit = i;
  }
  if (changed) {
    alarmSave();
    ckAlarmsDirty = true;
  }
  if (hit >= 0) {
    alarmTimeText(alarms[hit].h, alarms[hit].m, snoozeTime, sizeof(snoozeTime));
    ringShow(DL("Báo thức", "Alarm"), snoozeTime, true);
  }
}

// Called from loop() a few times per second.
static void clockTick() {
  if (ringOv && millis() - ringStarted > 120000UL) ringClose();  // give up after 2 minutes
  if (tmrEnd && (int32_t)(millis() - tmrEnd) >= 0) {
    tmrEnd = 0;
    tmrLeft = 0;
    ringShow(DL("Hết giờ hẹn", "Time's up"), "00:00", false);
  }
  if (snoozeAt && (int32_t)(millis() - snoozeAt) >= 0) {
    snoozeAt = 0;
    ringShow(DL("Báo thức", "Alarm"), snoozeTime, true);
  }
  alarmCheck();
}

// ---------- page: shared widgets ----------
static lv_obj_t *ckScr, *ckTabs[3];

static uint32_t tmrRemaining() { return tmrEnd ? (int32_t)(tmrEnd - millis()) > 0 ? tmrEnd - millis() : 0 : tmrLeft; }
static uint32_t swElapsed() { return swAcc + (swRun ? millis() - swStart : 0); }

// Pill button: filled (primary action) or tinted outline.
static lv_obj_t *ckBtn(lv_obj_t *parent, const char *text, lv_color_t col, bool filled, lv_event_cb_t cb, int w, int h,
                       void *ud = nullptr) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_set_size(b, w, h);
  lv_obj_set_style_radius(b, h / 2, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_bg_color(b, col, 0);
  lv_obj_set_style_bg_opa(b, filled ? LV_OPA_COVER : LV_OPA_20, 0);
  lv_obj_set_style_border_width(b, filled ? 0 : 1, 0);
  lv_obj_set_style_border_color(b, col, 0);
  lv_obj_set_style_bg_opa(b, filled ? LV_OPA_80 : LV_OPA_40, LV_STATE_PRESSED);
  lv_obj_set_style_opa(b, LV_OPA_40, LV_STATE_DISABLED);
  lv_obj_t *l = makeLabel(b, text, F_LABEL, filled ? lv_color_hex(0x04101A) : col);
  lv_obj_center(l);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
  return b;
}

// Segmented control / chip group on a button matrix.
static lv_obj_t *ckChips(lv_obj_t *parent, const char **map, bool oneChecked, lv_event_cb_t cb) {
  lv_obj_t *m = lv_btnmatrix_create(parent);
  lv_btnmatrix_set_map(m, map);
  lv_btnmatrix_set_btn_ctrl_all(m, LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(m, oneChecked);
  lv_obj_set_style_bg_opa(m, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(m, 0, 0);
  lv_obj_set_style_pad_all(m, 0, 0);
  lv_obj_set_style_pad_column(m, 6, 0);
  lv_obj_set_style_pad_row(m, 6, 0);
  lv_obj_set_style_text_font(m, F_BODY, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(m, C_CARD, LV_PART_ITEMS);
  lv_obj_set_style_bg_opa(m, LV_OPA_COVER, LV_PART_ITEMS);
  lv_obj_set_style_border_width(m, 1, LV_PART_ITEMS);
  lv_obj_set_style_border_color(m, C_BORDER, LV_PART_ITEMS);
  lv_obj_set_style_text_color(m, C_TEXT, LV_PART_ITEMS);
  lv_obj_set_style_shadow_width(m, 0, LV_PART_ITEMS);
  lv_obj_set_style_radius(m, 12, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(m, C_ACCENT, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_border_color(m, C_ACCENT, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(m, lv_color_hex(0x04101A), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(m, C_BORDER, LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_set_style_text_opa(m, LV_OPA_40, LV_PART_ITEMS | LV_STATE_DISABLED);
  if (cb) lv_obj_add_event_cb(m, cb, LV_EVENT_VALUE_CHANGED, nullptr);
  return m;
}

// ---------- alarm tab ----------
static lv_obj_t *ckAlList, *ckEdOv, *ckRollH, *ckRollM, *ckDayM, *ckEdSum;
static int ckEdIdx = -1;
static uint8_t ckEdDays = 0;
static void ckAlarmList();

static void ckEdSummary() {
  char rep[48];
  alarmRepeatText(ckEdDays, rep, sizeof(rep));
  setTextFmt(ckEdSum, "%s: %s", DL("Lặp lại", "Repeat"), rep);
}
static void ckEdShowDays() {  // chip i: 0..5 = Mon..Sat, 6 = Sun
  for (int i = 0; i < 7; i++) {
    int d = i == 6 ? 0 : i + 1;
    if (ckEdDays & (1 << d)) lv_btnmatrix_set_btn_ctrl(ckDayM, i, LV_BTNMATRIX_CTRL_CHECKED);
    else lv_btnmatrix_clear_btn_ctrl(ckDayM, i, LV_BTNMATRIX_CTRL_CHECKED);
  }
  ckEdSummary();
}
static void ckEdClose() {
  if (ckEdOv) lv_obj_del_async(ckEdOv);  // called from a button inside the overlay
  ckEdOv = nullptr;
}

static void ckEdSave(lv_event_t *) {
  AlarmItem a = {(uint8_t)lv_roller_get_selected(ckRollH), (uint8_t)lv_roller_get_selected(ckRollM), ckEdDays, 1};
  int idx = ckEdIdx;
  if (idx < 0) {
    if (alarmCount >= ALARM_MAX) return;
    idx = alarmCount++;
  }
  alarms[idx] = a;
  alarmFired[idx] = -1;
  for (int i = 1; i < alarmCount; i++)  // keep the list in time order (alarmFired moves along)
    for (int j = i; j > 0 && alarms[j].h * 60 + alarms[j].m < alarms[j - 1].h * 60 + alarms[j - 1].m; j--) {
      AlarmItem t = alarms[j];
      alarms[j] = alarms[j - 1];
      alarms[j - 1] = t;
      int32_t f = alarmFired[j];
      alarmFired[j] = alarmFired[j - 1];
      alarmFired[j - 1] = f;
    }
  alarmSave();
  ckEdClose();
  ckAlarmList();
  struct tm t;
  if (clockTimeNow(t)) {
    char away[40], msg[64];
    alarmAwayText(alarmMinutesAway(a, t), away, sizeof(away));
    snprintf(msg, sizeof(msg), "%s %s", DL("Báo thức", "Alarm"), away);
    uiToast(msg);
  }
}

static void ckEdDelete(lv_event_t *) {
  if (ckEdIdx < 0 || ckEdIdx >= alarmCount) return;
  for (int i = ckEdIdx; i < alarmCount - 1; i++) {
    alarms[i] = alarms[i + 1];
    alarmFired[i] = alarmFired[i + 1];
  }
  alarmCount--;
  alarmSave();
  ckEdClose();
  ckAlarmList();
  uiToast(DL("Đã xoá báo thức", "Alarm deleted"));
}

static lv_obj_t *ckRoller(lv_obj_t *parent, const char *opts, int sel) {
  // infinite: drag past 59 and it carries on at 00, 01, ... (LV_ROLLER_INF_PAGES is 3 in lv_conf.h, see there)
  lv_obj_t *r = lv_roller_create(parent);
  lv_roller_set_options(r, opts, LV_ROLLER_MODE_INFINITE);
  lv_obj_set_style_text_font(r, F_TITLE, 0);  // before the row count: the height is rows x this font's line height
  lv_roller_set_visible_row_count(r, 3);
  lv_roller_set_selected(r, sel, LV_ANIM_OFF);
  lv_obj_set_width(r, 96);
  lv_obj_set_style_text_align(r, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_bg_color(r, C_CARD, 0);
  lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(r, C_BORDER, 0);
  lv_obj_set_style_border_width(r, 1, 0);
  lv_obj_set_style_radius(r, 16, 0);
  lv_obj_set_style_text_color(r, C_MUTED, 0);
  lv_obj_set_style_bg_color(r, C_ACCENT, LV_PART_SELECTED);
  lv_obj_set_style_bg_opa(r, LV_OPA_COVER, LV_PART_SELECTED);
  lv_obj_set_style_text_color(r, lv_color_hex(0x04101A), LV_PART_SELECTED);
  return r;
}

static void ckEdOpen(int idx) {
  static char hOpts[24 * 3], mOpts[60 * 3];
  if (!hOpts[0]) {
    for (int i = 0; i < 24; i++) snprintf(hOpts + i * 3, 4, i < 23 ? "%02d\n" : "%02d", i);
    for (int i = 0; i < 60; i++) snprintf(mOpts + i * 3, 4, i < 59 ? "%02d\n" : "%02d", i);
  }
  ckEdIdx = idx;
  AlarmItem a = idx >= 0 ? alarms[idx] : AlarmItem{7, 0, 0x3E, 1};
  if (idx < 0) {  // new alarm: start from the next round hour
    struct tm t;
    if (clockTimeNow(t)) a.h = (t.tm_hour + 1) % 24;
  }
  ckEdDays = a.days;

  ckEdOv = lv_obj_create(ckScr);
  lv_obj_remove_style_all(ckEdOv);
  lv_obj_set_size(ckEdOv, 480, 320);
  lv_obj_set_style_bg_color(ckEdOv, lv_color_hex(0x0B1024), 0);
  lv_obj_set_style_bg_opa(ckEdOv, LV_OPA_COVER, 0);
  lv_obj_add_flag(ckEdOv, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ckEdOv, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *ttl = makeLabel(ckEdOv, idx >= 0 ? DL("Sửa báo thức", "Edit alarm") : DL("Báo thức mới", "New alarm"), F_TITLE, C_TEXT);
  lv_obj_set_pos(ttl, 20, SB_H + 10);

  // time: hour and minute rollers
  ckRollH = ckRoller(ckEdOv, hOpts, a.h);
  lv_obj_set_pos(ckRollH, 20, SB_H + 48);
  lv_obj_t *colon = makeLabel(ckEdOv, ":", F_HEADING, C_TEXT);
  lv_obj_align_to(colon, ckRollH, LV_ALIGN_OUT_RIGHT_MID, 6, -2);
  ckRollM = ckRoller(ckEdOv, mOpts, a.m);
  lv_obj_align_to(ckRollM, ckRollH, LV_ALIGN_OUT_RIGHT_MID, 26, 0);
  lv_obj_t *hl = makeLabel(ckEdOv, DL("Giờ", "Hour"), F_CAPTION, C_MUTED);
  lv_obj_align_to(hl, ckRollH, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
  lv_obj_t *ml = makeLabel(ckEdOv, DL("Phút", "Minute"), F_CAPTION, C_MUTED);
  lv_obj_align_to(ml, ckRollM, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

  // repeat: weekday chips + shortcuts
  lv_obj_t *rl = makeLabel(ckEdOv, "Repeat", F_CAPTION, C_MUTED);
  setText(rl, DL("LẶP LẠI", "REPEAT"));
  lv_obj_set_pos(rl, 256, SB_H + 48);
  static const char *dayMapVi[] = {"T2", "T3", "T4", "T5", "\n", "T6", "T7", "CN", ""};
  static const char *dayMapEn[] = {"Mo", "Tu", "We", "Th", "\n", "Fr", "Sa", "Su", ""};
  ckDayM = ckChips(ckEdOv, isVi() ? dayMapVi : dayMapEn, false, [](lv_event_t *) {
    ckEdDays = 0;
    for (int i = 0; i < 7; i++)
      if (lv_btnmatrix_has_btn_ctrl(ckDayM, i, LV_BTNMATRIX_CTRL_CHECKED)) ckEdDays |= 1 << (i == 6 ? 0 : i + 1);
    ckEdSummary();
  });
  lv_obj_set_size(ckDayM, 204, 82);
  lv_obj_set_pos(ckDayM, 256, SB_H + 68);
  static const char *quickMap[3];
  quickMap[0] = DL("Hằng ngày", "Every day");
  quickMap[1] = DL("T2 - T6", "Weekdays");
  quickMap[2] = "";
  lv_obj_t *quick = ckChips(ckEdOv, quickMap, false, [](lv_event_t *e) {
    lv_obj_t *m = lv_event_get_target(e);
    uint16_t id = lv_btnmatrix_get_selected_btn(m);
    lv_btnmatrix_clear_btn_ctrl_all(m, LV_BTNMATRIX_CTRL_CHECKED);  // shortcuts, not toggles
    if (id == 0) ckEdDays = 0x7F;
    else if (id == 1) ckEdDays = 0x3E;
    else return;
    ckEdShowDays();
  });
  lv_obj_set_size(quick, 204, 36);
  lv_obj_set_pos(quick, 256, SB_H + 158);
  ckEdSum = makeLabel(ckEdOv, "", F_CAPTION, C_ACCENT);
  lv_obj_set_width(ckEdSum, 204);
  lv_label_set_long_mode(ckEdSum, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(ckEdSum, 258, SB_H + 202);
  ckEdShowDays();

  // actions
  if (idx >= 0) {
    lv_obj_t *del = ckBtn(ckEdOv, LV_SYMBOL_TRASH, C_DANGER, false, ckEdDelete, 64, 46);
    lv_obj_set_pos(del, 20, 320 - 58);
  }
  lv_obj_t *cancel = ckBtn(ckEdOv, DL("Huỷ", "Cancel"), C_MUTED, false, [](lv_event_t *) { ckEdClose(); }, 120, 46);
  lv_obj_set_pos(cancel, 210, 320 - 58);
  lv_obj_t *save = ckBtn(ckEdOv, DL("Lưu", "Save"), C_ACCENT, true, ckEdSave, 120, 46);
  lv_obj_set_pos(save, 340, 320 - 58);
}

static void ckAlarmToggle(lv_event_t *e) {
  lv_obj_t *sw = lv_event_get_target(e);
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (i >= alarmCount) return;
  alarms[i].on = lv_obj_has_state(sw, LV_STATE_CHECKED);
  alarmFired[i] = -1;
  alarmSave();
  lv_obj_t *card = lv_obj_get_parent(sw);  // brighten / dim the time in place (the list is not rebuilt here)
  lv_obj_set_style_text_color(lv_obj_get_child(card, 0), alarms[i].on ? C_TEXT : C_MUTED, 0);
  ckAlarmsDirty = true;  // the "next alarm" line is refreshed by ckRefresh
}

static void ckAlarmList() {
  ckAlarmsDirty = false;
  lv_obj_clean(ckAlList);
  // next alarm (or why none will ring)
  lv_obj_t *next = makeLabel(ckAlList, "", F_BODY, C_ACCENT);
  struct tm t;
  int away = 0, n = alarmNext(&away);
  if (!clockTimeNow(t)) {
    setText(next, DL(LV_SYMBOL_WARNING "  Chưa đồng bộ giờ: báo thức sẽ chạy khi có Wi-Fi", LV_SYMBOL_WARNING "  Time not synced yet: alarms need Wi-Fi"));
    lv_obj_set_style_text_color(next, C_WARN, 0);
  } else if (n < 0) {
    setText(next, alarmCount ? DL("Tất cả báo thức đang tắt", "All alarms are off") : DL("Chưa có báo thức nào", "No alarms yet"));
    lv_obj_set_style_text_color(next, C_MUTED, 0);
  } else {
    char a[40];
    alarmAwayText(away, a, sizeof(a));
    setTextFmt(next, LV_SYMBOL_BELL "  %s %s", DL("Báo thức tiếp theo", "Next alarm"), a);
  }
  lv_obj_set_style_pad_left(next, 4, 0);

  for (int i = 0; i < alarmCount; i++) {
    const AlarmItem &a = alarms[i];
    lv_obj_t *card = makeCard(ckAlList, 456, 64);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_border_color(card, C_ACCENT, LV_STATE_PRESSED);
    lv_obj_add_event_cb(
        card, [](lv_event_t *e) { ckEdOpen((int)(intptr_t)lv_event_get_user_data(e)); }, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    char b[24];
    alarmTimeText(a.h, a.m, b, sizeof(b));
    lv_obj_t *tl = makeLabel(card, "", F_HEADING, a.on ? C_TEXT : C_MUTED);  // child 0: the time
    setText(tl, b);
    lv_obj_set_pos(tl, 18, 4);
    alarmRepeatText(a.days, b, sizeof(b));
    lv_obj_t *rl = makeLabel(card, "", F_CAPTION, C_MUTED);
    setText(rl, b);
    lv_obj_set_pos(rl, 19, 40);
    lv_obj_t *sw = lv_switch_create(card);
    lv_obj_set_size(sw, 58, 32);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -16, 0);
    lv_obj_set_style_bg_color(sw, C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, C_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_ext_click_area(sw, 10);
    if (a.on) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, ckAlarmToggle, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)i);
  }

  if (alarmCount < ALARM_MAX) {
    lv_obj_t *add = lv_obj_create(ckAlList);
    lv_obj_remove_style_all(add);
    lv_obj_set_size(add, 456, 48);
    lv_obj_set_style_radius(add, 14, 0);
    lv_obj_set_style_border_width(add, 1, 0);
    lv_obj_set_style_border_color(add, C_ACCENT, 0);
    lv_obj_set_style_border_opa(add, LV_OPA_60, 0);
    lv_obj_set_style_bg_color(add, C_ACCENT, 0);
    lv_obj_set_style_bg_opa(add, LV_OPA_10, 0);
    lv_obj_set_style_bg_opa(add, LV_OPA_30, LV_STATE_PRESSED);
    lv_obj_clear_flag(add, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *al = makeLabel(add, "", F_LABEL, C_ACCENT);
    setTextFmt(al, LV_SYMBOL_PLUS "  %s", DL("Thêm báo thức", "Add alarm"));
    lv_obj_center(al);
    lv_obj_add_event_cb(add, [](lv_event_t *) { ckEdOpen(-1); }, LV_EVENT_CLICKED, nullptr);
  }
}

// ---------- timer tab ----------
static lv_obj_t *ckArc, *ckTmrLbl, *ckTmrState, *ckTmrBtn, *ckTmrPre, *ckTmrMinus, *ckTmrPlus;
static const uint8_t TMR_PRESETS[] = {1, 3, 5, 10, 15, 30};

static void ckTmrPresetMark() {  // highlight the preset that matches the set time, if any
  lv_btnmatrix_clear_btn_ctrl_all(ckTmrPre, LV_BTNMATRIX_CTRL_CHECKED);
  for (int i = 0; i < 6; i++)
    if (tmrTotal == TMR_PRESETS[i] * 60000UL) lv_btnmatrix_set_btn_ctrl(ckTmrPre, i, LV_BTNMATRIX_CTRL_CHECKED);
}
static void ckTmrAdjust(lv_event_t *e) {
  if (tmrEnd || tmrLeft) return;  // only while the timer is idle
  int step = (int)(intptr_t)lv_event_get_user_data(e);
  int32_t v = (int32_t)tmrTotal + step * 60000;
  tmrTotal = v < 60000 ? 60000 : v > 99 * 60000 ? 99 * 60000 : v;
  ckTmrPresetMark();
}

// ---------- stopwatch tab ----------
static const int LAP_MAX = 30;
static uint32_t laps[LAP_MAX];  // lap durations, oldest first
static int lapCount = 0;
static lv_obj_t *ckSwLbl, *ckSwLap, *ckSwMain, *ckSwSide, *ckLapList;

static void ckFmtMs(uint32_t ms, char *out, size_t n) {
  uint32_t s = ms / 1000;
  if (s >= 3600) snprintf(out, n, "%lu:%02lu:%02lu", (unsigned long)(s / 3600), (unsigned long)(s / 60 % 60), (unsigned long)(s % 60));
  else snprintf(out, n, "%02lu:%02lu.%lu", (unsigned long)(s / 60), (unsigned long)(s % 60), (unsigned long)(ms / 100 % 10));
}

static void ckLapsBuild() {
  lv_obj_clean(ckLapList);
  if (!lapCount) {
    lv_obj_t *e = makeLabel(ckLapList, DL("Chạm \"Vòng\" khi đang chạy để ghi lại từng vòng", "Tap \"Lap\" while running to record laps"), F_CAPTION, C_MUTED);
    lv_obj_set_width(e, 160);
    lv_label_set_long_mode(e, LV_LABEL_LONG_WRAP);
    return;
  }
  int fast = 0, slow = 0;
  for (int i = 1; i < lapCount; i++) {
    if (laps[i] < laps[fast]) fast = i;
    if (laps[i] > laps[slow]) slow = i;
  }
  for (int i = lapCount - 1; i >= 0; i--) {  // newest first
    lv_obj_t *row = lv_obj_create(ckLapList);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, 164, 26);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_color_t col = lapCount > 2 && i == fast ? C_OK : lapCount > 2 && i == slow ? C_DANGER : C_TEXT;
    lv_obj_t *n = makeLabel(row, "", F_BODY, C_MUTED);
    setTextFmt(n, "%s %d", DL("Vòng", "Lap"), i + 1);
    lv_obj_align(n, LV_ALIGN_LEFT_MID, 0, 0);
    char b[16];
    ckFmtMs(laps[i], b, sizeof(b));
    lv_obj_t *v = makeLabel(row, "", F_BODY, col);
    setText(v, b);
    lv_obj_align(v, LV_ALIGN_RIGHT_MID, 0, 0);
  }
}

static uint32_t lapsTotal() {
  uint32_t s = 0;
  for (int i = 0; i < lapCount; i++) s += laps[i];
  return s;
}

// ---------- page ----------
static void ckRefresh(lv_timer_t *) {
  struct tm tt;
  static bool wasSynced = false;
  bool synced = clockTimeNow(tt);
  if (synced != wasSynced) {  // the list's header depends on it (time arrives a while after boot)
    wasSynced = synced;
    ckAlarmsDirty = true;
  }
  if (ckAlarmsDirty && !ckEdOv) ckAlarmList();
  // timer
  bool idle = !tmrEnd && !tmrLeft;
  uint32_t r = idle ? tmrTotal : tmrRemaining();
  uint32_t secs = (r + 999) / 1000;  // count down in whole seconds: 00:00 only when it is over
  setTextFmt(ckTmrLbl, "%02lu:%02lu", (unsigned long)(secs / 60), (unsigned long)(secs % 60));
  lv_arc_set_value(ckArc, idle ? 1000 : (int32_t)((uint64_t)r * 1000 / (tmrTotal ? tmrTotal : 1)));
  setText(ckTmrState, tmrEnd ? DL("Đang đếm ngược", "Counting down") : tmrLeft ? DL("Tạm dừng", "Paused") : DL("Sẵn sàng", "Ready"));
  setText(lv_obj_get_child(ckTmrBtn, 0), tmrEnd ? DL("Tạm dừng", "Pause") : tmrLeft ? DL("Tiếp tục", "Resume") : DL("Bắt đầu", "Start"));
  for (lv_obj_t *o : {ckTmrPre, ckTmrMinus, ckTmrPlus}) {
    if (idle) lv_obj_clear_state(o, LV_STATE_DISABLED);
    else lv_obj_add_state(o, LV_STATE_DISABLED);
  }
  // stopwatch
  uint32_t e = swElapsed();
  char b[16];
  ckFmtMs(e, b, sizeof(b));
  setText(ckSwLbl, b);
  if (lapCount || swRun) {
    ckFmtMs(e - lapsTotal(), b, sizeof(b));
    setTextFmt(ckSwLap, "%s %d   %s", DL("Vòng", "Lap"), lapCount + 1, b);
  } else {
    setText(ckSwLap, "");
  }
  setText(lv_obj_get_child(ckSwMain, 0), swRun ? DL("Dừng", "Stop") : e ? DL("Tiếp tục", "Resume") : DL("Bắt đầu", "Start"));
  setText(lv_obj_get_child(ckSwSide, 0), swRun ? DL("Vòng", "Lap") : DL("Đặt lại", "Reset"));
  if (swRun || e) lv_obj_clear_state(ckSwSide, LV_STATE_DISABLED);
  else lv_obj_add_state(ckSwSide, LV_STATE_DISABLED);
}

static void ckShowTab(int i) {
  for (int k = 0; k < 3; k++) {
    if (k == i) lv_obj_clear_flag(ckTabs[k], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ckTabs[k], LV_OBJ_FLAG_HIDDEN);
  }
}

static void pageClock() {
  lv_obj_t *c;
  ckScr = pageBegin("Clock", &c);
  lv_obj_del(c);
  ckEdOv = nullptr;
  lv_obj_set_width(lv_obj_get_child(ckScr, 1), 104);  // the title shares the header row with the tabs

  // tabs: segmented control in the header
  static const char *tabMap[4];
  tabMap[0] = DL("Báo thức", "Alarm");
  tabMap[1] = DL("Hẹn giờ", "Timer");
  tabMap[2] = DL("Bấm giờ", "Stopwatch");
  tabMap[3] = "";
  lv_obj_t *seg = ckChips(ckScr, tabMap, true, [](lv_event_t *e) {
    uint32_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
    if (id < 3) ckShowTab(id);
  });
  lv_btnmatrix_set_btn_ctrl(seg, 0, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_set_style_bg_color(seg, C_CARD, 0);  // one pill holding the three tabs
  lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(seg, 19, 0);
  lv_obj_set_style_border_width(seg, 1, 0);
  lv_obj_set_style_border_color(seg, C_BORDER, 0);
  lv_obj_set_style_pad_all(seg, 3, 0);
  lv_obj_set_style_pad_column(seg, 2, 0);
  lv_obj_set_style_radius(seg, 16, LV_PART_ITEMS);
  lv_obj_set_style_bg_opa(seg, LV_OPA_TRANSP, LV_PART_ITEMS);
  lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_border_width(seg, 0, LV_PART_ITEMS);
  lv_obj_set_style_text_color(seg, C_MUTED, LV_PART_ITEMS);
  lv_obj_set_size(seg, 300, 40);
  lv_obj_set_pos(seg, 168, SB_H + 5);

  for (int i = 0; i < 3; i++) {
    ckTabs[i] = lv_obj_create(ckScr);
    lv_obj_remove_style_all(ckTabs[i]);
    lv_obj_set_size(ckTabs[i], 456, 320 - (SB_H + 54));
    lv_obj_set_pos(ckTabs[i], 12, SB_H + 54);
    lv_obj_clear_flag(ckTabs[i], LV_OBJ_FLAG_SCROLLABLE);
  }

  // alarms: scrolling list of cards
  ckAlList = lv_obj_create(ckTabs[0]);
  lv_obj_remove_style_all(ckAlList);
  lv_obj_set_size(ckAlList, 456, 320 - (SB_H + 54));
  lv_obj_set_flex_flow(ckAlList, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(ckAlList, 8, 0);
  lv_obj_set_style_pad_bottom(ckAlList, 10, 0);
  lv_obj_set_scroll_dir(ckAlList, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(ckAlList, LV_SCROLLBAR_MODE_OFF);
  ckAlarmList();

  // timer: progress ring on the left, settings and controls on the right
  lv_obj_t *tt = ckTabs[1];
  ckArc = lv_arc_create(tt);
  lv_obj_set_size(ckArc, 200, 200);
  lv_obj_set_pos(ckArc, 8, 12);
  lv_arc_set_rotation(ckArc, 270);
  lv_arc_set_bg_angles(ckArc, 0, 360);
  lv_arc_set_range(ckArc, 0, 1000);
  lv_obj_remove_style(ckArc, nullptr, LV_PART_KNOB);
  lv_obj_clear_flag(ckArc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(ckArc, 12, LV_PART_MAIN);
  lv_obj_set_style_arc_color(ckArc, C_BORDER, LV_PART_MAIN);
  lv_obj_set_style_arc_width(ckArc, 12, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(ckArc, C_ACCENT, LV_PART_INDICATOR);
  // fixed width, centred text: aligned while empty, a content-sized label grew to the right over the ring
  ckTmrLbl = makeLabel(tt, "", F_DISPLAY, C_TEXT);
  lv_obj_set_width(ckTmrLbl, 170);
  lv_obj_set_style_text_align(ckTmrLbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align_to(ckTmrLbl, ckArc, LV_ALIGN_CENTER, 0, -10);
  ckTmrState = makeLabel(tt, "", F_CAPTION, C_MUTED);
  lv_obj_set_width(ckTmrState, 150);
  lv_obj_set_style_text_align(ckTmrState, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align_to(ckTmrState, ckArc, LV_ALIGN_CENTER, 0, 30);

  lv_obj_t *pl = makeLabel(tt, "", F_CAPTION, C_MUTED);
  setText(pl, DL("CHỌN NHANH (PHÚT)", "QUICK SET (MIN)"));
  lv_obj_set_pos(pl, 232, 12);
  static const char *preMap[] = {"1", "3", "5", "\n", "10", "15", "30", ""};
  ckTmrPre = ckChips(tt, preMap, true, [](lv_event_t *e) {
    uint32_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
    if (id >= 6 || tmrEnd || tmrLeft) return;
    tmrTotal = TMR_PRESETS[id] * 60000UL;
  });
  lv_obj_set_size(ckTmrPre, 220, 84);
  lv_obj_set_pos(ckTmrPre, 232, 32);
  ckTmrPresetMark();
  ckTmrMinus = ckBtn(tt, "- 1", C_MUTED, false, ckTmrAdjust, 106, 40, (void *)(intptr_t)-1);
  lv_obj_set_pos(ckTmrMinus, 232, 126);
  ckTmrPlus = ckBtn(tt, "+ 1", C_MUTED, false, ckTmrAdjust, 106, 40, (void *)(intptr_t)1);
  lv_obj_set_pos(ckTmrPlus, 346, 126);
  lv_obj_t *trs = ckBtn(tt, LV_SYMBOL_REFRESH, C_MUTED, false, [](lv_event_t *) {
    tmrEnd = 0;
    tmrLeft = 0;
  }, 66, 50);
  lv_obj_set_pos(trs, 232, 180);
  ckTmrBtn = ckBtn(tt, "", C_ACCENT, true, [](lv_event_t *) {
    if (tmrEnd) {  // pause
      tmrLeft = tmrRemaining();
      tmrEnd = 0;
    } else {
      tmrEnd = millis() + (tmrLeft ? tmrLeft : tmrTotal);
      tmrLeft = 0;
    }
  }, 146, 50);
  lv_obj_set_pos(ckTmrBtn, 306, 180);

  // stopwatch: big time + current lap on the left, lap list on the right
  lv_obj_t *st = ckTabs[2];
  lv_obj_t *face = makeCard(st, 262, 160);
  lv_obj_set_pos(face, 0, 6);
  ckSwLbl = makeLabel(face, "", F_DISPLAY, C_TEXT);
  lv_obj_set_width(ckSwLbl, 250);  // fixed width + centred text: the digits do not shift as they change
  lv_obj_set_style_text_align(ckSwLbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(ckSwLbl, LV_ALIGN_CENTER, 0, -14);
  ckSwLap = makeLabel(face, "", F_LABEL, C_ACCENT);
  lv_obj_set_width(ckSwLap, 240);
  lv_obj_set_style_text_align(ckSwLap, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(ckSwLap, LV_ALIGN_CENTER, 0, 40);
  ckSwSide = ckBtn(st, "", C_MUTED, false, [](lv_event_t *) {
    if (swRun) {  // lap
      if (lapCount == LAP_MAX) {
        memmove(laps, laps + 1, sizeof(uint32_t) * (LAP_MAX - 1));  // keep the latest LAP_MAX laps
        lapCount--;
      }
      uint32_t e = swElapsed(), done = lapsTotal();
      laps[lapCount++] = e - done;
    } else {  // reset
      swAcc = 0;
      lapCount = 0;
    }
    ckLapsBuild();
  }, 124, 50);
  lv_obj_set_pos(ckSwSide, 0, 180);
  ckSwMain = ckBtn(st, "", C_ACCENT, true, [](lv_event_t *) {
    if (swRun) swAcc += millis() - swStart;
    else swStart = millis();
    swRun = !swRun;
  }, 130, 50);
  lv_obj_set_pos(ckSwMain, 132, 180);
  lv_obj_t *lapCard = makeCard(st, 184, 224);
  lv_obj_set_pos(lapCard, 272, 6);
  lv_obj_t *lt = makeLabel(lapCard, "", F_CAPTION, C_MUTED);
  setText(lt, DL("CÁC VÒNG", "LAPS"));
  lv_obj_set_pos(lt, 10, 8);
  ckLapList = lv_obj_create(lapCard);
  lv_obj_remove_style_all(ckLapList);
  lv_obj_set_size(ckLapList, 168, 190);
  lv_obj_set_pos(ckLapList, 10, 28);
  lv_obj_set_flex_flow(ckLapList, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scroll_dir(ckLapList, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(ckLapList, LV_SCROLLBAR_MODE_OFF);
  ckLapsBuild();

  ckShowTab(0);
  ckRefresh(nullptr);
  pageTimer(ckScr, ckRefresh, 100);
  pageShow(ckScr);
}
