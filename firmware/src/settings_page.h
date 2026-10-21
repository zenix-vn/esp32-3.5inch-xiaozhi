#pragma once
// Settings: one tidy hub (grouped rows with a coloured icon, the value on the right and a chevron) and a few small
// sub-pages (Display, Sound, QR account). Wi-Fi, Storage, About and the wallpaper picker are opened from here; they
// return here through pageParent (pages.h). Included by main.cpp after wallpaper.h.

static lv_obj_t *stScr = nullptr;  // the hub, while it exists

// Opens a page as a child of Settings: its back button comes back to the hub.
static void stOpen(void (*open)()) {
  pageParent = stScr;
  open();
}

// ---------- building blocks ----------
static lv_obj_t *stGroup(lv_obj_t *parent) {  // one rounded card holding rows separated by hairlines
  lv_obj_t *g = lv_obj_create(parent);
  lv_obj_remove_style_all(g);
  lv_obj_set_size(g, 456, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(g, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_radius(g, 16, 0);
  lv_obj_set_style_bg_color(g, C_CARD, 0);
  lv_obj_set_style_bg_opa(g, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(g, 1, 0);
  lv_obj_set_style_border_color(g, C_BORDER, 0);
  lv_obj_set_style_clip_corner(g, true, 0);
  lv_obj_clear_flag(g, LV_OBJ_FLAG_SCROLLABLE);
  return g;
}

static lv_obj_t *stHeader(lv_obj_t *parent, const char *text) {
  lv_obj_t *l = makeLabel(parent, "", F_CAPTION, lv_color_hex(0x9AA3C7));
  lv_label_set_text(l, text);
  lv_obj_set_style_pad_left(l, 6, 0);
  lv_obj_set_style_pad_top(l, 6, 0);
  return l;
}

// A row: icon chip, title, value (right), chevron when it opens something. cb == nullptr: not tappable.
static lv_obj_t *stRow(lv_obj_t *grp, const char *sym, uint32_t col, const char *title, const char *value, lv_color_t vcol, lv_event_cb_t cb,
                       bool chevron = true, void *ud = nullptr) {
  lv_obj_t *r = lv_obj_create(grp);
  lv_obj_remove_style_all(r);
  lv_obj_set_size(r, 456, 46);
  lv_obj_set_style_border_width(r, 1, 0);
  lv_obj_set_style_border_side(r, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_color(r, C_BORDER, 0);
  lv_obj_set_style_border_opa(r, LV_OPA_60, 0);
  lv_obj_set_style_bg_color(r, C_ACCENT, 0);
  lv_obj_set_style_bg_opa(r, LV_OPA_20, LV_STATE_PRESSED);
  lv_obj_clear_flag(r, LV_OBJ_FLAG_SCROLLABLE);
  if (cb) {
    lv_obj_add_flag(r, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(r, cb, LV_EVENT_CLICKED, ud);
  }
  lv_obj_t *chip = lv_obj_create(r);
  lv_obj_remove_style_all(chip);
  lv_obj_set_size(chip, 28, 28);
  lv_obj_set_pos(chip, 12, 9);
  lv_obj_set_style_radius(chip, 8, 0);
  lv_obj_set_style_bg_color(chip, lv_color_hex(col), 0);
  lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, 0);
  lv_obj_clear_flag(chip, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *ic = makeLabel(chip, "", F_CAPTION, lv_color_hex(0xFFFFFF));
  lv_label_set_text(ic, sym);
  lv_obj_center(ic);
  lv_obj_t *t = makeLabel(r, "", F_BODY, C_TEXT);
  lv_obj_set_size(t, 190, 20);
  lv_label_set_long_mode(t, LV_LABEL_LONG_DOT);
  lv_label_set_text(t, title);
  lv_obj_align(t, LV_ALIGN_LEFT_MID, 52, 0);
  if (value && value[0]) {
    lv_obj_t *v = makeLabel(r, "", F_BODY, vcol);
    lv_obj_set_size(v, 170, 20);
    lv_label_set_long_mode(v, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(v, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(v, value);
    lv_obj_align(v, LV_ALIGN_RIGHT_MID, chevron ? -32 : -14, 0);
  }
  if (chevron) {
    lv_obj_t *c = makeLabel(r, LV_SYMBOL_RIGHT, F_CAPTION, C_MUTED);
    lv_obj_align(c, LV_ALIGN_RIGHT_MID, -14, 0);
  }
  return r;
}

// Content list of a sub-page.
static lv_obj_t *stPage(const char *title, lv_obj_t **content) { return pageBegin(title, content); }

static lv_obj_t *stSlider(lv_obj_t *parent, const char *sym, uint32_t col, const char *title, int lo, int hi, int val, lv_event_cb_t cb, lv_obj_t **pct) {
  lv_obj_t *card = makeCard(parent, 456, 74);
  lv_obj_t *chip = lv_obj_create(card);
  lv_obj_remove_style_all(chip);
  lv_obj_set_size(chip, 28, 28);
  lv_obj_set_pos(chip, 12, 9);
  lv_obj_set_style_radius(chip, 8, 0);
  lv_obj_set_style_bg_color(chip, lv_color_hex(col), 0);
  lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, 0);
  lv_obj_t *ic = makeLabel(chip, "", F_CAPTION, lv_color_hex(0xFFFFFF));
  lv_label_set_text(ic, sym);
  lv_obj_center(ic);
  lv_obj_t *t = makeLabel(card, "", F_BODY, C_TEXT);
  lv_label_set_text(t, title);
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 52, 13);
  *pct = makeLabel(card, "", F_BODY, C_ACCENT);
  lv_obj_align(*pct, LV_ALIGN_TOP_RIGHT, -16, 13);
  lv_obj_t *sl = lv_slider_create(card);
  lv_slider_set_range(sl, lo, hi);
  lv_slider_set_value(sl, val, LV_ANIM_OFF);
  lv_obj_set_width(sl, 424);
  lv_obj_align(sl, LV_ALIGN_BOTTOM_MID, 0, -14);
  lv_obj_set_style_bg_color(sl, C_BORDER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(sl, C_ACCENT, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(sl, C_TEXT, LV_PART_KNOB);
  lv_obj_set_ext_click_area(sl, 12);
  lv_obj_add_event_cb(sl, cb, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(sl, cb, LV_EVENT_RELEASED, nullptr);
  return sl;
}

// ---------- Display ----------
static lv_obj_t *stBrightPct, *stVolPct;
static void stBrightness(lv_event_t *e) {
  ccBrightness(e);
  setTextFmt(stBrightPct, "%d%%", blLevel * 100 / 255);
}
static const uint8_t ST_SLEEP[4] = {0, 1, 5, 10};

static void pageSettingsDisplay() {
  lv_obj_t *c;
  lv_obj_t *s = stPage(DL("Màn hình", "Display"), &c);
  stSlider(c, LV_SYMBOL_EYE_OPEN, 0xF2A33A, DL("Độ sáng", "Brightness"), 20, 255, blLevel, stBrightness, &stBrightPct);
  setTextFmt(stBrightPct, "%d%%", blLevel * 100 / 255);

  lv_obj_t *card = makeCard(c, 456, 88);
  lv_obj_t *t = makeLabel(card, "", F_BODY, C_TEXT);
  lv_label_set_text(t, DL("Khi không dùng trong", "When idle for"));
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 16, 10);
  static const char *sleepMap[5];
  sleepMap[0] = DL("Không", "Never");
  sleepMap[1] = DL("1 phút", "1 min");
  sleepMap[2] = DL("5 phút", "5 min");
  sleepMap[3] = DL("10 phút", "10 min");
  sleepMap[4] = "";
  lv_obj_t *chips = ckChips(card, sleepMap, true, [](lv_event_t *e) {
    uint32_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
    if (id >= 4) return;
    sleepMin = ST_SLEEP[id];
    Preferences p;
    p.begin("ui", false);
    p.putUChar("sleep", sleepMin);
    p.end();
  });
  for (int i = 0; i < 4; i++)
    if (ST_SLEEP[i] == sleepMin) lv_btnmatrix_set_btn_ctrl(chips, i, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_set_size(chips, 424, 38);
  lv_obj_align(chips, LV_ALIGN_BOTTOM_MID, 0, -10);

  lv_obj_t *card2 = makeCard(c, 456, 88);
  lv_obj_t *t2 = makeLabel(card2, "", F_BODY, C_TEXT);
  lv_label_set_text(t2, DL("Sau đó", "Then"));
  lv_obj_align(t2, LV_ALIGN_TOP_LEFT, 16, 10);
  static const char *idleMap[3];
  idleMap[0] = DL("Tắt màn hình", "Turn screen off");
  idleMap[1] = DL("Màn hình chờ", "Ambient clock");
  idleMap[2] = "";
  lv_obj_t *chips2 = ckChips(card2, idleMap, true, [](lv_event_t *e) {
    uint32_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
    if (id > 1) return;
    cfg.idleMode = id;
    cfgSave();
  });
  lv_btnmatrix_set_btn_ctrl(chips2, cfg.idleMode ? 1 : 0, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_set_size(chips2, 424, 38);
  lv_obj_align(chips2, LV_ALIGN_BOTTOM_MID, 0, -10);

  stHeader(c, DL("HÌNH NỀN", "WALLPAPER"));
  lv_obj_t *g = stGroup(c);
  stRow(g, LV_SYMBOL_IMAGE, 0x3FA7D6, DL("Chọn từ thẻ nhớ", "Choose from the card"), cfg.wallOn ? DL("Đang dùng", "In use") : "", C_ACCENT,
        [](lv_event_t *) { stOpen(pageWallpaper); });
  if (cfg.wallOn)
    stRow(g, LV_SYMBOL_TRASH, 0xE0566E, DL("Gỡ hình nền", "Remove the wallpaper"), "", C_MUTED, [](lv_event_t *e) {
      wallpaperOff();
      cfg.wallOn = false;
      cfgSave();
      uiToast(DL("Đã gỡ hình nền", "Wallpaper removed"), C_ACCENT);
      lv_obj_add_flag(lv_event_get_target(e), LV_OBJ_FLAG_HIDDEN);
    }, false);
  pageShow(s);
}

// ---------- Sound ----------
static void stVolume(lv_event_t *e) {
  ccVolume(e);
  setTextFmt(stVolPct, "%d%%", cfg.volume);
}
static void pageSettingsSound() {
  lv_obj_t *c;
  lv_obj_t *s = stPage(DL("Âm thanh", "Sound"), &c);
  stSlider(c, LV_SYMBOL_VOLUME_MAX, 0x7B6BFF, DL("Âm lượng", "Volume"), 0, 100, cfg.volume, stVolume, &stVolPct);
  setTextFmt(stVolPct, "%d%%", cfg.volume);
  lv_obj_t *g = stGroup(c);
  lv_obj_t *r = stRow(g, LV_SYMBOL_MUTE, 0xE0566E, DL("Chế độ im lặng", "Do not disturb"), "", C_MUTED, nullptr, false);
  lv_obj_t *sw = lv_switch_create(r);
  lv_obj_set_size(sw, 52, 28);
  lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -14, 0);
  lv_obj_set_style_bg_color(sw, C_BORDER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(sw, C_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
  if (cfg.dnd) lv_obj_add_state(sw, LV_STATE_CHECKED);
  lv_obj_add_event_cb(sw, [](lv_event_t *e) {
    cfg.dnd = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    cfgSave();
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_t *h = makeLabel(c, "", F_CAPTION, C_MUTED);
  lv_obj_set_width(h, 440);
  lv_label_set_long_mode(h, LV_LABEL_LONG_WRAP);
  lv_label_set_text(h, DL("Im lặng tắt tiếng chạm và tiếng nhắc của rồng; báo thức vẫn kêu. Tiếng ru ngủ do bạn tự bật nên vẫn phát.",
                          "Silent mode mutes taps and dragon reminders; alarms still ring. Sleep sounds you start yourself keep playing."));
  pageShow(s);
}

// ---------- microphone test ----------
static lv_obj_t *mtBar, *mtState, *mtPins;
static int mtPeak = 0, mtSilent = 0;

static void mtShowPins() {
  setTextFmt(mtPins, "SCK = IO%d   WS = IO%d   SD = IO%d", micSwap ? MIC_PIN_A : MIC_PIN_B, micSwap ? MIC_PIN_B : MIC_PIN_A, MIC_PIN_SD);
}

static void mtTick(lv_timer_t *) {
  bool flat = false;
  int lv = micLevel(&flat);
  if (lv < 0) {
    if (++mtSilent > 6) setText(mtState, DL("Không nhận được dữ liệu từ mic", "No data from the mic"));
    return;
  }
  if (flat) {
    if (++mtSilent > 6) setText(mtState, DL("Chân SD đứng yên: kiểm tra dây, thử Đổi SCK/WS", "SD line is stuck: check the wiring, try Swap SCK/WS"));
    lv_bar_set_value(mtBar, 0, LV_ANIM_OFF);
    return;
  }
  mtSilent = 0;
  mtPeak = max(lv, mtPeak * 9 / 10);
  int v = lv > 0 ? (int)(log10f((float)lv + 1.0f) * 1000.0f / 4.5f) : 0;  // log scale, 0..~1000
  lv_bar_set_value(mtBar, constrain(v, 0, 1000), LV_ANIM_OFF);
  setText(mtState, lv > 400 ? DL("Mic hoạt động: đang nghe thấy âm thanh", "Mic works: sound detected")
                            : DL("Mic có tín hiệu. Hãy nói hoặc vỗ tay thử", "Mic is live. Try speaking or clapping"));
}

static void pageSettingsMic() {
  lv_obj_t *c;
  lv_obj_t *s = stPage(DL("Micro", "Microphone"), &c);
  mtPeak = mtSilent = 0;
  lv_obj_add_event_cb(s, [](lv_event_t *) { micStop(); }, LV_EVENT_DELETE, nullptr);
  lv_obj_t *card = makeCard(c, 456, 112);
  mtBar = lv_bar_create(card);
  lv_obj_set_size(mtBar, 420, 18);
  lv_obj_align(mtBar, LV_ALIGN_TOP_MID, 0, 16);
  lv_bar_set_range(mtBar, 0, 1000);
  lv_obj_set_style_bg_color(mtBar, C_BORDER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(mtBar, C_ACCENT, LV_PART_INDICATOR);
  mtState = makeLabel(card, "", F_BODY, C_TEXT);
  lv_obj_set_width(mtState, 420);
  lv_label_set_long_mode(mtState, LV_LABEL_LONG_WRAP);
  lv_obj_align(mtState, LV_ALIGN_TOP_LEFT, 18, 46);
  mtPins = makeLabel(card, "", F_CAPTION, C_MUTED);
  lv_obj_align(mtPins, LV_ALIGN_BOTTOM_LEFT, 18, -10);
  mtShowPins();
  if (micStart()) setText(mtState, DL("Đang nghe...", "Listening..."));
  else setText(mtState, DL("Không khởi động được I2S cho mic", "Could not start I2S for the mic"));

  lv_obj_t *g = stGroup(c);
  stRow(g, LV_SYMBOL_SHUFFLE, 0x35B0A8, DL("Đổi SCK/WS (IO21 <-> IO22)", "Swap SCK/WS (IO21 <-> IO22)"), "", C_MUTED, [](lv_event_t *) {
    micStop();
    micSwap = !micSwap;
    micPrefSave();
    mtSilent = 0;
    mtShowPins();
    micStart();
  }, false);
  lv_obj_t *h = makeLabel(c, "", F_CAPTION, C_MUTED);
  lv_obj_set_width(h, 440);
  lv_label_set_long_mode(h, LV_LABEL_LONG_WRAP);
  lv_label_set_text(h, DL("Mic I2S (INMP441...): IO35 = SD (dữ liệu), IO21/IO22 = SCK và WS. Nói vào mic: thanh phải nhảy. Nếu thanh đứng yên hoặc báo lỗi, bấm Đổi SCK/WS; chân L/R của mic nối GND.",
                          "I2S mic (INMP441...): IO35 = SD (data), IO21/IO22 = SCK and WS. Speak into it: the bar should move. If it stays flat or reports an error, press Swap SCK/WS; the mic's L/R pin goes to GND."));
  pageTimer(s, mtTick, 60);
  pageShow(s);
}

// ---------- QR account ----------
static lv_obj_t *stQrRows[QR_MAX];
static void stQrRefresh() {
  for (int i = 0; i < qrCount; i++) {
    bool on = i == qrSel;
    lv_obj_t *l = lv_obj_get_child(stQrRows[i], 1);  // child 0 is the chip, 1 the title
    lv_obj_set_style_text_color(l, on ? C_ACCENT : C_TEXT, 0);
    lv_obj_t *tick = lv_obj_get_child(stQrRows[i], 2);
    if (on) lv_obj_clear_flag(tick, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(tick, LV_OBJ_FLAG_HIDDEN);
  }
}
static void pageSettingsQr() {
  lv_obj_t *c;
  lv_obj_t *s = stPage(DL("Tài khoản QR", "Payment account"), &c);
  lv_obj_t *g = stGroup(c);
  for (int i = 0; i < qrCount; i++) {
    stQrRows[i] = stRow(g, "QR", 0x2FB67C, qrLabel(i).c_str(), "", C_MUTED, [](lv_event_t *e) {
      int i = (int)(intptr_t)lv_event_get_user_data(e);
      if (i == qrSel || i >= qrCount) return;
      qrSel = i;
      qrApply();
      qrSave();
      cfgSave();
      qrRebuild();
      stQrRefresh();
      uiToast(DL("Đã chọn tài khoản QR", "QR account selected"), C_OK);
    }, false, (void *)(intptr_t)i);
    lv_obj_t *tick = makeLabel(stQrRows[i], LV_SYMBOL_OK, F_BODY, C_ACCENT);
    lv_obj_align(tick, LV_ALIGN_RIGHT_MID, -14, 0);
  }
  stQrRefresh();
  lv_obj_t *h = makeLabel(c, "", F_CAPTION, C_MUTED);
  lv_obj_set_width(h, 440);
  lv_label_set_long_mode(h, LV_LABEL_LONG_WRAP);
  lv_label_set_text(h, DL("Thêm, sửa hoặc xoá tài khoản trong trang web của thiết bị (mục Thanh toán). Tài khoản đang chọn hiện trên màn hình chính.",
                          "Add, edit or delete accounts on the device web page (Payment). The selected one is shown on the main screen."));
  pageShow(s);
}

// ---------- language, time zone ----------
static uint8_t stPendingLang = LANG_VI;
static void stDoLang() {
  i18nSave(stPendingLang);
  ESP.restart();
}
static void stAskLang(lv_event_t *e) {
  uint8_t l = (uint8_t)(intptr_t)lv_event_get_user_data(e);
  if (l == gLang) return;
  stPendingLang = l;
  confirmDialog("Change language?", "The device restarts to apply the new language.", stDoLang);
}
static lv_obj_t *stTzLabel;
static int stTz;
static void stTzStep(lv_event_t *e) {
  stTz = constrain(stTz + (int)(intptr_t)lv_event_get_user_data(e), -12, 14);
  setTextFmt(stTzLabel, "UTC%+d", stTz);
  wifiSetTimezone(stTz);
}

// ---------- the hub ----------
static void pageSettings() {
  lv_obj_t *c;
  lv_obj_t *s = pageBegin("Settings", &c);
  stScr = s;
  lv_obj_add_event_cb(s, [](lv_event_t *) {
    if (pageParent == stScr) pageParent = nullptr;
    stScr = nullptr;
  }, LV_EVENT_DELETE, nullptr);
  char b[96];

  // connection
  stHeader(c, DL("KẾT NỐI", "CONNECTION"));
  lv_obj_t *g = stGroup(c);
  stRow(g, LV_SYMBOL_WIFI, 0x2FA6E8, "Wi-Fi", wifiConnected() ? WiFi.SSID().c_str() : DL("Chưa kết nối", "Not connected"),
        wifiConnected() ? C_OK : C_WARN, [](lv_event_t *) { stOpen(pageWifi); });
  if (wifiConnected()) {
    snprintf(b, sizeof(b), "%s.local", webHost());
    stRow(g, LV_SYMBOL_GPS, 0x6A7BE8, DL("Trang web cấu hình", "Web setup"), b, C_ACCENT, nullptr, false);
  }
  stRow(g, LV_SYMBOL_AUDIO, 0x9B6AE0, "Xiaozhi", xiaozhiState() == XZ_ACTIVATED ? DL("Đã kết nối", "Connected") : xiaozhiMessage(),
        xiaozhiState() == XZ_ACTIVATED ? C_OK : C_WARN, nullptr, false);

  // device
  stHeader(c, DL("THIẾT BỊ", "DEVICE"));
  g = stGroup(c);
  snprintf(b, sizeof(b), "%d%%", blLevel * 100 / 255);
  stRow(g, LV_SYMBOL_EYE_OPEN, 0xF2A33A, DL("Màn hình", "Display"), b, C_MUTED, [](lv_event_t *) { stOpen(pageSettingsDisplay); });
  snprintf(b, sizeof(b), cfg.dnd ? DL("Im lặng", "Silent") : "%d%%", cfg.volume);
  stRow(g, LV_SYMBOL_VOLUME_MAX, 0x7B6BFF, DL("Âm thanh", "Sound"), b, C_MUTED, [](lv_event_t *) { stOpen(pageSettingsSound); });
  stRow(g, LV_SYMBOL_AUDIO, 0xD9559A, DL("Micro", "Microphone"), "", C_MUTED, [](lv_event_t *) { stOpen(pageSettingsMic); });
  if (cfg.qrOn) stRow(g, "QR", 0x2FB67C, DL("Tài khoản QR", "Payment account"), qrCount ? (cfg.qrName.length() ? cfg.qrName.c_str() : cfg.qrAcct.c_str()) : "", C_MUTED,
                      [](lv_event_t *) { stOpen(pageSettingsQr); });
  if (sdOk) {
    char cap[24];
    fmtBytes(SD.cardSize(), cap, sizeof(cap));
    stRow(g, LV_SYMBOL_DRIVE, 0x4C8BF5, DL("Bộ nhớ", "Storage"), cap, C_MUTED, [](lv_event_t *) { stOpen(pageStorage); });
  } else {
    stRow(g, LV_SYMBOL_DRIVE, 0x4C8BF5, DL("Bộ nhớ", "Storage"), DL("Chưa có thẻ", "No card"), C_WARN, [](lv_event_t *) { stOpen(pageStorage); });
  }

  // general
  stHeader(c, DL("CHUNG", "GENERAL"));
  g = stGroup(c);
  lv_obj_t *r = stRow(g, "Aa", 0xE6708A, DL("Ngôn ngữ", "Language"), "", C_MUTED, nullptr, false);
  lv_obj_t *en = smallBtn(r, "EN", gLang == LANG_EN ? C_ACCENT : C_MUTED, stAskLang, (void *)(intptr_t)LANG_EN, 52);
  lv_obj_set_height(en, 32);
  lv_obj_align(en, LV_ALIGN_RIGHT_MID, -10, 0);
  lv_obj_t *vi = smallBtn(r, "VI", gLang == LANG_VI ? C_ACCENT : C_MUTED, stAskLang, (void *)(intptr_t)LANG_VI, 52);
  lv_obj_set_height(vi, 32);
  lv_obj_align(vi, LV_ALIGN_RIGHT_MID, -68, 0);
  r = stRow(g, LV_SYMBOL_LOOP, 0x35B0A8, DL("Múi giờ", "Time zone"), "", C_MUTED, nullptr, false);
  stTz = wifiTimezone();
  lv_obj_t *plus = smallBtn(r, LV_SYMBOL_PLUS, C_ACCENT, stTzStep, (void *)(intptr_t)1, 40);
  lv_obj_set_height(plus, 32);
  lv_obj_align(plus, LV_ALIGN_RIGHT_MID, -10, 0);
  stTzLabel = makeLabel(r, "", F_BODY, C_TEXT);
  lv_obj_set_width(stTzLabel, 64);
  lv_obj_set_style_text_align(stTzLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(stTzLabel, LV_ALIGN_RIGHT_MID, -52, 0);
  setTextFmt(stTzLabel, "UTC%+d", stTz);
  lv_obj_t *minus = smallBtn(r, LV_SYMBOL_MINUS, C_ACCENT, stTzStep, (void *)(intptr_t)-1, 40);
  lv_obj_set_height(minus, 32);
  lv_obj_align(minus, LV_ALIGN_RIGHT_MID, -118, 0);
  snprintf(b, sizeof(b), "Zenix %s", FW_VERSION);
  stRow(g, "i", 0x6C7A99, DL("Giới thiệu", "About"), b, C_MUTED, [](lv_event_t *) { stOpen(pageAbout); });

  // system
  stHeader(c, DL("HỆ THỐNG", "SYSTEM"));
  g = stGroup(c);
  stRow(g, LV_SYMBOL_EDIT, 0x4C8BF5, DL("Hiệu chuẩn cảm ứng", "Recalibrate touch"), "", C_MUTED, askRecalibrate);
  stRow(g, LV_SYMBOL_REFRESH, 0xF2A33A, DL("Khởi động lại", "Restart"), "", C_MUTED, askRestart);
  stRow(g, LV_SYMBOL_TRASH, 0xE0566E, DL("Khôi phục cài đặt gốc", "Factory reset"), "", C_DANGER, askFactoryReset);
  lv_obj_t *pad = lv_obj_create(c);  // a little room under the last group
  lv_obj_remove_style_all(pad);
  lv_obj_set_size(pad, 10, 6);
  pageShow(s);
}
