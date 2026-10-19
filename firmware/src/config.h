#pragma once
// User options edited from the web setup page (namespace "cfg"). Display options apply live;
// layout options (language, start screen, QR page, app list) are read once at boot.
#include <Arduino.h>
#include <Preferences.h>

static const int APP_COUNT = 8;
static const int SETTINGS_APP = 6;  // index of the Settings app; it can never be hidden

static const char *QR_DEFAULT_PAYLOAD =
    "00020101021138570010A000000727012700069704220113VQRQAHIZT41140208QRIBFTTA53037045802VN62150107NPS686908006304C8A9";

struct AppCfg {
  uint8_t startTile = 0;    // 0 home, 1 payment QR, 2 apps
  uint16_t idleReturn = 0;  // seconds of inactivity before going back to the start screen, 0 = never
  bool qrOn = true;         // show the payment QR page
  bool clock24 = true;
  uint8_t dateFmt = 0;      // 0 long ("Monday, 19 October"), 1 numeric (19/10/2026), 2 hidden
  bool greetOn = true;
  bool tempF = false;
  String userName;
  String qrPayload = QR_DEFAULT_PAYLOAD;
  String qrName = "HO KINH DOANH ZPLUS";
  String qrAcct = "1122100288";
  String qrBank = "VietQR  |  MB Bank";
  String qrTitle;            // empty = built-in "Scan to pay"
  String apps = "0,1,2,3,4,5,6,7";  // visible apps in launcher order
};
static AppCfg cfg;

static bool cfgAppsContain(const String &csv, int idx) {
  String needle = String(idx);
  int from = 0;
  while (from <= (int)csv.length()) {
    int to = csv.indexOf(',', from);
    if (to < 0) to = csv.length();
    if (csv.substring(from, to) == needle) return true;
    from = to + 1;
  }
  return false;
}

// Keeps only valid, unique indices and guarantees the Settings app is present.
static String cfgCleanApps(const String &csv) {
  String out;
  int from = 0;
  while (from <= (int)csv.length()) {
    int to = csv.indexOf(',', from);
    if (to < 0) to = csv.length();
    String t = csv.substring(from, to);
    t.trim();
    if (t.length() && t.length() <= 2) {
      int v = t.toInt();
      if (v >= 0 && v < APP_COUNT && String(v) == t && !cfgAppsContain(out, v)) out += (out.length() ? "," : "") + t;
    }
    from = to + 1;
  }
  if (!cfgAppsContain(out, SETTINGS_APP)) out += (out.length() ? "," : "") + String(SETTINGS_APP);
  return out;
}

static void cfgLoad() {
  Preferences p;
  p.begin("cfg", false);
  cfg.startTile = constrain(p.getUChar("start", 0), 0, 2);
  cfg.idleReturn = p.getUShort("idle", 0);
  cfg.qrOn = p.getBool("qrOn", true);
  cfg.clock24 = p.getBool("c24", true);
  cfg.dateFmt = constrain(p.getUChar("datef", 0), 0, 2);
  cfg.greetOn = p.getBool("greet", true);
  cfg.tempF = p.getBool("tempF", false);
  cfg.userName = p.getString("name", "");
  cfg.qrPayload = p.getString("qrP", QR_DEFAULT_PAYLOAD);
  cfg.qrName = p.getString("qrN", "HO KINH DOANH ZPLUS");
  cfg.qrAcct = p.getString("qrA", "1122100288");
  cfg.qrBank = p.getString("qrB", "VietQR  |  MB Bank");
  cfg.qrTitle = p.getString("qrT", "");
  cfg.apps = cfgCleanApps(p.getString("apps", "0,1,2,3,4,5,6,7"));
  p.end();
  if (!cfg.qrOn && cfg.startTile == 1) cfg.startTile = 0;
}

static void cfgSave() {
  Preferences p;
  p.begin("cfg", false);
  p.putUChar("start", cfg.startTile);
  p.putUShort("idle", cfg.idleReturn);
  p.putBool("qrOn", cfg.qrOn);
  p.putBool("c24", cfg.clock24);
  p.putUChar("datef", cfg.dateFmt);
  p.putBool("greet", cfg.greetOn);
  p.putBool("tempF", cfg.tempF);
  p.putString("name", cfg.userName);
  p.putString("qrP", cfg.qrPayload);
  p.putString("qrN", cfg.qrName);
  p.putString("qrA", cfg.qrAcct);
  p.putString("qrB", cfg.qrBank);
  p.putString("qrT", cfg.qrTitle);
  p.putString("apps", cfg.apps);
  p.end();
}
