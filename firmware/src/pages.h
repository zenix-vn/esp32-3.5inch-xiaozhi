#pragma once
// App pages: Settings, Wi-Fi (scan / connect / password keyboard), Storage, About, "coming soon".
// Included by main.cpp after its design tokens and helpers (makeLabel, makeCard, mainScr, ...).
#include <esp_system.h>
#include <nvs_flash.h>

static const char *webHost();  // webconfig.h
static int photosCount();      // photos.h
static void pageWallpaper();   // wallpaper.h
static void wallpaperOff();    // wallpaper.h
static void pageSettings();
static void uiToast(const char *text, lv_color_t col);  // statusbar.h (its definition adds the default colour)
static uint8_t sleepMin = 0;
static bool pageReplace = false;  // set while opening a page from another page: the old one is deleted  // screen-off timeout in minutes, 0 = never

// ---------- generic page plumbing ----------
static void goBack(lv_event_t *) { lv_scr_load_anim(mainScr, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 250, 0, true); }
static void pageShow(lv_obj_t *s, lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_MOVE_LEFT, bool delOld = false) {
  lv_scr_load_anim(s, anim, 250, 0, delOld || pageReplace);
}

// Timer that lives exactly as long as the given page screen.
static lv_timer_t *pageTimer(lv_obj_t *scr, lv_timer_cb_t cb, uint32_t ms) {
  lv_timer_t *t = lv_timer_create(cb, ms, nullptr);
  lv_obj_add_event_cb(
      scr, [](lv_event_t *e) { lv_timer_del((lv_timer_t *)lv_event_get_user_data(e)); }, LV_EVENT_DELETE, t);
  return t;
}

