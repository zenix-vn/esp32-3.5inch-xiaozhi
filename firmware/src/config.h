#pragma once
// User options edited from the web setup page (namespace "cfg"). Display options apply live;
// layout options (language, start screen, QR page, app list) are read once at boot.
#include <Arduino.h>
#include <Preferences.h>

static const int APP_COUNT = 10;
static const int SETTINGS_APP = 6;  // index of the Settings app; it can never be hidden

static const char *QR_DEFAULT_PAYLOAD =
    "00020101021138570010A000000727012700069704220113VQRQAHIZT41140208QRIBFTTA53037045802VN62150107NPS686908006304C8A9";

// app ids: 0 assistant, 1 weather, 2 dragon, 3 photos, 4 wi-fi, 5 storage, 6 settings, 7 about, 8 clock, 9 payment
static const char *APPS_DEFAULT = "0,2,1,8,9,3,4,6,5,7";

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
  String apps = APPS_DEFAULT;  // visible apps in launcher order
  // V2
  uint8_t theme = 0;        // 0 sky, 1 forest, 2 sunset, 3 mono
  uint8_t volume = 60;      // speaker 0..100
  bool dnd = false;         // do not disturb: no sounds, no dragon nagging
  bool fxOn = true;         // weather effects on the home screen
  bool lunarOn = true;      // lunar date on the home screen
  bool quoteOn = true;      // quote of the day on the home screen
  uint8_t idleMode = 1;     // what the screen does after the idle time: 0 turn off, 1 ambient clock
  int8_t ambStart = 22;     // nightly ambient schedule (hours, -1 = off)
  int8_t ambEnd = 6;
  bool alarmOn = false;
  uint8_t alarmH = 6, alarmM = 30;
  uint8_t photoSec = 10;  // slideshow: seconds per picture
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
  String apps = p.getString("apps", APPS_DEFAULT);
  if (p.getUChar("appsV", 1) < 2) {  // V1 list: id 1 was Music (now Weather); add the new apps
    apps = apps == "0,1,2,3,4,5,6,7" ? String(APPS_DEFAULT) : apps + ",8,9";
  }
  cfg.apps = cfgCleanApps(apps);
  cfg.theme = p.getUChar("theme", 0) & 3;
  cfg.volume = constrain(p.getUChar("vol", 60), 0, 100);
  cfg.dnd = p.getBool("dnd", false);
  cfg.fxOn = p.getBool("fx", true);
  cfg.lunarOn = p.getBool("lunar", true);
  cfg.quoteOn = p.getBool("quote", true);
  cfg.idleMode = p.getUChar("idleM", 1) ? 1 : 0;
  cfg.ambStart = p.getChar("ambS", 22);
  cfg.ambEnd = p.getChar("ambE", 6);
  cfg.alarmOn = p.getBool("alOn", false);
  cfg.alarmH = p.getUChar("alH", 6) % 24;
  cfg.alarmM = p.getUChar("alM", 30) % 60;
  cfg.photoSec = constrain(p.getUChar("phS", 10), 3, 120);
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
  p.putUChar("theme", cfg.theme);
  p.putUChar("vol", cfg.volume);
  p.putBool("dnd", cfg.dnd);
  p.putBool("fx", cfg.fxOn);
  p.putBool("lunar", cfg.lunarOn);
  p.putBool("quote", cfg.quoteOn);
  p.putUChar("idleM", cfg.idleMode);
  p.putChar("ambS", cfg.ambStart);
  p.putChar("ambE", cfg.ambEnd);
  p.putBool("alOn", cfg.alarmOn);
  p.putUChar("alH", cfg.alarmH);
  p.putUChar("alM", cfg.alarmM);
  p.putUChar("phS", cfg.photoSec);
  p.putUChar("appsV", 2);
  p.end();
}
