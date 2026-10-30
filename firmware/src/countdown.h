#pragma once
// Countdown: days left until the dates that matter. An event happens once (a solar date with its year), every year on
// a solar date (birthdays) or every year on a lunar date (Tết, Trung thu, death anniversaries). The list is sorted by
// how soon each comes. The home screen shows, in turn, every coming event whose "home" switch is on (the switch on its
// card). Stored in NVS namespace "cd".
// Included by main.cpp after clock_app.h (ckChips, ckBtn, ckRoller) and calendar_app.h.

struct CdEvent {
  char name[40];
  int16_t y;      // year (once only)
  uint8_t m, d;   // month and day (lunar for CD_LUNAR)
  uint8_t kind : 2;    // CD_ONCE, CD_YEARLY, CD_LUNAR
  uint8_t hidden : 1;  // not shown on the home screen (shares the byte with kind: older saves read as shown)
};
enum { CD_ONCE, CD_YEARLY, CD_LUNAR };
static const int CD_MAX = 12;
static CdEvent cdEv[CD_MAX];
static int cdCount = 0;
static bool cdLoaded = false;
static long cdNextJd[CD_MAX];  // next occurrence (Julian day), worked out once per day
static long cdCacheDay = -1;

static void cdSave() {
  Preferences p;
  p.begin("cd", false);
  p.putUChar("n", cdCount);
  p.putBytes("ev", cdEv, sizeof(CdEvent) * cdCount);
  p.end();
  cdCacheDay = -1;
}

static void cdLoad() {
  if (cdLoaded) return;
  cdLoaded = true;
  Preferences p;
  p.begin("cd", true);
  bool has = p.isKey("n");
  cdCount = has ? min((int)p.getUChar("n", 0), CD_MAX) : 0;
  if (cdCount) p.getBytes("ev", cdEv, sizeof(CdEvent) * cdCount);
  p.end();
  if (!has) {  // first run: the big holidays to start with
    const CdEvent seed[] = {{"", 0, 1, 1, CD_LUNAR}, {"", 0, 8, 15, CD_LUNAR}, {"", 0, 12, 25, CD_YEARLY}};
    const char *names[] = {DL("Tết Nguyên Đán", "Lunar New Year"), DL("Tết Trung thu", "Mid-Autumn Festival"), DL("Giáng sinh", "Christmas")};
    for (int i = 0; i < 3; i++) {
      cdEv[i] = seed[i];
      strlcpy(cdEv[i].name, names[i], sizeof(cdEv[i].name));
    }
    cdCount = 3;
    cdSave();
  }
}

