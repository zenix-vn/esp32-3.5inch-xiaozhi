#pragma once
// Calendar app: month view with the lunar date under every day, holidays, swipe / arrows to change month, a
// summary of the selected day and a detail page (Can Chi, solar term, Trực, Sao, moon, lucky hours, what to do
// and what to avoid). The algorithms live in lunar_core.h / almanac.h (checked against the LichSo project).
// The month grid is ONE object that draws its 42 cells itself: no widget per day, so it costs almost no RAM.
#include "almanac.h"

static const int CAL_CW = 44, CAL_CH = 36, CAL_HEAD = 22, CAL_PAD = 7;  // cell size, weekday header height, side padding
static const int CAL_GW = 7 * CAL_CW + 2 * CAL_PAD, CAL_GH = CAL_HEAD + 6 * CAL_CH + 6;  // grid panel size
static const int CAL_MIN_Y = 1901, CAL_MAX_Y = 2099;
static const uint32_t CC_RED = 0xFF7A8C, CC_AMBER = 0xFFC93C, CC_DARK = 0x04101A, CC_DIM = 0x5C668F;

enum { CF_OTHER = 1, CF_LEAP = 2, CF_HOL = 4, CF_OFF = 8, CF_SUN = 16 };
struct CalCell {
  uint8_t d, m, ld, lm, flags;
  int16_t y;
};
static CalCell calCells[42];
static int calY, calM, calRows = 5;          // month shown
static int calSelY, calSelM, calSelD;        // selected day
static int calTodayY, calTodayM, calTodayD;  // today (refreshed every 30 s)
static lv_obj_t *calScr, *calGrid, *calMonthLbl, *calTodayBtn;
static lv_obj_t *calBig, *calDow, *calLunar, *calCan, *calHol;

static void pageCalendar();
static void calOpenDetail();