// New page screen with header (back button + title); content is a scrollable column.
static lv_obj_t *pageBegin(const char *title, lv_obj_t **content, lv_event_cb_t back = goBack) {
  lv_obj_t *s = lv_obj_create(nullptr);
  styleScreen(s);
  lv_obj_t *b = lv_btn_create(s);
  lv_obj_set_size(b, 48, 38);
  lv_obj_align(b, LV_ALIGN_TOP_LEFT, 12, SB_H + 6);
  lv_obj_set_style_radius(b, 19, 0);
  lv_obj_set_style_bg_color(b, C_CARD, 0);
  lv_obj_set_style_border_width(b, 1, 0);
  lv_obj_set_style_border_color(b, C_BORDER, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_t *bl = makeLabel(b, LV_SYMBOL_LEFT, F_LABEL, C_TEXT);
  lv_obj_center(bl);
  lv_obj_add_event_cb(b, back, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *t = makeLabel(s, title, F_TITLE, C_TEXT);
  lv_obj_set_width(t, 380);
  lv_label_set_long_mode(t, LV_LABEL_LONG_DOT);
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 72, SB_H + 13);

  lv_obj_t *c = lv_obj_create(s);
  lv_obj_remove_style_all(c);
  lv_obj_set_size(c, 456, 320 - (SB_H + 52));
  lv_obj_set_pos(c, 12, SB_H + 52);
  lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(c, 8, 0);
  lv_obj_set_style_pad_bottom(c, 12, 0);
  lv_obj_set_scroll_dir(c, LV_DIR_VER);
  *content = c;
  return s;
}

static void sectionLabel(lv_obj_t *parent, const char *text) {
  lv_obj_t *l = makeLabel(parent, text, F_CAPTION, C_MUTED);
  lv_obj_set_style_pad_top(l, 8, 0);
  lv_obj_set_style_pad_left(l, 4, 0);
}

static lv_obj_t *infoRow(lv_obj_t *parent, const char *key, const char *value, lv_color_t vcol = lv_color_hex(0xF2F5FF)) {
  lv_obj_t *r = makeCard(parent, 456, 44);
  lv_obj_t *k = makeLabel(r, key, F_BODY, C_MUTED);
  lv_obj_align(k, LV_ALIGN_LEFT_MID, 16, 0);
  lv_obj_t *v = makeLabel(r, value, F_BODY, vcol);
  lv_obj_set_width(v, 300);
  lv_label_set_long_mode(v, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(v, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_align(v, LV_ALIGN_RIGHT_MID, -16, 0);
  return r;
}
static void setRowValue(lv_obj_t *row, const char *text) { setText(lv_obj_get_child(row, 1), text); }

static lv_obj_t *actionBtn(lv_obj_t *parent, const char *text, lv_color_t color, lv_event_cb_t cb) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_set_size(b, 456, 44);
  lv_obj_set_style_radius(b, 14, 0);
  lv_obj_set_style_bg_color(b, color, 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_20, 0);
  lv_obj_set_style_border_width(b, 1, 0);
  lv_obj_set_style_border_color(b, color, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_40, LV_STATE_PRESSED);
  lv_obj_t *l = makeLabel(b, text, F_BODY, color);
  lv_obj_center(l);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
  return b;
}

static lv_obj_t *smallBtn(lv_obj_t *parent, const char *text, lv_color_t color, lv_event_cb_t cb, void *ud, int w = 36) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_set_size(b, w, 36);
  lv_obj_set_style_radius(b, 12, 0);
  lv_obj_set_style_bg_color(b, color, 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_20, 0);
  lv_obj_set_style_border_width(b, 1, 0);
  lv_obj_set_style_border_color(b, color, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_40, LV_STATE_PRESSED);
  lv_obj_t *l = makeLabel(b, text, F_BODY, color);
  lv_obj_center(l);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
  return b;
}

// ---------- confirm dialog ----------
static void (*pendingAction)() = nullptr;
static const char *confirmBtns[3];
static void confirmCb(lv_event_t *e) {
  lv_obj_t *m = lv_event_get_current_target(e);
  uint16_t idx = lv_msgbox_get_active_btn(m);
  lv_msgbox_close(m);
  if (idx == 1 && pendingAction) pendingAction();
  pendingAction = nullptr;
}
static void confirmDialog(const char *title, const char *text, void (*action)()) {
  pendingAction = action;
  confirmBtns[0] = tr("Cancel");
  confirmBtns[1] = tr("Confirm");
  confirmBtns[2] = "";
  lv_obj_t *m = lv_msgbox_create(nullptr, tr(title), tr(text), confirmBtns, false);
  lv_obj_set_width(m, 340);
  lv_obj_set_style_bg_color(m, C_CARD, 0);
  lv_obj_set_style_border_color(m, C_BORDER, 0);
  lv_obj_set_style_border_width(m, 1, 0);
  lv_obj_set_style_radius(m, 18, 0);
  lv_obj_set_style_text_color(m, C_TEXT, 0);
  lv_obj_set_style_text_font(m, F_BODY, 0);
  lv_obj_t *btns = lv_msgbox_get_btns(m);
  lv_obj_set_style_text_font(btns, F_BODY, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(btns, C_BORDER, LV_PART_ITEMS);
  lv_obj_set_style_text_color(btns, C_TEXT, LV_PART_ITEMS);
  lv_obj_add_event_cb(m, confirmCb, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_center(m);
}

static void clearNamespace(const char *ns) {
  Preferences p;
  p.begin(ns, false);
  p.clear();
  p.end();
}
static void doForgetWifi() {
  clearNamespace("wifi");
  ESP.restart();
}
static void doRecalibrate() {
  clearNamespace("touch");
  ESP.restart();
}
static void doRestart() { ESP.restart(); }
static void doFactoryReset() {
  nvs_flash_erase();
  nvs_flash_init();
  ESP.restart();
}
static void askForgetWifi(lv_event_t *) {
  confirmDialog("Forget Wi-Fi?", "The device restarts and opens the Zin100-Setup hotspot.", doForgetWifi);
}
static void askRecalibrate(lv_event_t *) {
  confirmDialog("Recalibrate touch?", "The device restarts and shows 4 targets to tap.", doRecalibrate);
}
static void askRestart(lv_event_t *) { confirmDialog("Restart?", "The device will reboot now.", doRestart); }
static void askFactoryReset(lv_event_t *) {
  confirmDialog("Factory reset?", "Erases Wi-Fi, touch calibration and all settings, then restarts.", doFactoryReset);
}

// =====================================================================================
// Wi-Fi
// =====================================================================================
struct NetItem {
  char ssid[33];
  bool secure;
  int rssi;
};
static NetItem nets[14];
static int netCount = 0;
static lv_obj_t *wifiList = nullptr;
static lv_timer_t *scanTimer = nullptr;

static void showWifiPage(lv_scr_load_anim_t anim, bool delOld);
static void pagePassword(const char *ssid);

static void wifiBars(lv_obj_t *parent, int rssi) {
  int level = rssi > -55 ? 4 : rssi > -65 ? 3 : rssi > -75 ? 2 : 1;
  lv_obj_t *box = lv_obj_create(parent);
  lv_obj_remove_style_all(box);
  lv_obj_set_size(box, 26, 16);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE);
  for (int i = 0; i < 4; i++) {
    int h = 5 + i * 3;
    lv_obj_t *b = lv_obj_create(box);
    lv_obj_remove_style_all(b);
    lv_obj_set_size(b, 4, h);
    lv_obj_set_pos(b, i * 6, 16 - h);
    lv_obj_set_style_radius(b, 1, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(b, i < level ? C_TEXT : C_BORDER, 0);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_CLICKABLE);
  }
}

// ---- connection progress overlay ----
static lv_obj_t *connOverlay = nullptr, *connLabel, *connSpinner, *connBtn;
static void connDone(lv_event_t *) { showWifiPage(LV_SCR_LOAD_ANIM_FADE_ON, true); }
static void connRetry(lv_event_t *) {
  if (connOverlay) lv_obj_del(connOverlay);
  connOverlay = nullptr;
}
static void connPoll(lv_timer_t *t) {
  if (!connOverlay) {
    lv_timer_pause(t);
    return;
  }
  int r = wifiConnectResult();
  if (r == 0) return;
  lv_timer_pause(t);
  lv_obj_add_flag(connSpinner, LV_OBJ_FLAG_HIDDEN);
  bool ok = r == 1;
  setText(connLabel, tr(ok ? "Connected" : "Could not connect. Check the password and try again."));
  lv_obj_set_style_text_color(connLabel, ok ? C_OK : C_WARN, 0);
  lv_obj_t *bl = lv_obj_get_child(connBtn, 0);
  setText(bl, tr(ok ? "Done" : "Try again"));
  lv_obj_remove_event_cb(connBtn, connDone);
  lv_obj_remove_event_cb(connBtn, connRetry);
  lv_obj_add_event_cb(connBtn, ok ? connDone : connRetry, LV_EVENT_CLICKED, nullptr);
  lv_obj_clear_flag(connBtn, LV_OBJ_FLAG_HIDDEN);
}
static void startConnect(const char *ssid, const char *password) {
  lv_obj_t *scr = lv_scr_act();
  connOverlay = lv_obj_create(scr);
  lv_obj_remove_style_all(connOverlay);
  lv_obj_set_size(connOverlay, 480, 320);
  lv_obj_set_style_bg_color(connOverlay, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(connOverlay, LV_OPA_70, 0);
  lv_obj_add_flag(connOverlay, LV_OBJ_FLAG_CLICKABLE);  // swallow touches
  lv_obj_t *card = makeCard(connOverlay, 320, 170);
  lv_obj_center(card);
  connSpinner = lv_spinner_create(card, 1000, 70);
  lv_obj_set_size(connSpinner, 44, 44);
  lv_obj_align(connSpinner, LV_ALIGN_TOP_MID, 0, 18);
  lv_obj_set_style_arc_color(connSpinner, C_ACCENT, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(connSpinner, C_BORDER, LV_PART_MAIN);
  char buf[80];
  snprintf(buf, sizeof(buf), tr("Connecting to %s..."), ssid);
  connLabel = makeLabel(card, buf, F_BODY, C_TEXT);
  lv_obj_set_width(connLabel, 280);
  lv_label_set_long_mode(connLabel, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(connLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(connLabel, LV_ALIGN_TOP_MID, 0, 74);
  connBtn = smallBtn(card, "Done", C_ACCENT, connDone, nullptr, 120);
  lv_obj_align(connBtn, LV_ALIGN_BOTTOM_MID, 0, -14);
  lv_obj_add_flag(connBtn, LV_OBJ_FLAG_HIDDEN);
  pageTimer(scr, connPoll, 300);
  wifiConnect(ssid, password);
}

// ---- network list ----
static void netClicked(lv_event_t *e) {
  const NetItem *n = (const NetItem *)lv_event_get_user_data(e);
  if (wifiConnected() && WiFi.SSID() == n->ssid) return;  // already on this network
  if (n->secure) pagePassword(n->ssid);
  else startConnect(n->ssid, "");
}

static void fillNetworks(int n) {
  lv_obj_clean(wifiList);
  int idx[40], cnt = 0;
  for (int i = 0; i < n && cnt < 40; i++) idx[cnt++] = i;
  for (int i = 1; i < cnt; i++) {  // insertion sort by signal, strongest first
    int k = idx[i], j = i - 1;
    while (j >= 0 && wifiScanAp(idx[j])->rssi < wifiScanAp(k)->rssi) {
      idx[j + 1] = idx[j];
      j--;
    }
    idx[j + 1] = k;
  }
  netCount = 0;
  for (int i = 0; i < cnt && netCount < 14; i++) {
    const wifi_ap_record_t *ap = wifiScanAp(idx[i]);
    String s = (const char *)ap->ssid;
    if (s.length() == 0 || s.length() > 32) continue;
    bool dup = false;
    for (int j = 0; j < netCount; j++) dup |= (s == nets[j].ssid);
    if (dup) continue;
    strlcpy(nets[netCount].ssid, s.c_str(), sizeof(nets[netCount].ssid));
    nets[netCount].secure = ap->authmode != WIFI_AUTH_OPEN;
    nets[netCount].rssi = ap->rssi;
    netCount++;
  }
  if (netCount == 0) {
    infoRow(wifiList, "Networks", "None found", C_WARN);
    return;
  }
  for (int i = 0; i < netCount; i++) {
    bool current = wifiConnected() && WiFi.SSID() == nets[i].ssid;
    lv_obj_t *r = makeCard(wifiList, 456, 48);
    lv_obj_add_flag(r, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(r, C_BORDER, LV_STATE_PRESSED);
    if (current) lv_obj_set_style_border_color(r, C_OK, 0);
    lv_obj_t *name = makeLabel(r, nets[i].ssid, F_LABEL, current ? C_OK : C_TEXT);
    lv_obj_set_width(name, 300);
    lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
    lv_obj_align(name, LV_ALIGN_LEFT_MID, 16, 0);
    wifiBars(r, nets[i].rssi);
    lv_obj_align(lv_obj_get_child(r, 1), LV_ALIGN_RIGHT_MID, -16, 0);
    if (current) {
      lv_obj_t *ck = makeLabel(r, LV_SYMBOL_OK, F_BODY, C_OK);
      lv_obj_align(ck, LV_ALIGN_RIGHT_MID, -56, 0);
    } else if (nets[i].secure) {
      lv_obj_t *lk = makeLabel(r, LV_SYMBOL_EYE_CLOSE, F_BODY, C_MUTED);  // closest built-in "locked" glyph
      lv_obj_align(lk, LV_ALIGN_RIGHT_MID, -56, 0);
    }
    lv_obj_add_event_cb(r, netClicked, LV_EVENT_CLICKED, &nets[i]);
  }
}

static void scanPoll(lv_timer_t *t) {
  if (!wifiList) {
    lv_timer_pause(t);
    return;
  }
  int n = wifiScanResult();
  if (n == -1) return;
  lv_timer_pause(t);
  if (n < 0) {
    lv_obj_clean(wifiList);
    infoRow(wifiList, "Scan", "Failed, tap refresh", C_WARN);
    return;
  }
  fillNetworks(n);
}
static void startScan(lv_event_t *) {
  if (!wifiList) return;
  lv_obj_clean(wifiList);
  infoRow(wifiList, "Networks", "Scanning...", C_MUTED);
  wifiScanStart();
  if (scanTimer) lv_timer_resume(scanTimer);
}

static void showWifiPage(lv_scr_load_anim_t anim, bool delOld) {
  lv_obj_t *c;
  lv_obj_t *s = pageBegin("Wi-Fi", &c);
  bool on = wifiConnected();

  // current connection
  lv_obj_t *st = makeCard(c, 456, 64);
  lv_obj_t *ic = makeLabel(st, LV_SYMBOL_WIFI, F_TITLE, on ? C_OK : C_WARN);
  lv_obj_align(ic, LV_ALIGN_LEFT_MID, 16, 0);
  lv_obj_t *nm = makeLabel(st, on ? WiFi.SSID().c_str() : "Not connected", F_LABEL, C_TEXT);
  lv_obj_set_width(nm, 270);
  lv_label_set_long_mode(nm, LV_LABEL_LONG_DOT);
  lv_obj_align(nm, LV_ALIGN_TOP_LEFT, 56, 10);
  char det[96];
  if (on) snprintf(det, sizeof(det), "%s   %d dBm   ch %d", WiFi.localIP().toString().c_str(), WiFi.RSSI(), WiFi.channel());
  else snprintf(det, sizeof(det), "%s", tr(wifiPortalActive() ? "Hotspot Zin100-Setup is open" : wifiStatus()));
  lv_obj_t *dt = makeLabel(st, det, F_CAPTION, C_MUTED);
  lv_obj_set_width(dt, 290);
  lv_label_set_long_mode(dt, LV_LABEL_LONG_DOT);
  lv_obj_align(dt, LV_ALIGN_TOP_LEFT, 56, 38);
  if (on) {
    lv_obj_t *fb = smallBtn(st, "Forget", C_WARN, askForgetWifi, nullptr, 84);
    lv_obj_align(fb, LV_ALIGN_RIGHT_MID, -14, 0);
  }

  // section header with refresh
  lv_obj_t *hdr = lv_obj_create(c);
  lv_obj_remove_style_all(hdr);
  lv_obj_set_size(hdr, 456, 36);
  lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *hl = makeLabel(hdr, "Available networks", F_CAPTION, C_MUTED);
  lv_obj_align(hl, LV_ALIGN_LEFT_MID, 4, 0);
  lv_obj_t *rb = smallBtn(hdr, LV_SYMBOL_REFRESH, C_ACCENT, startScan, nullptr, 48);
  lv_obj_align(rb, LV_ALIGN_RIGHT_MID, 0, 0);

  wifiList = lv_obj_create(c);
  lv_obj_remove_style_all(wifiList);
  lv_obj_set_size(wifiList, 456, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(wifiList, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(wifiList, 8, 0);
  lv_obj_clear_flag(wifiList, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(
      s, [](lv_event_t *) { wifiList = nullptr; scanTimer = nullptr; }, LV_EVENT_DELETE, nullptr);

  scanTimer = pageTimer(s, scanPoll, 400);
  lv_timer_pause(scanTimer);
  startScan(nullptr);
  pageShow(s, anim, delOld);
}
static void pageWifi() { showWifiPage(LV_SCR_LOAD_ANIM_MOVE_LEFT, false); }

// ---- password entry ----
static lv_obj_t *pwTa;
static char pwSsid[33];
static void pwBack(lv_event_t *) { showWifiPage(LV_SCR_LOAD_ANIM_MOVE_RIGHT, true); }
static void pwToggle(lv_event_t *) { lv_textarea_set_password_mode(pwTa, !lv_textarea_get_password_mode(pwTa)); }
static void pwKeyboard(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY) startConnect(pwSsid, lv_textarea_get_text(pwTa));
  else if (code == LV_EVENT_CANCEL) pwBack(e);
}
static void pagePassword(const char *ssid) {
  strlcpy(pwSsid, ssid, sizeof(pwSsid));
  lv_obj_t *c;
  lv_obj_t *s = pageBegin(pwSsid, &c, pwBack);
  lv_obj_del(c);

  pwTa = lv_textarea_create(s);
  lv_textarea_set_one_line(pwTa, true);
  lv_textarea_set_password_mode(pwTa, true);
  lv_textarea_set_max_length(pwTa, 63);
  lv_textarea_set_placeholder_text(pwTa, tr("Password"));
  lv_obj_set_size(pwTa, 396, 44);
  lv_obj_set_pos(pwTa, 12, SB_H + 52);
  lv_obj_set_style_bg_color(pwTa, C_CARD, 0);
  lv_obj_set_style_text_color(pwTa, C_TEXT, 0);
  lv_obj_set_style_border_color(pwTa, C_BORDER, 0);
  lv_obj_set_style_border_width(pwTa, 1, 0);
  lv_obj_set_style_radius(pwTa, 12, 0);
  lv_obj_set_style_border_color(pwTa, C_ACCENT, LV_STATE_FOCUSED);

  lv_obj_t *eye = smallBtn(s, LV_SYMBOL_EYE_OPEN, C_MUTED, pwToggle, nullptr, 52);
  lv_obj_set_size(eye, 52, 44);
  lv_obj_set_pos(eye, 416, SB_H + 52);

  lv_obj_t *kb = lv_keyboard_create(s);
  lv_obj_set_size(kb, 480, 190);
  lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(kb, C_CARD, 0);
  lv_obj_set_style_border_width(kb, 0, 0);
  lv_obj_set_style_bg_color(kb, C_BORDER, LV_PART_ITEMS);
  lv_obj_set_style_text_color(kb, C_TEXT, LV_PART_ITEMS);
  lv_obj_set_style_border_width(kb, 0, LV_PART_ITEMS);
  lv_keyboard_set_textarea(kb, pwTa);
  lv_obj_add_event_cb(kb, pwKeyboard, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(kb, pwKeyboard, LV_EVENT_CANCEL, nullptr);
  pageShow(s, LV_SCR_LOAD_ANIM_MOVE_LEFT, true);  // replaces the Wi-Fi page
}

// =====================================================================================
// Settings
// =====================================================================================
static lv_obj_t *brightnessPct;
static void brightnessEvent(lv_event_t *e) {
  lv_obj_t *sl = lv_event_get_target(e);
  int v = lv_slider_get_value(sl);
  setBacklight(v);
  setTextFmt(brightnessPct, "%d%%", v * 100 / 255);
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
    Preferences p;
    p.begin("ui", false);
    p.putUChar("bl", (uint8_t)v);
    p.end();
  }
}

static const char *sleepMap[5];
static const uint8_t sleepVals[] = {0, 1, 5, 10};
static void sleepEvent(lv_event_t *e) {
  uint32_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
  if (id >= 4) return;
  sleepMin = sleepVals[id];
  Preferences p;
  p.begin("ui", false);
  p.putUChar("sleep", sleepMin);
  p.end();
}

static uint8_t pendingLang = LANG_VI;
static void doChangeLang() {
  i18nSave(pendingLang);
  ESP.restart();
}
static void askLang(lv_event_t *e) {
  uint8_t l = (uint8_t)(intptr_t)lv_event_get_user_data(e);
  if (l == gLang) return;
  pendingLang = l;
  confirmDialog("Change language?", "The device restarts to apply the new language.", doChangeLang);
}

static lv_obj_t *tzLabel;
static int tzCur;
static void tzShow() { setTextFmt(tzLabel, "UTC%+d", tzCur); }
static void tzStep(lv_event_t *e) {
  tzCur = constrain(tzCur + (int)(intptr_t)lv_event_get_user_data(e), -12, 14);
  tzShow();
  wifiSetTimezone(tzCur);
}

static void pageSettings() {
  lv_obj_t *c;
  lv_obj_t *s = pageBegin("Settings", &c);

  sectionLabel(c, "DISPLAY");
  lv_obj_t *card = makeCard(c, 456, 74);
  lv_obj_t *bt = makeLabel(card, "Brightness", F_BODY, C_MUTED);
  lv_obj_align(bt, LV_ALIGN_TOP_LEFT, 16, 10);
  brightnessPct = makeLabel(card, "", F_BODY, C_TEXT);
  lv_obj_align(brightnessPct, LV_ALIGN_TOP_RIGHT, -16, 10);
  setTextFmt(brightnessPct, "%d%%", blLevel * 100 / 255);
  lv_obj_t *sl = lv_slider_create(card);
  lv_slider_set_range(sl, 20, 255);
  lv_slider_set_value(sl, blLevel, LV_ANIM_OFF);
  lv_obj_set_width(sl, 420);
  lv_obj_align(sl, LV_ALIGN_BOTTOM_MID, 0, -16);
  lv_obj_set_style_bg_color(sl, C_BORDER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(sl, C_ACCENT, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(sl, C_TEXT, LV_PART_KNOB);
  lv_obj_add_event_cb(sl, brightnessEvent, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(sl, brightnessEvent, LV_EVENT_RELEASED, nullptr);

  lv_obj_t *card2 = makeCard(c, 456, 86);
  lv_obj_t *st = makeLabel(card2, "When idle for", F_BODY, C_MUTED);
  lv_obj_align(st, LV_ALIGN_TOP_LEFT, 16, 10);
  sleepMap[0] = tr("Never");
  sleepMap[1] = tr("1 min");
  sleepMap[2] = tr("5 min");
  sleepMap[3] = tr("10 min");
  sleepMap[4] = "";
  lv_obj_t *bm = lv_btnmatrix_create(card2);
  lv_btnmatrix_set_map(bm, sleepMap);
  lv_btnmatrix_set_btn_ctrl_all(bm, LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(bm, true);
  for (int i = 0; i < 4; i++)
    if (sleepVals[i] == sleepMin) lv_btnmatrix_set_btn_ctrl(bm, i, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_set_size(bm, 424, 38);
  lv_obj_align(bm, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_bg_opa(bm, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(bm, 0, 0);
  lv_obj_set_style_pad_all(bm, 0, 0);
  lv_obj_set_style_pad_column(bm, 8, 0);
  lv_obj_set_style_text_font(bm, F_BODY, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(bm, C_BORDER, LV_PART_ITEMS);
  lv_obj_set_style_text_color(bm, C_TEXT, LV_PART_ITEMS);
  lv_obj_set_style_border_width(bm, 0, LV_PART_ITEMS);
  lv_obj_set_style_shadow_width(bm, 0, LV_PART_ITEMS);
  lv_obj_set_style_radius(bm, 10, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(bm, C_ACCENT, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(bm, lv_color_hex(0x04101A), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_add_event_cb(bm, sleepEvent, LV_EVENT_VALUE_CHANGED, nullptr);

  // what happens after the idle time
  static const char *idleMap[3];
  idleMap[0] = tr("Turn screen off");
  idleMap[1] = tr("Ambient clock");
  idleMap[2] = "";
  lv_obj_t *card3 = makeCard(c, 456, 86);
  lv_obj_t *it = makeLabel(card3, "Then", F_BODY, C_MUTED);
  lv_obj_align(it, LV_ALIGN_TOP_LEFT, 16, 10);
  lv_obj_t *im = lv_btnmatrix_create(card3);
  lv_btnmatrix_set_map(im, idleMap);
  lv_btnmatrix_set_btn_ctrl_all(im, LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(im, true);
  lv_btnmatrix_set_btn_ctrl(im, cfg.idleMode ? 1 : 0, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_set_size(im, 424, 38);
  lv_obj_align(im, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_bg_opa(im, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(im, 0, 0);
  lv_obj_set_style_pad_all(im, 0, 0);
  lv_obj_set_style_pad_column(im, 8, 0);
  lv_obj_set_style_text_font(im, F_BODY, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(im, C_BORDER, LV_PART_ITEMS);
  lv_obj_set_style_text_color(im, C_TEXT, LV_PART_ITEMS);
  lv_obj_set_style_border_width(im, 0, LV_PART_ITEMS);
  lv_obj_set_style_shadow_width(im, 0, LV_PART_ITEMS);
  lv_obj_set_style_radius(im, 10, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(im, C_ACCENT, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(im, lv_color_hex(0x04101A), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_add_event_cb(
      im,
      [](lv_event_t *e) {
        uint32_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
        if (id > 1) return;
        cfg.idleMode = id;
        cfgSave();
      },
      LV_EVENT_VALUE_CHANGED, nullptr);

  // wallpaper of the main screen: pick a picture from the memory card, or remove it
  sectionLabel(c, DL("HÌNH NỀN", "WALLPAPER"));
  actionBtn(c, DL("Chọn hình nền từ thẻ nhớ", "Choose a wallpaper from the card"), C_ACCENT, [](lv_event_t *) {
    pageReplace = true;  // the Settings page is replaced by the picker
    pageWallpaper();
    pageReplace = false;
  });
  if (cfg.wallOn)
    actionBtn(c, DL("Gỡ hình nền", "Remove the wallpaper"), C_DANGER, [](lv_event_t *) {
      wallpaperOff();
      cfg.wallOn = false;
      cfgSave();
      uiToast(DL("Đã gỡ hình nền", "Wallpaper removed"), C_ACCENT);
    });

  sectionLabel(c, "GENERAL");
  lv_obj_t *tz = makeCard(c, 456, 52);
  lv_obj_t *tl = makeLabel(tz, "Time zone", F_BODY, C_MUTED);
  lv_obj_align(tl, LV_ALIGN_LEFT_MID, 16, 0);
  tzCur = wifiTimezone();
  lv_obj_t *plus = smallBtn(tz, LV_SYMBOL_PLUS, C_ACCENT, tzStep, (void *)(intptr_t)1);
  lv_obj_align(plus, LV_ALIGN_RIGHT_MID, -10, 0);
  tzLabel = makeLabel(tz, "", F_LABEL, C_TEXT);
  lv_obj_set_width(tzLabel, 70);
  lv_obj_set_style_text_align(tzLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(tzLabel, LV_ALIGN_RIGHT_MID, -50, 0);
  tzShow();
  lv_obj_t *minus = smallBtn(tz, LV_SYMBOL_MINUS, C_ACCENT, tzStep, (void *)(intptr_t)-1);
  lv_obj_align(minus, LV_ALIGN_RIGHT_MID, -126, 0);

  lv_obj_t *lg = makeCard(c, 456, 52);
  lv_obj_t *ll = makeLabel(lg, "Language", F_BODY, C_MUTED);
  lv_obj_align(ll, LV_ALIGN_LEFT_MID, 16, 0);
  lv_obj_t *lEn = smallBtn(lg, "English", gLang == LANG_EN ? C_ACCENT : C_MUTED, askLang, (void *)(intptr_t)LANG_EN, 100);
  lv_obj_align(lEn, LV_ALIGN_RIGHT_MID, -10, 0);
  lv_obj_t *lVi = smallBtn(lg, "Tiếng Việt", gLang == LANG_VI ? C_ACCENT : C_MUTED, askLang, (void *)(intptr_t)LANG_VI, 110);
  lv_obj_align(lVi, LV_ALIGN_RIGHT_MID, -118, 0);

  infoRow(c, "Wi-Fi", wifiConnected() ? WiFi.SSID().c_str() : "Not connected", wifiConnected() ? C_OK : C_WARN);
  if (wifiConnected()) {
    String url = String("http://") + webHost() + ".local";
    infoRow(c, "Web setup", url.c_str(), C_ACCENT);
  }
  infoRow(c, "Xiaozhi", xiaozhiState() == XZ_ACTIVATED ? "Connected" : xiaozhiMessage(),
          xiaozhiState() == XZ_ACTIVATED ? C_OK : C_WARN);

  sectionLabel(c, "SYSTEM");
  actionBtn(c, "Recalibrate touch", C_ACCENT, askRecalibrate);
  actionBtn(c, "Restart device", C_WARN, askRestart);
  actionBtn(c, "Factory reset", C_DANGER, askFactoryReset);
  pageShow(s);
}

// =====================================================================================
// Storage
// =====================================================================================
static void fmtBytes(uint64_t b, char *out, size_t n) {
  if (b >= 1048576ULL * 1024) snprintf(out, n, "%.2f GB", b / 1073741824.0);
  else if (b >= 1048576ULL) snprintf(out, n, "%.1f MB", b / 1048576.0);
  else if (b >= 1024) snprintf(out, n, "%u KB", (unsigned)(b / 1024));
  else snprintf(out, n, "%u B", (unsigned)b);
}

static void pageStorage() {
  lv_obj_t *c;
  lv_obj_t *s = pageBegin("Storage", &c);
  if (!sdOk) {
    infoRow(c, "microSD", "No card detected", C_WARN);
  } else {
    uint64_t total = SD.cardSize(), used = SD.usedBytes();
    int pct = total ? (int)(used * 100 / total) : 0;
    if (pct == 0 && used > 0) pct = 1;
    lv_obj_t *card = makeCard(c, 456, 74);
    lv_obj_t *t = makeLabel(card, "microSD", F_BODY, C_MUTED);
    lv_obj_align(t, LV_ALIGN_TOP_LEFT, 16, 10);
    char buf[48];
    snprintf(buf, sizeof(buf), tr("%d%% used"), pct);
    lv_obj_t *p = makeLabel(card, buf, F_BODY, C_TEXT);
    lv_obj_align(p, LV_ALIGN_TOP_RIGHT, -16, 10);
    lv_obj_t *bar = lv_bar_create(card);
    lv_obj_set_size(bar, 424, 10);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -18);
    lv_bar_set_value(bar, pct, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, C_ACCENT, LV_PART_INDICATOR);
    fmtBytes(total, buf, sizeof(buf));
    infoRow(c, "Capacity", buf);
    fmtBytes(used, buf, sizeof(buf));
    infoRow(c, "Used", buf);
    fmtBytes(total > used ? total - used : 0, buf, sizeof(buf));
    infoRow(c, "Free", buf, C_OK);
    snprintf(buf, sizeof(buf), "%d", photosCount());
    infoRow(c, DL("Ảnh tìm thấy", "Photos found"), buf, C_ACCENT);

    // what is on the card (top level)
    sectionLabel(c, DL("NỘI DUNG THẺ", "CARD CONTENTS"));
    File root = SD.open("/");
    int shown = 0;
    for (File f = root.openNextFile(); f && shown < 40; f = root.openNextFile(), shown++) {
      String nm = f.name();
      int sl = nm.lastIndexOf('/');
      if (sl >= 0) nm = nm.substring(sl + 1);
      bool dir = f.isDirectory();
      if (dir) strlcpy(buf, DL("Thư mục", "Folder"), sizeof(buf));
      else fmtBytes(f.size(), buf, sizeof(buf));
      f.close();
      lv_obj_t *r = infoRow(c, nm.c_str(), buf, dir ? C_ACCENT : lv_color_hex(0xF2F5FF));
      lv_obj_t *k = lv_obj_get_child(r, 0), *v = lv_obj_get_child(r, 1);  // long names must not run under the size
      lv_obj_set_size(k, 320, 20);  // fixed height: cut long names with "..."
      lv_label_set_long_mode(k, LV_LABEL_LONG_DOT);
      lv_obj_set_width(v, 100);
    }
    root.close();
    if (!shown) infoRow(c, DL("Thẻ trống", "The card is empty"), "", C_MUTED);
  }
  pageShow(s);
}

// =====================================================================================
// About
// =====================================================================================
static const char *resetReasonText() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return "Power on";
    case ESP_RST_SW: return "Software restart";
    case ESP_RST_PANIC: return "Crash";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT: return "Watchdog";
    case ESP_RST_BROWNOUT: return "Brownout";
    case ESP_RST_DEEPSLEEP: return "Deep sleep";
    default: return "Other";
  }
}

static lv_obj_t *aboutUp, *aboutHeap;
static void aboutTick(lv_timer_t *) {
  char buf[40];
  uint32_t up = millis() / 1000;
  snprintf(buf, sizeof(buf), "%lud %02luh %02lum %02lus", (unsigned long)(up / 86400), (unsigned long)((up / 3600) % 24),
           (unsigned long)((up / 60) % 60), (unsigned long)(up % 60));
  setRowValue(aboutUp, buf);
  snprintf(buf, sizeof(buf), tr("%u KB free"), (unsigned)(ESP.getFreeHeap() / 1024));
  setRowValue(aboutHeap, buf);
}

static void pageAbout() {
  lv_obj_t *c;
  lv_obj_t *s = pageBegin("About", &c);
  char buf[72];

  // identity card with a tiny mascot
  lv_obj_t *id = makeCard(c, 456, 84);
  lv_obj_t *face = lv_obj_create(id);
  lv_obj_remove_style_all(face);
  lv_obj_set_size(face, 56, 56);
  lv_obj_align(face, LV_ALIGN_LEFT_MID, 16, 0);
  lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(face, 2, 0);
  lv_obj_set_style_border_color(face, C_ACCENT2, 0);
  lv_obj_set_style_bg_opa(face, LV_OPA_10, 0);
  lv_obj_set_style_bg_color(face, C_ACCENT2, 0);
  for (int i = 0; i < 2; i++) {
    lv_obj_t *e = lv_obj_create(face);
    lv_obj_remove_style_all(e);
    lv_obj_set_size(e, 9, 18);
    lv_obj_set_style_radius(e, 5, 0);
    lv_obj_set_style_bg_opa(e, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(e, C_ACCENT, 0);
    lv_obj_align(e, LV_ALIGN_CENTER, i ? 9 : -9, 0);
  }
  lv_obj_t *nm = makeLabel(id, "Zin100", F_HEADING, C_TEXT);
  lv_obj_align(nm, LV_ALIGN_TOP_LEFT, 88, 12);
  snprintf(buf, sizeof(buf), tr("Firmware %s  -  built %s"), FW_VERSION, __DATE__);
  lv_obj_t *fw = makeLabel(id, buf, F_CAPTION, C_MUTED);
  lv_obj_align(fw, LV_ALIGN_TOP_LEFT, 88, 52);

  sectionLabel(c, "HARDWARE");
  infoRow(c, "Board", "ESP32-2432S035");
  snprintf(buf, sizeof(buf), "%s rev %d", ESP.getChipModel(), (int)ESP.getChipRevision());
  infoRow(c, "Chip", buf);
  snprintf(buf, sizeof(buf), tr("%d MHz, %d cores"), (int)ESP.getCpuFreqMHz(), (int)ESP.getChipCores());
  infoRow(c, "CPU", buf);
  snprintf(buf, sizeof(buf), "%u MB flash, %u KB RAM", (unsigned)(ESP.getFlashChipSize() / 1048576),
           (unsigned)(ESP.getHeapSize() / 1024));
  infoRow(c, "Memory", buf);
  infoRow(c, "Display", "3.5-inch 480x320 ST7796");
  infoRow(c, "Touch", "Resistive XPT2046");
  if (sdOk) snprintf(buf, sizeof(buf), "%llu MB", (unsigned long long)(SD.cardSize() / 1048576ULL));
  else snprintf(buf, sizeof(buf), "%s", tr("No card"));
  infoRow(c, "microSD", buf, sdOk ? C_TEXT : C_WARN);

  sectionLabel(c, "NETWORK");
  infoRow(c, "Wi-Fi", wifiConnected() ? WiFi.SSID().c_str() : "Not connected", wifiConnected() ? C_OK : C_WARN);
  infoRow(c, "IP address", wifiConnected() ? WiFi.localIP().toString().c_str() : "-");
  infoRow(c, "MAC", WiFi.macAddress().c_str());
  infoRow(c, "Xiaozhi", xiaozhiState() == XZ_ACTIVATED ? "Connected" : xiaozhiMessage(),
          xiaozhiState() == XZ_ACTIVATED ? C_OK : C_WARN);

  sectionLabel(c, "SYSTEM");
  aboutUp = infoRow(c, "Uptime", "");
  aboutHeap = infoRow(c, "Memory now", "");
  infoRow(c, "Last restart", resetReasonText());
  infoRow(c, "SDK", ESP.getSdkVersion());
  aboutTick(nullptr);
  pageTimer(s, aboutTick, 1000);
  pageShow(s);
}

// =====================================================================================
// placeholder
// =====================================================================================
static void pageSoon(const char *title, const char *icon) {
  lv_obj_t *c;
  lv_obj_t *s = pageBegin(title, &c);
  lv_obj_set_flex_align(c, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  makeLabel(c, icon, F_DISPLAY, C_MUTED);
  makeLabel(c, "Coming soon", F_HEADING, C_TEXT);
  char buf[64];
  snprintf(buf, sizeof(buf), tr("%s is not available yet"), tr(title));
  makeLabel(c, buf, F_BODY, C_MUTED);
  pageShow(s);
}