// Today as a Julian day, 0 while the clock is not set.
static long cdToday() {
  struct tm t;
  if (time(nullptr) < 1700000000 || !getLocalTime(&t, 0)) return 0;
  return lunar::jdFromDate(t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
}

static long cdSolarJd(int d, int m, int y) {
  int dim = (int)(lunar::jdFromDate(1, m == 12 ? 1 : m + 1, m == 12 ? y + 1 : y) - lunar::jdFromDate(1, m, y));
  return lunar::jdFromDate(min(d, dim), m, y);  // 29/2 in a common year: the 28th
}

static long cdNext(const CdEvent &e, long today) {
  int dd, mm, yy;
  jdToDate(today, dd, mm, yy);
  if (e.kind == CD_ONCE) return cdSolarJd(e.d, e.m, e.y);
  if (e.kind == CD_YEARLY) {
    long jd = cdSolarJd(e.d, e.m, yy);
    return jd >= today ? jd : cdSolarJd(e.d, e.m, yy + 1);
  }
  int ly = solarToLunar(dd, mm, yy).year;  // lunar: this lunar year's date, or next year's
  for (int y = ly; y <= ly + 1; y++) {
    long start = lunarMonthStart(e.m, y, false);
    if (!start) continue;
    long jd = start + min((int)e.d, lunarMonthLength(start)) - 1;
    if (jd >= today) return jd;
  }
  return 0;
}

static void cdRefresh(long today) {
  if (today == cdCacheDay) return;
  cdCacheDay = today;
  for (int i = 0; i < cdCount; i++) cdNextJd[i] = cdNext(cdEv[i], today);
}

// Events by how soon they come; past one-off events last.
static void cdOrder(long today, int *idx) {
  for (int i = 0; i < cdCount; i++) idx[i] = i;
  auto key = [&](int i) { long d = cdNextJd[i] - today; return d < 0 ? 100000 - d : d; };
  for (int i = 1; i < cdCount; i++)
    for (int j = i; j > 0 && key(idx[j]) < key(idx[j - 1]); j--) std::swap(idx[j], idx[j - 1]);
}

static void cdDaysText(long days, char *out, size_t n) {
  if (days == 0) strlcpy(out, DL("hôm nay!", "today!"), n);
  else if (days == 1) strlcpy(out, DL("ngày mai", "tomorrow"), n);
  else if (days > 0) snprintf(out, n, isVi() ? "còn %ld ngày" : "%ld days left", days);
  else snprintf(out, n, isVi() ? "đã qua %ld ngày" : "%ld days ago", -days);
}

// The events the home screen takes turns with, nearest first: coming (not past) and switched on. Returns the count.
static int cdHomeList(int *out) {
  cdLoad();
  long today = cdToday();
  if (!today || !cdCount) return 0;
  cdRefresh(today);
  int idx[CD_MAX], n = 0;
  cdOrder(today, idx);
  for (int k = 0; k < cdCount; k++)
    if (!cdEv[idx[k]].hidden && cdNextJd[idx[k]] >= today) out[n++] = idx[k];
  return n;
}

static long cdDaysLeft(int i) { return cdNextJd[i] - cdToday(); }  // after cdHomeList()
static const char *cdName(int i) { return cdEv[i].name; }

// "Tết Nguyên Đán - còn 89 ngày" for event i.
static void cdHomeText(int i, char *out, size_t n) {
  char d[32];
  cdDaysText(cdNextJd[i] - cdToday(), d, sizeof(d));
  snprintf(out, n, "%s  -  %s", cdEv[i].name, d);
}

// ---------- the page ----------
static lv_obj_t *cdScr, *cdList, *cdEdOv, *cdEdName, *cdEdKb, *cdRollD, *cdRollM, *cdRollY, *cdEdKind;
static int cdEdIdx = -1, cdEdYear0 = 2026;
static void cdBuildList();

static const char *cdKindText(const CdEvent &e, char *out, size_t n) {
  if (e.kind == CD_ONCE) snprintf(out, n, "%d/%d/%d", e.d, e.m, e.y);
  else if (e.kind == CD_YEARLY) snprintf(out, n, isVi() ? "%d/%d hằng năm" : "%d/%d every year", e.d, e.m);
  else snprintf(out, n, isVi() ? "%d/%d âm lịch hằng năm" : "%d/%d lunar, every year", e.d, e.m);
  return out;
}

static void cdEdClose() {
  if (cdEdOv) lv_obj_del(cdEdOv);
  cdEdOv = nullptr;
}

static int cdEdKindSel() {  // the checked chip (the "selected" one is only the last pressed)
  for (int i = 0; i < 3; i++)
    if (lv_btnmatrix_has_btn_ctrl(cdEdKind, i, LV_BTNMATRIX_CTRL_CHECKED)) return i;
  return CD_ONCE;
}

static void cdEdShowYear() {
  int kind = cdEdKindSel();
  if (kind == CD_ONCE) lv_obj_clear_flag(cdRollY, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(cdRollY, LV_OBJ_FLAG_HIDDEN);
}

static void cdEdSave(lv_event_t *) {
  CdEvent e = {};
  if (cdEdIdx >= 0) e.hidden = cdEv[cdEdIdx].hidden;  // the home switch lives on the card
  const char *nm = lv_textarea_get_text(cdEdName);
  strlcpy(e.name, nm[0] ? nm : DL("Sự kiện", "Event"), sizeof(e.name));
  e.kind = cdEdKindSel();
  e.d = lv_roller_get_selected(cdRollD) + 1;
  e.m = lv_roller_get_selected(cdRollM) + 1;
  e.y = cdEdYear0 + lv_roller_get_selected(cdRollY);
  if (e.kind == CD_LUNAR && e.d > 30) e.d = 30;
  if (cdEdIdx >= 0) {
    cdEv[cdEdIdx] = e;
  } else if (cdCount < CD_MAX) {
    cdEv[cdCount++] = e;
  }
  cdSave();
  cdEdClose();
  cdBuildList();
  soundPlay(SND(SND_OK), 0, false);
}

static void cdEdDelete(lv_event_t *) {
  if (cdEdIdx < 0 || cdEdIdx >= cdCount) return;
  for (int i = cdEdIdx; i < cdCount - 1; i++) cdEv[i] = cdEv[i + 1];
  cdCount--;
  cdSave();
  cdEdClose();
  cdBuildList();
}

static void cdEdOpen(int idx) {
  static char dOpts[31 * 3], mOpts[12 * 3], yOpts[11 * 5];
  long today = cdToday();
  int td = 1, tm = 1, ty = 2026;
  if (today) jdToDate(today, td, tm, ty);
  cdEdYear0 = ty;
  if (!dOpts[0]) {
    for (int i = 1; i <= 31; i++) snprintf(dOpts + (i - 1) * 3, 4, i < 31 ? "%02d\n" : "%02d", i);
    for (int i = 1; i <= 12; i++) snprintf(mOpts + (i - 1) * 3, 4, i < 12 ? "%02d\n" : "%02d", i);
  }
  for (int i = 0; i <= 10; i++) snprintf(yOpts + i * 5, 6, i < 10 ? "%04d\n" : "%04d", ty + i);
  cdEdIdx = idx;
  CdEvent e = idx >= 0 ? cdEv[idx] : CdEvent{"", (int16_t)ty, (uint8_t)tm, (uint8_t)td, CD_ONCE};

  cdEdOv = lv_obj_create(cdScr);
  lv_obj_remove_style_all(cdEdOv);
  lv_obj_set_size(cdEdOv, 480, 320);
  lv_obj_set_style_bg_color(cdEdOv, lv_color_hex(0x0B1024), 0);
  lv_obj_set_style_bg_opa(cdEdOv, LV_OPA_COVER, 0);
  lv_obj_add_flag(cdEdOv, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(cdEdOv, LV_OBJ_FLAG_SCROLLABLE);

  // name, with ready-made names (the on-screen keyboard has no Vietnamese accents)
  cdEdName = lv_textarea_create(cdEdOv);
  lv_textarea_set_one_line(cdEdName, true);
  lv_textarea_set_max_length(cdEdName, 36);
  lv_textarea_set_placeholder_text(cdEdName, DL("Tên sự kiện", "Event name"));
  lv_textarea_set_text(cdEdName, e.name);
  lv_obj_set_style_text_font(cdEdName, F_BODY, 0);
  lv_obj_set_size(cdEdName, 456, 40);
  lv_obj_set_pos(cdEdName, 12, SB_H + 6);
  lv_obj_set_style_bg_color(cdEdName, C_CARD, 0);
  lv_obj_set_style_text_color(cdEdName, C_TEXT, 0);
  lv_obj_set_style_border_color(cdEdName, C_BORDER, 0);
  lv_obj_set_style_border_width(cdEdName, 1, 0);
  lv_obj_add_event_cb(cdEdName, [](lv_event_t *) { lv_obj_clear_flag(cdEdKb, LV_OBJ_FLAG_HIDDEN); }, LV_EVENT_FOCUSED, nullptr);
  static const char *presetMap[6];
  presetMap[0] = DL("Sinh nhật", "Birthday");
  presetMap[1] = DL("Kỷ niệm", "Anniversary");
  presetMap[2] = DL("Ngày giỗ", "Memorial");
  presetMap[3] = DL("Du lịch", "Trip");
  presetMap[4] = DL("Kỳ thi", "Exam");
  presetMap[5] = "";
  lv_obj_t *presets = ckChips(cdEdOv, presetMap, false, [](lv_event_t *e) {
    lv_obj_t *m = lv_event_get_target(e);
    uint16_t id = lv_btnmatrix_get_selected_btn(m);
    lv_btnmatrix_clear_btn_ctrl_all(m, LV_BTNMATRIX_CTRL_CHECKED);
    if (id > 4) return;
    lv_textarea_set_text(cdEdName, lv_btnmatrix_get_btn_text(m, id));
    if (id == 2) lv_btnmatrix_set_btn_ctrl(cdEdKind, CD_LUNAR, LV_BTNMATRIX_CTRL_CHECKED);  // memorials follow the lunar calendar
    else if (id < 2) lv_btnmatrix_set_btn_ctrl(cdEdKind, CD_YEARLY, LV_BTNMATRIX_CTRL_CHECKED);
    cdEdShowYear();
  });
  lv_obj_set_size(presets, 456, 34);
  lv_obj_set_pos(presets, 12, SB_H + 52);

  // how it repeats
  static const char *kindMap[4];
  kindMap[0] = DL("Một lần", "Once");
  kindMap[1] = DL("Hằng năm", "Every year");
  kindMap[2] = DL("Âm lịch hằng năm", "Lunar, every year");
  kindMap[3] = "";
  cdEdKind = ckChips(cdEdOv, kindMap, true, [](lv_event_t *) { cdEdShowYear(); });
  lv_btnmatrix_set_btn_ctrl(cdEdKind, e.kind, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_set_size(cdEdKind, 456, 36);
  lv_obj_set_pos(cdEdKind, 12, SB_H + 92);

  // date: day / month / year rollers
  cdRollD = ckRoller(cdEdOv, dOpts, e.d - 1);
  lv_obj_set_pos(cdRollD, 12, SB_H + 136);
  cdRollM = ckRoller(cdEdOv, mOpts, e.m - 1);
  lv_obj_set_pos(cdRollM, 116, SB_H + 136);
  cdRollY = ckRoller(cdEdOv, yOpts, constrain(e.y - ty, 0, 10));
  lv_obj_set_width(cdRollY, 116);
  lv_obj_set_pos(cdRollY, 220, SB_H + 136);
  cdEdShowYear();

  if (idx >= 0) {
    lv_obj_t *del = ckBtn(cdEdOv, LV_SYMBOL_TRASH, C_DANGER, false, cdEdDelete, 120, 40);
    lv_obj_set_pos(del, 348, SB_H + 136);
  }
  lv_obj_t *cancel = ckBtn(cdEdOv, DL("Huỷ", "Cancel"), C_MUTED, false, [](lv_event_t *) { cdEdClose(); }, 120, 40);
  lv_obj_set_pos(cancel, 348, SB_H + 182);
  lv_obj_t *save = ckBtn(cdEdOv, DL("Lưu", "Save"), C_ACCENT, true, cdEdSave, 120, 40);
  lv_obj_set_pos(save, 348, SB_H + 228);

  // keyboard over the lower half while the name is being typed
  cdEdKb = lv_keyboard_create(cdEdOv);
  lv_obj_set_size(cdEdKb, 480, 190);
  lv_obj_align(cdEdKb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(cdEdKb, C_CARD, 0);
  lv_obj_set_style_border_width(cdEdKb, 0, 0);
  lv_obj_set_style_bg_color(cdEdKb, C_BORDER, LV_PART_ITEMS);
  lv_obj_set_style_text_color(cdEdKb, C_TEXT, LV_PART_ITEMS);
  lv_obj_set_style_border_width(cdEdKb, 0, LV_PART_ITEMS);
  lv_keyboard_set_textarea(cdEdKb, cdEdName);
  lv_obj_add_event_cb(cdEdKb, [](lv_event_t *) {
    lv_obj_add_flag(cdEdKb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_state(cdEdName, LV_STATE_FOCUSED);
  }, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(cdEdKb, [](lv_event_t *) {
    lv_obj_add_flag(cdEdKb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_state(cdEdName, LV_STATE_FOCUSED);
  }, LV_EVENT_CANCEL, nullptr);
  lv_obj_add_flag(cdEdKb, LV_OBJ_FLAG_HIDDEN);
}

static void cdBuildList() {
  lv_obj_clean(cdList);
  long today = cdToday();
  if (today) cdRefresh(today);
  int idx[CD_MAX];
  if (today) cdOrder(today, idx);
  else
    for (int i = 0; i < cdCount; i++) idx[i] = i;
  for (int k = 0; k < cdCount; k++) {
    int i = idx[k];
    const CdEvent &e = cdEv[i];
    long days = today ? cdNextJd[i] - today : 0;
    bool past = today && days < 0, now = today && days == 0;
    lv_obj_t *card = makeCard(cdList, 456, 62);
    if (now) {
      lv_obj_set_style_border_color(card, lv_color_hex(0xFFC93C), 0);
      lv_obj_set_style_border_width(card, 2, 0);
    }
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(card, C_BORDER, LV_STATE_PRESSED);
    lv_obj_add_event_cb(card, [](lv_event_t *ev) { cdEdOpen((int)(intptr_t)lv_event_get_user_data(ev)); }, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    // days: a big number in a coloured badge
    uint32_t col = past ? 0x5C668F : now ? 0xFFC93C : days <= 7 ? 0xFF7A90 : days <= 30 ? 0xFFB84A : 0x38E1FF;
    lv_obj_t *badge = shape(card, 8, 7, 86, 46, col, 12, LV_OPA_20);
    lv_obj_t *num = makeLabel(badge, "", F_HEADING, lv_color_hex(col));
    if (!today) setText(num, "--");
    else if (now) setText(num, LV_SYMBOL_BELL);
    else setTextFmt(num, "%ld", days < 0 ? -days : days);
    lv_obj_center(num);
    lv_obj_t *nm = makeLabel(card, "", F_LABEL, past ? C_MUTED : C_TEXT);
    lv_obj_set_width(nm, 270);
    lv_label_set_long_mode(nm, LV_LABEL_LONG_DOT);
    lv_label_set_text(nm, e.name);
    lv_obj_set_pos(nm, 106, 9);
    char a[48], b[32], line[96];
    cdKindText(e, a, sizeof(a));
    if (today) {
      cdDaysText(days, b, sizeof(b));
      int dd, mm, yy;
      jdToDate(cdNextJd[i], dd, mm, yy);
      if (e.kind == CD_ONCE) snprintf(line, sizeof(line), "%s  -  %s", b, a);
      else snprintf(line, sizeof(line), "%s  -  %d/%d/%d  (%s)", b, dd, mm, yy, a);
    } else {
      strlcpy(line, a, sizeof(line));
    }
    lv_obj_t *sub = makeLabel(card, "", F_CAPTION, now ? lv_color_hex(0xFFC93C) : C_MUTED);
    lv_obj_set_width(sub, 270);
    lv_label_set_long_mode(sub, LV_LABEL_LONG_DOT);
    lv_label_set_text(sub, line);
    lv_obj_set_pos(sub, 106, 36);
    // on the home screen or not (its own tap: it does not open the editor)
    lv_obj_t *hl = makeLabel(card, "", F_CAPTION, C_MUTED);
    lv_label_set_text(hl, DL("Màn hình chính", "Home screen"));
    lv_obj_align(hl, LV_ALIGN_TOP_RIGHT, -10, 4);
    lv_obj_t *sw = lv_switch_create(card);
    lv_obj_set_size(sw, 46, 24);
    lv_obj_align(sw, LV_ALIGN_BOTTOM_RIGHT, -14, -6);
    lv_obj_set_ext_click_area(sw, 8);
    lv_obj_set_style_bg_color(sw, C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, C_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (!e.hidden) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, [](lv_event_t *ev) {
      int i = (int)(intptr_t)lv_event_get_user_data(ev);
      cdEv[i].hidden = !lv_obj_has_state(lv_event_get_target(ev), LV_STATE_CHECKED);
      cdSave();
    }, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)i);
  }
  if (!cdCount) {
    lv_obj_t *h = makeLabel(cdList, "", F_BODY, C_MUTED);
    lv_label_set_text(h, DL("Chưa có sự kiện. Bấm + để thêm.", "No events yet. Press + to add one."));
  }
}

static void pageCountdown() {
  cdLoad();
  lv_obj_t *c;
  cdScr = pageBegin(DL("Đếm ngược", "Countdown"), &c);
  cdList = c;
  cdEdOv = nullptr;
  lv_obj_add_event_cb(cdScr, [](lv_event_t *) { cdScr = nullptr; }, LV_EVENT_DELETE, nullptr);
  lv_obj_t *add = ckBtn(cdScr, LV_SYMBOL_PLUS, C_ACCENT, true, [](lv_event_t *) {
    if (cdCount >= CD_MAX) {
      uiToast(DL("Đã đủ 12 sự kiện", "12 events at most"), C_WARN);
      return;
    }
    cdEdOpen(-1);
  }, 48, 38);
  lv_obj_align(add, LV_ALIGN_TOP_RIGHT, -12, SB_H + 6);
  cdBuildList();
  pageShow(cdScr);
}