// ---------- dates ----------
static const char *const CAL_WD_VI[7] = {"T2", "T3", "T4", "T5", "T6", "T7", "CN"};  // Monday first
static const char *const CAL_WD_EN[7] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
static const char *const CAL_WDL_VI[7] = {"Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy", "Chủ Nhật"};
static const char *const CAL_WDL_EN[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

static void calNow(int &y, int &m, int &d) {
  struct tm t;
  if (time(nullptr) > 1700000000 && getLocalTime(&t, 0)) {
    y = t.tm_year + 1900;
    m = t.tm_mon + 1;
    d = t.tm_mday;
    return;
  }
  static const char MON[] = "JanFebMarAprMayJunJulAugSepOctNovDec";  // not synced yet: the build date, "Oct 21 2026"
  char mon[4] = "";
  sscanf(__DATE__, "%3s %d %d", mon, &d, &y);
  const char *p = strstr(MON, mon);
  m = p ? (int)(p - MON) / 3 + 1 : 1;
}

static int calDaysIn(int y, int m) {
  int ny = m == 12 ? y + 1 : y, nm = m == 12 ? 1 : m + 1;
  return (int)(lunar::jdFromDate(1, nm, ny) - lunar::jdFromDate(1, m, y));
}

// Lunar month of the previous cell is reused while the days stay inside it (a full conversion costs ~15 ms).
static LunarDate calLc;
static int calLcLen = 0;
static void calLunarOf(long jd, int d, int m, int y, int &ld, int &lm, bool &leap, int &len) {
  if (!calLcLen || jd < calLc.start || jd >= calLc.start + calLcLen) {
    calLc = solarToLunar(d, m, y);
    calLcLen = lunarMonthLength(calLc.start);
  }
  ld = (int)(jd - calLc.start) + 1;
  lm = calLc.month;
  leap = calLc.leap;
  len = calLcLen;
}

static void calBuild() {
  long first = lunar::jdFromDate(1, calM, calY);
  int dow = alm::weekday(first), dim = calDaysIn(calY, calM);
  calRows = dow + dim > 35 ? 6 : 5;
  calLcLen = 0;
  for (int i = 0; i < 42; i++) {
    long jd = first - dow + i;
    int d, m, y, ld, lm, len;
    bool leap;
    jdToDate(jd, d, m, y);
    calLunarOf(jd, d, m, y, ld, lm, leap, len);
    CalCell &c = calCells[i];
    c.d = d;
    c.m = m;
    c.y = y;
    c.ld = ld;
    c.lm = lm;
    c.flags = (m != calM ? CF_OTHER : 0) | (leap ? CF_LEAP : 0) | (i % 7 == 6 ? CF_SUN : 0);
    const alm::Holiday *h[3];
    int n = alm::holidaysOn(d, m, ld, lm, leap, len, h);
    for (int k = 0; k < n; k++) {
      c.flags |= CF_HOL;
      if (h[k]->off) c.flags |= CF_OFF;
    }
  }
}

// ---------- drawing ----------
static void calDraw(lv_event_t *e) {
  lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);
  const lv_area_t *clip = ctx->clip_area;
  lv_area_t g;
  lv_obj_get_coords(calGrid, &g);

  lv_draw_rect_dsc_t bg;  // the month sits on a soft dark panel instead of loose numbers on the wallpaper
  lv_draw_rect_dsc_init(&bg);
  bg.radius = 16;
  bg.bg_color = lv_color_hex(0x000000);
  bg.bg_opa = LV_OPA_30;
  lv_draw_rect(ctx, &bg, &g);

  lv_draw_label_dsc_t ld;
  lv_draw_label_dsc_init(&ld);
  ld.align = LV_TEXT_ALIGN_CENTER;
  if (g.y1 + CAL_HEAD + 4 >= clip->y1) {  // weekday header
    ld.font = F_CAPTION;
    for (int c = 0; c < 7; c++) {
      lv_area_t a = {(lv_coord_t)(g.x1 + CAL_PAD + c * CAL_CW), (lv_coord_t)(g.y1 + 6), (lv_coord_t)(g.x1 + CAL_PAD + (c + 1) * CAL_CW - 1), (lv_coord_t)(g.y1 + CAL_HEAD + 4)};
      ld.color = c == 6 ? lv_color_hex(CC_RED) : lv_color_hex(0x9AA3C7);
      lv_draw_label(ctx, &ld, &a, isVi() ? CAL_WD_VI[c] : CAL_WD_EN[c], nullptr);
    }
  }
  for (int r = 0; r < calRows; r++) {
    int y1 = g.y1 + CAL_HEAD + 4 + r * CAL_CH, y2 = y1 + CAL_CH - 1;
    if (y2 < clip->y1 || y1 > clip->y2) continue;
    for (int c = 0; c < 7; c++) {
      const CalCell &k = calCells[r * 7 + c];
      lv_area_t a = {(lv_coord_t)(g.x1 + CAL_PAD + c * CAL_CW), (lv_coord_t)y1, (lv_coord_t)(g.x1 + CAL_PAD + (c + 1) * CAL_CW - 1), (lv_coord_t)y2};
      bool sel = k.y == calSelY && k.m == calSelM && k.d == calSelD;
      bool today = k.y == calTodayY && k.m == calTodayM && k.d == calTodayD;
      bool other = k.flags & CF_OTHER;
      if (sel || today) {
        lv_draw_rect_dsc_t rd;
        lv_draw_rect_dsc_init(&rd);
        rd.radius = 10;
        rd.bg_opa = sel ? LV_OPA_COVER : LV_OPA_TRANSP;
        rd.bg_color = C_ACCENT;
        rd.border_width = sel ? 0 : 1;
        rd.border_color = C_ACCENT;
        lv_area_t b = {(lv_coord_t)(a.x1 + 2), (lv_coord_t)(a.y1 + 1), (lv_coord_t)(a.x2 - 2), (lv_coord_t)(a.y2 - 1)};
        lv_draw_rect(ctx, &rd, &b);
      }
      lv_color_t numCol = sel ? lv_color_hex(CC_DARK) : other ? lv_color_hex(0x5C668F) : (k.flags & (CF_OFF | CF_SUN)) ? lv_color_hex(CC_RED) : today ? C_ACCENT : C_TEXT;
      lv_color_t lunCol = sel ? lv_color_hex(CC_DARK) : other ? lv_color_hex(0x434B70) : k.ld == 1 ? lv_color_hex(CC_AMBER) : lv_color_hex(0x7F89B5);
      char b[12];
      snprintf(b, sizeof(b), "%d", k.d);
      lv_area_t na = {a.x1, (lv_coord_t)(a.y1 + 1), a.x2, (lv_coord_t)(a.y1 + 21)};
      ld.font = F_LABEL;
      ld.color = numCol;
      lv_draw_label(ctx, &ld, &na, b, nullptr);
      if (k.ld == 1) snprintf(b, sizeof(b), (k.flags & CF_LEAP) ? "1/%dn" : "1/%d", k.lm);
      else snprintf(b, sizeof(b), "%d", k.ld);
      lv_area_t la = {a.x1, (lv_coord_t)(a.y1 + 20), a.x2, (lv_coord_t)(a.y2)};
      ld.font = F_CAPTION;
      ld.color = lunCol;
      lv_draw_label(ctx, &ld, &la, b, nullptr);
      if ((k.flags & CF_HOL) && !other) {  // holiday dot (red: public day off)
        lv_draw_rect_dsc_t dd;
        lv_draw_rect_dsc_init(&dd);
        dd.radius = LV_RADIUS_CIRCLE;
        dd.bg_opa = LV_OPA_COVER;
        dd.bg_color = sel ? lv_color_hex(CC_DARK) : (k.flags & CF_OFF) ? lv_color_hex(0xFF5C7A) : lv_color_hex(0xFFB84A);
        lv_area_t da = {(lv_coord_t)(a.x2 - 10), (lv_coord_t)(a.y1 + 5), (lv_coord_t)(a.x2 - 6), (lv_coord_t)(a.y1 + 9)};
        lv_draw_rect(ctx, &dd, &da);
      }
    }
  }
}

