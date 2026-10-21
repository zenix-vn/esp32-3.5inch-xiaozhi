#pragma once
// Weather app (V2.4): now, the next 12 hours and 5 days. Opened from the launcher or the home weather card.

static void pageWeather() {
  lv_obj_t *c;
  lv_obj_t *s = pageBegin("Weather", &c);
  WeatherData w = weatherGet();
  if (!w.valid) {
    infoRow(c, "Weather", w.failed ? "Weather unavailable" : (wifiConnected() ? "Loading weather..." : "Waiting for network"), C_WARN);
    pageShow(s);
    return;
  }
  // now
  lv_obj_t *now = makeCard(c, 456, 84);
  lv_obj_t *ic = wxIconBox(now, iconKindFor(w.code, w.isDay));
  lv_obj_align(ic, LV_ALIGN_LEFT_MID, 14, 0);
  lv_obj_t *t = makeLabel(now, "", F_HEADING, C_TEXT);
  setTextFmt(t, "%d%s%c", tempOut(w.temp), DEG, cfg.tempF ? 'F' : 'C');
  lv_obj_set_pos(t, 72, 10);
  lv_obj_t *d = makeLabel(now, "", F_BODY, C_TEXT);
  setTextFmt(d, "%s  -  %s", tr(weatherText(w.code)), w.city);
  lv_obj_set_pos(d, 72, 48);
  lv_obj_t *hm = makeLabel(now, "", F_CAPTION, C_MUTED);
  setTextFmt(hm, isVi() ? "Độ ẩm %d%%\nGió %d km/h" : "Humidity %d%%\nWind %d km/h", w.humidity, (int)lroundf(w.wind));
  lv_obj_align(hm, LV_ALIGN_RIGHT_MID, -14, 0);
  lv_obj_set_style_text_align(hm, LV_TEXT_ALIGN_RIGHT, 0);

  // next hours (horizontal strip)
  sectionLabel(c, DL("12 GIỜ TỚI", "NEXT 12 HOURS"));
  lv_obj_t *strip = lv_obj_create(c);
  lv_obj_remove_style_all(strip);
  lv_obj_set_size(strip, 456, 74);
  lv_obj_set_flex_flow(strip, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(strip, 6, 0);
  lv_obj_set_scroll_dir(strip, LV_DIR_HOR);
  lv_obj_set_scrollbar_mode(strip, LV_SCROLLBAR_MODE_OFF);
  static const uint32_t dotCol[] = {0x2A3359, 0xFFC93C, 0xDDE3FF, 0xFFC93C, 0xB8C1E8, 0x9AA3C7, 0x5AA9FF, 0xFFFFFF, 0xFFC93C};
  for (int i = 0; i < w.nHours; i++) {
    const WxHour &h = w.hours[i];
    lv_obj_t *cell = makeCard(strip, 64, 74);
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *hl = makeLabel(cell, "", F_CAPTION, C_MUTED);
    setTextFmt(hl, i == 0 ? (isVi() ? "Bây giờ" : "Now") : "%dh", h.hour);
    lv_obj_align(hl, LV_ALIGN_TOP_MID, 0, 6);
    shape(cell, 27, 26, 10, 10, dotCol[iconKindFor(h.code, h.hour >= 6 && h.hour < 18)]);
    lv_obj_t *tl = makeLabel(cell, "", F_LABEL, C_TEXT);
    setTextFmt(tl, "%d%s", tempOut(h.temp), DEG);
    lv_obj_align(tl, LV_ALIGN_BOTTOM_MID, 0, -16);
    if (h.pop >= 20) {
      lv_obj_t *pl = makeLabel(cell, "", F_CAPTION, lv_color_hex(0x7FB8FF));
      setTextFmt(pl, "%d%%", h.pop);
      lv_obj_align(pl, LV_ALIGN_BOTTOM_MID, 0, -2);
    }
  }

  // next days
  sectionLabel(c, DL("5 NGÀY TỚI", "NEXT 5 DAYS"));
  for (int i = 0; i < w.nDays; i++) {
    const WxDay &x = w.days[i];
    lv_obj_t *r = makeCard(c, 456, 52);
    lv_obj_t *dl = makeLabel(r, i == 0 ? DL("Hôm nay", "Today") : (isVi() ? DAYS_VI[x.wday] : DAYS_EN[x.wday]), F_BODY, C_TEXT);
    lv_obj_align(dl, LV_ALIGN_LEFT_MID, 14, -8);
    lv_obj_t *dd = makeLabel(r, "", F_CAPTION, C_MUTED);
    setTextFmt(dd, "%02d/%02d", x.mday, x.mon);
    lv_obj_align(dd, LV_ALIGN_LEFT_MID, 14, 10);
    lv_obj_t *ib = wxIconBox(r, iconKindFor(x.code, true));
    lv_obj_align(ib, LV_ALIGN_LEFT_MID, 124, 0);
    lv_obj_t *tx = makeLabel(r, tr(weatherText(x.code)), F_CAPTION, C_MUTED);
    lv_obj_set_width(tx, 110);
    lv_label_set_long_mode(tx, LV_LABEL_LONG_DOT);
    lv_obj_align(tx, LV_ALIGN_LEFT_MID, 176, 0);
    if (x.pop >= 20) {
      lv_obj_t *pl = makeLabel(r, "", F_CAPTION, lv_color_hex(0x7FB8FF));
      setTextFmt(pl, "%d%%", x.pop);
      lv_obj_align(pl, LV_ALIGN_RIGHT_MID, -110, 0);
    }
    lv_obj_t *mm = makeLabel(r, "", F_LABEL, C_TEXT);
    setTextFmt(mm, "%d%s / %d%s", tempOut(x.tmin), DEG, tempOut(x.tmax), DEG);
    lv_obj_align(mm, LV_ALIGN_RIGHT_MID, -14, 0);
  }
  lv_obj_t *up = smallBtn(c, DL("Cập nhật", "Refresh"), C_ACCENT, [](lv_event_t *) {
    weatherRefresh();
    uiToast(tr("Updating..."));
  }, nullptr, 456);
  (void)up;
  pageShow(s);
}
