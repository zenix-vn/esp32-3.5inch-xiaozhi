#pragma once
// User options edited from the web setup page (namespace "cfg"). Display options apply live;
// layout options (language, start screen, QR page, app list) are read once at boot.
#include <Arduino.h>
#include <Preferences.h>

static const int APP_COUNT = 16;
static const int SETTINGS_APP = 6;  // index of the Settings app; it can never be hidden

static const char *QR_DEFAULT_PAYLOAD =
    "00020101021138570010A000000727012700069704220113VQRQAHIZT41140208QRIBFTTA53037045802VN62150107NPS686908006304C8A9";

// Wi-Fi (4), Storage (5) and About (7) live in Settings now, not in the launcher; 0 (the AI assistant) is gone.
static bool appInLauncher(int id) { return id != 0 && id != 4 && id != 5 && id != 7; }
// app ids: 0 (removed: assistant), 1 weather, 2 dragon, 3 photos, 4 wi-fi, 5 storage, 6 settings, 7 about, 8 clock, 9 payment, 10 calendar, 11 sleep sounds, 12 games, 13 radio, 14 countdown, 15 kids letters
static const char *APPS_DEFAULT = "2,1,8,10,14,15,11,12,13,9,3,6";

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
  uint8_t photoSec = 10;  // slideshow: seconds per picture
  bool wallOn = false;    // a wallpaper is stored in flash and shown on the main screen
  uint8_t ledMode = 2;    // front RGB LED (led.h): 0 off, 1 alerts only, 2 everything
  uint8_t ledBri = 40;    // LED brightness 5..100 %
  bool ledTouch = false;  // LED blinks on every touch
  bool ledNight = true;   // only alarms and an empty battery at night / in do-not-disturb
  int8_t dock[3] = {8, 14, 15};  // the home screen dock: app ids (clock, countdown, kids letters), -1 = empty
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
      if (v >= 0 && v < APP_COUNT && appInLauncher(v) && String(v) == t && !cfgAppsContain(out, v)) out += (out.length() ? "," : "") + t;
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
  uint8_t appsV = p.getUChar("appsV", 1);
  if (appsV < 2) {  // V1 list: id 1 was Music (now Weather); add the new apps
    apps = apps == "0,1,2,3,4,5,6,7" ? String(APPS_DEFAULT) : apps + ",8,9";
  }
  if (appsV < 4 && !cfgAppsContain(apps, 10)) apps += ",10";  // the Calendar app
  if (appsV < 5 && !cfgAppsContain(apps, 11)) apps += ",11";  // the Sleep sounds app
  if (appsV < 6 && !cfgAppsContain(apps, 12)) apps += ",12";  // the Games app
  if (appsV < 7 && !cfgAppsContain(apps, 13)) apps += ",13";  // the Radio app
  if (appsV < 8 && !cfgAppsContain(apps, 14)) apps += ",14";  // Countdown
  if (appsV < 8 && !cfgAppsContain(apps, 15)) apps += ",15";  // the kids' alphabet game
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
  cfg.photoSec = constrain(p.getUChar("phS", 10), 3, 120);
  cfg.wallOn = p.getBool("wall", false);
  cfg.ledMode = min((int)p.getUChar("ledM", 2), 2);
  cfg.ledBri = constrain(p.getUChar("ledB", 40), 5, 100);
  cfg.ledTouch = p.getBool("ledT", false);
  cfg.ledNight = p.getBool("ledN", true);
  if (p.getBytesLength("dock") == 3) p.getBytes("dock", cfg.dock, 3);
  for (auto &d : cfg.dock)
    if (d < -1 || d >= APP_COUNT || (d >= 0 && !appInLauncher(d))) d = -1;
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
  p.putUChar("phS", cfg.photoSec);
  p.putBool("wall", cfg.wallOn);
  p.putUChar("ledM", cfg.ledMode);
  p.putUChar("ledB", cfg.ledBri);
  p.putBool("ledT", cfg.ledTouch);
  p.putBool("ledN", cfg.ledNight);
  p.putBytes("dock", cfg.dock, 3);
  p.remove("vmk");  // settings of the removed Vietmap app
  p.remove("veh");
  p.remove("spw");
  p.putUChar("appsV", 8);
  p.end();
}

// ---------- payment QR accounts ----------
// A list of accounts (managed on the web page, chosen in Settings). The selected one is copied into cfg.qr*,
// which the main-screen QR tile and the Payment app read. Stored in NVS namespace "qrs".
struct QrAccount {
  String title, bank, name, acct, payload;
};
static const int QR_MAX = 8;
static QrAccount qrAccs[QR_MAX];
static int qrCount = 0, qrSel = 0;

static void qrApply() {
  if (qrSel >= qrCount) qrSel = 0;
  const QrAccount &a = qrAccs[qrSel];
  cfg.qrTitle = a.title;
  cfg.qrBank = a.bank;
  cfg.qrName = a.name;
  cfg.qrAcct = a.acct;
  cfg.qrPayload = a.payload;
}

static void qrSave() {
  Preferences p;
  p.begin("qrs", false);
  p.putUChar("n", qrCount);
  p.putUChar("sel", qrSel);
  for (int i = 0; i < QR_MAX; i++) {
    char k[4][4] = {"t0", "b0", "n0", "a0"}, kp[4] = "p0";
    for (auto &x : k) x[1] = '0' + i;
    kp[1] = '0' + i;
    if (i < qrCount) {
      p.putString(k[0], qrAccs[i].title);
      p.putString(k[1], qrAccs[i].bank);
      p.putString(k[2], qrAccs[i].name);
      p.putString(k[3], qrAccs[i].acct);
      p.putString(kp, qrAccs[i].payload);
    } else {
      for (auto &x : k) p.remove(x);
      p.remove(kp);
    }
  }
  p.end();
}

// Call after cfgLoad(): the first time, the single account of the earlier firmware becomes account 1.
static void qrLoad() {
  Preferences p;
  p.begin("qrs", false);
  int n = p.getUChar("n", 0);
  if (n < 1 || n > QR_MAX) {
    p.end();
    qrCount = 1;
    qrSel = 0;
    qrAccs[0] = {cfg.qrTitle, cfg.qrBank, cfg.qrName, cfg.qrAcct, cfg.qrPayload};
    qrSave();
    return;
  }
  qrCount = n;
  qrSel = p.getUChar("sel", 0);
  for (int i = 0; i < n; i++) {
    char t[4] = "t0", b[4] = "b0", nm[4] = "n0", a[4] = "a0", pl[4] = "p0";
    t[1] = b[1] = nm[1] = a[1] = pl[1] = '0' + i;
    qrAccs[i] = {p.getString(t, ""), p.getString(b, ""), p.getString(nm, ""), p.getString(a, ""), p.getString(pl, "")};
  }
  p.end();
  qrApply();
}

// "NAME  -  1234567890" for lists
static String qrLabel(int i) {
  const QrAccount &a = qrAccs[i];
  String n = a.name.length() ? a.name : a.title;
  if (!n.length()) n = String("#") + (i + 1);
  return a.acct.length() ? n + "  -  " + a.acct : n;
}