// ---------- selected day summary (right panel) ----------
static void calSummary() {
  long jd = lunar::jdFromDate(calSelD, calSelM, calSelY);
  LunarDate l = solarToLunar(calSelD, calSelM, calSelY);
  int len = lunarMonthLength(l.start), wd = alm::weekday(jd);
  setTextFmt(calBig, "%d", calSelD);
  setText(calDow, isVi() ? CAL_WDL_VI[wd] : CAL_WDL_EN[wd]);
  setTextFmt(calLunar, isVi() ? "Âm lịch %d/%d%s" : "Lunar %d/%d%s", l.day, l.month, l.leap ? (isVi() ? " nhuận" : " leap") : "");
  setTextFmt(calCan, isVi() ? "Ngày %s %s" : "Day %s %s", CAN[alm::dayCan(jd)], CHI[alm::dayChi(jd)]);
  const alm::Holiday *h[3];
  int n = alm::holidaysOn(calSelD, calSelM, l.day, l.month, l.leap, len, h);
  String s;
  for (int i = 0; i < n; i++) {
    if (i) s += ", ";
    s += isVi() ? h[i]->vi : h[i]->en;
  }
  setText(calHol, s.c_str());
}

static void calSelect(int y, int m, int d) {
  calSelY = y;
  calSelM = m;
  calSelD = d;
  calSummary();
  lv_obj_invalidate(calGrid);
  bool onToday = y == calTodayY && m == calTodayM && d == calTodayD;  // "Today" is shown only when it would change something
  if (onToday) lv_obj_add_flag(calTodayBtn, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(calTodayBtn, LV_OBJ_FLAG_HIDDEN);
}

static void calSetMonth(int y, int m, int selDay = 0) {
  if (m < 1) {
    m = 12;
    y--;
  } else if (m > 12) {
    m = 1;
    y++;
  }
  if (y < CAL_MIN_Y || y > CAL_MAX_Y) return;
  calY = y;
  calM = m;
  calBuild();
  if (isVi()) setTextFmt(calMonthLbl, "Tháng %d, %d", m, y);
  else setTextFmt(calMonthLbl, "%s %d", MONTHS_EN[m - 1], y);
  if (!selDay) {  // keep the day number (or today, when it is in this month)
    bool thisMonth = y == calTodayY && m == calTodayM;
    selDay = thisMonth ? calTodayD : (calSelD < calDaysIn(y, m) ? calSelD : calDaysIn(y, m));
  }
  calSelect(y, m, selDay);
}

static void calGoToday() {
  calNow(calTodayY, calTodayM, calTodayD);
  calSetMonth(calTodayY, calTodayM, calTodayD);
}

static lv_point_t calPress;
static void calGridClick(lv_event_t *) {
  lv_indev_t *in = lv_indev_get_act();
  if (!in) return;
  lv_point_t p;
  lv_indev_get_point(in, &p);
  if (abs(p.x - calPress.x) > 20 || abs(p.y - calPress.y) > 20) return;  // that was a swipe
  lv_area_t g;
  lv_obj_get_coords(calGrid, &g);
  int c = (p.x - g.x1 - CAL_PAD) / CAL_CW, r = (p.y - g.y1 - CAL_HEAD - 4) / CAL_CH;
  if (p.x < g.x1 + CAL_PAD || p.y < g.y1 + CAL_HEAD + 4 || c < 0 || c > 6 || r < 0 || r >= calRows) return;
  const CalCell &k = calCells[r * 7 + c];
  soundPlay(SND(SND_TAP), 0, false);
  if (k.y == calSelY && k.m == calSelM && k.d == calSelD) calOpenDetail();  // second tap on the same day
  else if (k.flags & CF_OTHER) calSetMonth(k.y, k.m, k.d);
  else calSelect(k.y, k.m, k.d);
}

static void calGridGesture(lv_event_t *) {
  lv_indev_t *in = lv_indev_get_act();
  if (!in) return;
  lv_dir_t dir = lv_indev_get_gesture_dir(in);
  if (dir == LV_DIR_LEFT) calSetMonth(calY, calM + 1);
  else if (dir == LV_DIR_RIGHT) calSetMonth(calY, calM - 1);
}

// ---------- detail page ----------
static int calDY, calDM, calDD;
static lv_obj_t *calDetBox = nullptr;
static void calDetFill();

static void calDetBack(lv_event_t *) {
  if (calScr) lv_scr_load_anim(calScr, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 250, 0, true);  // deletes the detail page
  else goBack(nullptr);
}

static void calDetStep(int delta) {
  long jd = lunar::jdFromDate(calDD, calDM, calDY) + delta;
  int d, m, y;
  jdToDate(jd, d, m, y);
  if (y < CAL_MIN_Y || y > CAL_MAX_Y) return;
  calDY = y;
  calDM = m;
  calDD = d;
  calDetFill();
}

static const char *calRate(alm::Rating r) {
  return r == alm::R_GOOD ? DL("tốt", "good") : r == alm::R_BAD ? DL("xấu", "bad") : DL("bình thường", "neutral");
}
static lv_color_t calRateCol(alm::Rating r) { return r == alm::R_GOOD ? C_OK : r == alm::R_BAD ? C_DANGER : C_WARN; }

// A card with a small caption and a wrapped body text.
static void calTextCard(lv_obj_t *parent, const char *caption, const char *body, lv_color_t col) {
  lv_obj_t *card = makeCard(parent, 456, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(card, 12, 0);
  lv_obj_set_style_pad_row(card, 4, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  makeLabel(card, caption, F_CAPTION, C_MUTED);
  lv_obj_t *b = makeLabel(card, "", F_BODY, col);
  lv_obj_set_width(b, 430);
  lv_label_set_long_mode(b, LV_LABEL_LONG_WRAP);
  lv_label_set_text(b, body);
}

static void calDetFill() {
  lv_obj_clean(calDetBox);
  int y = calDY, m = calDM, d = calDD;
  long jd = lunar::jdFromDate(d, m, y);
  LunarDate l = solarToLunar(d, m, y);
  int len = lunarMonthLength(l.start), wd = alm::weekday(jd);
  char b[160];
  lv_obj_t *c = calDetBox;

  // date + lunar date + holidays
  lv_obj_t *head = makeCard(c, 456, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(head, 12, 0);
  lv_obj_set_style_pad_row(head, 3, 0);
  lv_obj_set_flex_flow(head, LV_FLEX_FLOW_COLUMN);
  if (isVi()) snprintf(b, sizeof(b), "%s, %d tháng %d năm %d", CAL_WDL_VI[wd], d, m, y);
  else snprintf(b, sizeof(b), "%s, %d %s %d", CAL_WDL_EN[wd], d, MONTHS_EN[m - 1], y);
  lv_obj_t *t1 = makeLabel(head, "", F_TITLE, C_TEXT);
  lv_label_set_text(t1, b);
  snprintf(b, sizeof(b), isVi() ? "Âm lịch %d/%d%s năm %s %s" : "Lunar %d/%d%s, year %s %s", l.day, l.month, l.leap ? (isVi() ? " nhuận" : " leap") : "",
           CAN[alm::yearCan(l.year)], CHI[alm::yearChi(l.year)]);
  lv_obj_t *t2 = makeLabel(head, "", F_LABEL, C_ACCENT);
  lv_label_set_text(t2, b);
  const alm::Holiday *h[3];
  int n = alm::holidaysOn(d, m, l.day, l.month, l.leap, len, h);
  if (n) {
    String s;
    for (int i = 0; i < n; i++) {
      if (i) s += "  -  ";
      s += isVi() ? h[i]->vi : h[i]->en;
    }
    lv_obj_t *t3 = makeLabel(head, "", F_BODY, lv_color_hex(CC_AMBER));
    lv_obj_set_width(t3, 430);
    lv_label_set_long_mode(t3, LV_LABEL_LONG_WRAP);
    lv_label_set_text(t3, s.c_str());
  }
  if (alm::isNguyetKy(l.day) || alm::isTamNuong(l.day)) {
    lv_obj_t *t4 = makeLabel(head, "", F_BODY, C_DANGER);
    lv_label_set_text(t4, alm::isNguyetKy(l.day) ? DL("Nguyệt kỵ: kiêng mọi việc lớn", "Nguyệt kỵ: avoid major undertakings")
                                                : DL("Tam nương sát: không cưới hỏi", "Tam nương sát: no weddings"));
  }

  // Can Chi of day / month / year
  lv_obj_t *cc = makeCard(c, 456, 64);
  const char *cap[3] = {DL("Ngày", "Day"), DL("Tháng", "Month"), DL("Năm", "Year")};
  int can[3] = {alm::dayCan(jd), alm::monthCan(l.month, l.year), alm::yearCan(l.year)};
  int chi[3] = {alm::dayChi(jd), alm::monthChi(l.month), alm::yearChi(l.year)};
  for (int i = 0; i < 3; i++) {
    lv_obj_t *k = makeLabel(cc, cap[i], F_CAPTION, C_MUTED);
    lv_obj_set_pos(k, 16 + i * 148, 10);
    lv_obj_t *v = makeLabel(cc, "", F_LABEL, C_TEXT);
    snprintf(b, sizeof(b), "%s %s", CAN[can[i]], CHI[chi[i]]);
    lv_label_set_text(v, b);
    lv_obj_set_pos(v, 16 + i * 148, 30);
  }

  // solar term
  alm::TermInfo ti = alm::solarTerm(jd, y);
  if (ti.cur >= 0 && ti.next >= 0) {
    snprintf(b, sizeof(b), isVi() ? "%s  (%s sau %ld ngày)" : "%s  (%s in %ld d)", alm::TERMS[ti.cur].name, alm::TERMS[ti.next].name, ti.nextJd - jd);
    infoRow(c, DL("Tiết khí", "Solar term"), b);
  }
  // Trực and Sao, coloured by rating
  int tr = alm::trucIndex(jd, l.month), sa = alm::saoIndex(jd);
  snprintf(b, sizeof(b), "%s (%s)", alm::TRUC[tr], calRate(alm::TRUC_RATING[tr]));
  infoRow(c, DL("Trực", "Trực (duty)"), b, calRateCol(alm::TRUC_RATING[tr]));
  snprintf(b, sizeof(b), "%s (%s)", alm::SAO[sa], calRate(alm::SAO_RATING[sa]));
  infoRow(c, DL("Sao", "Sao (mansion)"), b, calRateCol(alm::SAO_RATING[sa]));
  double age;
  int mp = alm::moonPhase(jd, &age);
  snprintf(b, sizeof(b), isVi() ? "%s  (tuổi trăng %d)" : "%s  (age %d d)", isVi() ? alm::MOON_VI[mp] : alm::MOON_EN[mp], (int)age);
  infoRow(c, DL("Mặt trăng", "Moon"), b);

  // what to do / what to avoid
  int ci = alm::dayChi(jd);
  String yes, no;
  for (int i = 0; i < 4; i++) yes += String(i ? ", " : "") + alm::NEN_LAM[ci][i];
  for (int i = 0; i < 2; i++) no += String(i ? ", " : "") + alm::KHONG_NEN[ci][i];
  calTextCard(c, DL("NÊN LÀM", "GOOD FOR"), yes.c_str(), C_OK);
  calTextCard(c, DL("KHÔNG NÊN", "AVOID"), no.c_str(), C_DANGER);

  // lucky directions
  int cn = alm::dayCan(jd);
  infoRow(c, DL("Hướng Thần tài", "Wealth direction"), alm::THAN_TAI[cn]);
  infoRow(c, DL("Hướng Hỷ thần", "Joy direction"), alm::HY_THAN[cn]);
  infoRow(c, DL("Hướng Hung thần", "Unlucky direction"), alm::HUNG_THAN[ci], C_DANGER);

  // lucky hours
  lv_obj_t *hc = makeCard(c, 456, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(hc, 12, 0);
  lv_obj_set_style_pad_row(hc, 8, 0);
  lv_obj_set_flex_flow(hc, LV_FLEX_FLOW_COLUMN);
  makeLabel(hc, DL("GIỜ HOÀNG ĐẠO", "LUCKY HOURS"), F_CAPTION, C_MUTED);
  lv_obj_t *row = lv_obj_create(hc);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, 432, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_row(row, 6, 0);
  lv_obj_set_style_pad_column(row, 6, 0);
  struct tm now;
  bool isToday = y == calTodayY && m == calTodayM && d == calTodayD && getLocalTime(&now, 0);
  int nowChi = isToday ? ((now.tm_hour + 1) / 2) % 12 : -1;
  unsigned lucky = alm::luckyHours(jd);
  for (int i = 0; i < 12; i++) {
    if (!(lucky & (1u << i))) continue;
    lv_obj_t *chip = lv_obj_create(row);
    lv_obj_remove_style_all(chip);
    lv_obj_set_size(chip, 136, 34);
    lv_obj_set_style_radius(chip, 17, 0);
    lv_obj_set_style_bg_color(chip, C_OK, 0);
    lv_obj_set_style_bg_opa(chip, LV_OPA_20, 0);
    lv_obj_set_style_border_width(chip, i == nowChi ? 2 : 1, 0);
    lv_obj_set_style_border_color(chip, i == nowChi ? C_ACCENT : C_OK, 0);
    lv_obj_clear_flag(chip, LV_OBJ_FLAG_SCROLLABLE);
    snprintf(b, sizeof(b), "%s  %s", CHI[i], alm::GIO_TIME[i]);
    lv_obj_t *l1 = makeLabel(chip, "", F_BODY, C_TEXT);
    lv_label_set_text(l1, b);
    lv_obj_center(l1);
  }
  lv_obj_scroll_to_y(calDetBox, 0, LV_ANIM_OFF);
}

static void calOpenDetail() {
  calDY = calSelY;
  calDM = calSelM;
  calDD = calSelD;
  lv_obj_t *c;
  lv_obj_t *s = pageBegin(DL("Chi tiết ngày", "Day details"), &c, calDetBack);
  calDetBox = c;
  lv_obj_set_width(lv_obj_get_child(s, 1), 200);  // the title shares the row with the day arrows
  lv_obj_add_event_cb(s, [](lv_event_t *) { calDetBox = nullptr; }, LV_EVENT_DELETE, nullptr);
  lv_obj_t *prev = smallBtn(s, LV_SYMBOL_LEFT, C_ACCENT, [](lv_event_t *) { calDetStep(-1); }, nullptr, 52);
  lv_obj_set_pos(prev, 360, SB_H + 7);
  lv_obj_t *next = smallBtn(s, LV_SYMBOL_RIGHT, C_ACCENT, [](lv_event_t *) { calDetStep(1); }, nullptr, 52);
  lv_obj_set_pos(next, 416, SB_H + 7);
  calDetFill();
  pageShow(s);
}

// ---------- page ----------
static void calTick(lv_timer_t *) {
  int y, m, d;
  calNow(y, m, d);
  if (y != calTodayY || m != calTodayM || d != calTodayD) {  // midnight passed
    calTodayY = y;
    calTodayM = m;
    calTodayD = d;
    lv_obj_invalidate(calGrid);
  }
}

// Borderless arrow: a soft round highlight only while pressed.
static lv_obj_t *calArrow(const char *sym, int x, lv_event_cb_t cb) {
  lv_obj_t *b = lv_obj_create(calScr);
  lv_obj_remove_style_all(b);
  lv_obj_set_size(b, 40, 38);
  lv_obj_set_pos(b, x, SB_H + 6);
  lv_obj_set_style_radius(b, 19, 0);
  lv_obj_set_style_bg_color(b, C_ACCENT, 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_30, LV_STATE_PRESSED);
  lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *l = makeLabel(b, sym, F_LABEL, C_ACCENT);
  lv_obj_center(l);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
  return b;
}

static void pageCalendar() {
  lv_obj_t *c;
  calScr = pageBegin("", &c);
  lv_obj_del(c);
  lv_obj_add_event_cb(calScr, [](lv_event_t *) { calScr = calDetBox = nullptr; }, LV_EVENT_DELETE, nullptr);
  calNow(calTodayY, calTodayM, calTodayD);
  calSelY = calTodayY;
  calSelM = calTodayM;
  calSelD = calTodayD;

  // header: back, month and year (the page title), two soft arrows, and "Today" when it is useful
  calMonthLbl = lv_obj_get_child(calScr, 1);
  lv_obj_set_width(calMonthLbl, 160);
  lv_obj_set_pos(calMonthLbl, 70, SB_H + 13);
  calArrow(LV_SYMBOL_LEFT, 236, [](lv_event_t *) { calSetMonth(calY, calM - 1); });
  calArrow(LV_SYMBOL_RIGHT, 278, [](lv_event_t *) { calSetMonth(calY, calM + 1); });
  calTodayBtn = smallBtn(calScr, DL("Hôm nay", "Today"), C_OK, [](lv_event_t *) { calGoToday(); }, nullptr, 96);
  lv_obj_set_pos(calTodayBtn, 372, SB_H + 7);

  // month grid: one object, drawn by calDraw
  calGrid = lv_obj_create(calScr);
  lv_obj_remove_style_all(calGrid);
  lv_obj_set_size(calGrid, CAL_GW, CAL_GH);
  lv_obj_set_pos(calGrid, 12, SB_H + 50);
  lv_obj_add_flag(calGrid, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(calGrid, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(calGrid, calDraw, LV_EVENT_DRAW_MAIN, nullptr);
  lv_obj_add_event_cb(calGrid, [](lv_event_t *) { lv_indev_get_point(lv_indev_get_act(), &calPress); }, LV_EVENT_PRESSED, nullptr);
  lv_obj_add_event_cb(calGrid, calGridClick, LV_EVENT_SHORT_CLICKED, nullptr);
  lv_obj_add_event_cb(calGrid, calGridGesture, LV_EVENT_GESTURE, nullptr);

  // the selected day: a quiet panel with the date, the lunar date and one line of Can Chi
  lv_obj_t *p = lv_obj_create(calScr);
  lv_obj_remove_style_all(p);
  const int PW = 468 - (12 + CAL_GW + 8);  // panel width
  lv_obj_set_size(p, PW, CAL_GH);
  lv_obj_set_pos(p, 12 + CAL_GW + 8, SB_H + 50);
  lv_obj_set_style_radius(p, 16, 0);
  lv_obj_set_style_bg_color(p, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(p, LV_OPA_30, 0);
  lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
  const int pw = PW - 12;
  auto centred = [&](lv_obj_t *l, int y) {
    lv_obj_set_width(l, pw);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(l, 6, y);
  };
  calBig = makeLabel(p, "", F_DISPLAY, C_TEXT);
  centred(calBig, 8);
  calDow = makeLabel(p, "", F_LABEL, C_ACCENT);
  centred(calDow, 68);
  calLunar = makeLabel(p, "", F_BODY, lv_color_hex(0xD9C8FF));
  centred(calLunar, 94);
  calCan = makeLabel(p, "", F_CAPTION, lv_color_hex(0x9AA3C7));
  centred(calCan, 116);
  calHol = makeLabel(p, "", F_CAPTION, lv_color_hex(CC_AMBER));
  lv_label_set_long_mode(calHol, LV_LABEL_LONG_DOT);
  lv_obj_set_height(calHol, 32);
  centred(calHol, 142);
  // "Details >" as a link, not another bordered button
  lv_obj_t *det = lv_obj_create(p);
  lv_obj_remove_style_all(det);
  lv_obj_set_size(det, pw, 34);
  lv_obj_align(det, LV_ALIGN_BOTTOM_MID, 0, -8);
  lv_obj_set_style_radius(det, 17, 0);
  lv_obj_set_style_bg_color(det, C_ACCENT, 0);
  lv_obj_set_style_bg_opa(det, LV_OPA_20, LV_STATE_PRESSED);
  lv_obj_add_flag(det, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(det, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *dl = makeLabel(det, "", F_BODY, C_ACCENT);
  lv_label_set_text_fmt(dl, "%s  " LV_SYMBOL_RIGHT, DL("Chi tiết", "Details"));
  lv_obj_center(dl);
  lv_obj_add_event_cb(det, [](lv_event_t *) { calOpenDetail(); }, LV_EVENT_CLICKED, nullptr);

  calSetMonth(calTodayY, calTodayM, calTodayD);
  pageTimer(calScr, calTick, 30000);
  pageShow(calScr);
}
