// Wi-Fi: saved-credentials connect, "Zin100-Setup" captive portal, in-app connect, NTP time zone.
#include "wifi_setup.h"
#include "i18n.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <time.h>
#include <esp_wifi.h>

static const char *AP_NAME = "Zin100-Setup";
static const uint32_t CONNECT_TIMEOUT_MS = 20000;
static const uint32_t ATTEMPT_TIMEOUT_MS = 15000;
static const int PIN_BOOT = 0;

static Preferences prefs;
static WebServer server(80);
static DNSServer dns;
static String ssid, pass, scanOptions;
static bool portalOn = false, ntpStarted = false;
static uint32_t connectStart = 0, bootHeldAt = 0;
static char statusBuf[64] = "Wi-Fi: starting";
static int tzHours = 7;

// in-app connection attempt (Wi-Fi page)
static bool attempting = false;
static uint32_t attemptStart = 0;
static String newSsid, newPass;
static volatile int attemptResult = 0;  // 0 running, 1 connected, -1 failed

static void startNtp() {
  configTime(tzHours * 3600, 0, "pool.ntp.org", "time.google.com");
  ntpStarted = true;
}

static void stopPortal() {
  if (!portalOn) return;
  server.stop();
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  portalOn = false;
}

static void startPortal() {
  if (portalOn) return;
  portalOn = true;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_NAME);
  dns.start(53, "*", WiFi.softAPIP());

  // Scan once now; the page lists the networks as tappable rows (strongest first), "/scan" refreshes.
  auto scan = []() {
    int n = WiFi.scanNetworks();
    int idx[30], cnt = 0;
    for (int i = 0; i < n && cnt < 30; i++) {
      String s = WiFi.SSID(i);
      bool dup = s.length() == 0;
      for (int k = 0; k < cnt && !dup; k++) dup = WiFi.SSID(idx[k]) == s;
      if (!dup) idx[cnt++] = i;
    }
    for (int i = 1; i < cnt; i++)
      for (int j = i; j > 0 && WiFi.RSSI(idx[j]) > WiFi.RSSI(idx[j - 1]); j--) std::swap(idx[j], idx[j - 1]);
    scanOptions = "";
    for (int k = 0; k < cnt; k++) {
      int i = idx[k], r = WiFi.RSSI(i);
      String esc = WiFi.SSID(i);
      esc.replace("&", "&amp;");
      esc.replace("<", "&lt;");
      esc.replace("\"", "&quot;");
      int bars = r > -55 ? 4 : r > -65 ? 3 : r > -75 ? 2 : 1;
      scanOptions += "<div class=net data-s=\"" + esc + "\"><span>" + esc + "</span><b>" + String("▂▄▆█").substring(0, bars * 3) +
                     (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "" : " 🔒") + "</b></div>";
    }
    WiFi.scanDelete();
  };
  scan();

  auto page = []() {
    bool vi = isVi();
    String h = String("<!doctype html><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
               "<title>Zin100 Wi-Fi</title><style>body{font-family:system-ui,sans-serif;background:#0b1020;color:#eef;padding:20px;max-width:440px;margin:auto}"
               "input,button,a.b{display:block;width:100%;padding:12px;margin:8px 0;border-radius:10px;border:0;font-size:16px;box-sizing:border-box;text-align:center}"
               "button{background:#38e1ff;font-weight:bold;color:#04101a}a.b{background:#1f2849;color:#eef;text-decoration:none}"
               ".net{display:flex;justify-content:space-between;padding:13px 12px;margin:6px 0;border-radius:10px;background:#151b33;border:1px solid #2a3359;cursor:pointer}"
               ".net.on{border-color:#38e1ff;background:#123047}.net b{color:#8a93b8;font-weight:normal;white-space:nowrap;margin-left:10px}"
               "p{color:#8a93b8;font-size:14px}</style><h2>Zin100 Wi-Fi</h2><p>") +
               (vi ? "Chạm vào mạng Wi-Fi để chọn, nhập mật khẩu rồi Lưu. Thiết bị chỉ dùng được Wi-Fi 2,4 GHz."
                   : "Tap a network, type its password and Save. The device only works with 2.4 GHz Wi-Fi.") +
               "</p><div id=l>" + scanOptions + "</div><a class=b href=/scan>" + (vi ? "Quét lại" : "Scan again") +
               "</a><form method=POST action=/save><input name=s id=s placeholder='" + (vi ? "Tên Wi-Fi" : "Wi-Fi name") +
               "' required><input name=p type=password placeholder='" + (vi ? "Mật khẩu" : "Password") + "'><button>" +
               (vi ? "Lưu và kết nối" : "Save and connect") + "</button></form>"
               "<script>document.querySelectorAll('.net').forEach(n=>n.onclick=()=>{document.querySelectorAll('.net').forEach(x=>x.classList.remove('on'));"
               "n.classList.add('on');document.getElementById('s').value=n.dataset.s;document.querySelector('[name=p]').focus()})</script>";
    server.send(200, "text/html", h);
  };
  server.on("/", page);
  server.on("/scan", [scan]() {
    scan();
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  server.on("/save", HTTP_POST, []() {
    prefs.putString("ssid", server.arg("s"));
    prefs.putString("pass", server.arg("p"));
    server.send(200, "text/html", String("<meta charset=utf-8><meta name=viewport content='width=device-width'><h3>") +
                                      (isVi() ? "Đã lưu. Thiết bị đang khởi động lại..." : "Saved. Restarting...") + "</h3>");
    delay(800);
    ESP.restart();
  });
  server.onNotFound(page);  // captive portal redirect
  server.begin();
}

void wifiBegin() {
  prefs.begin("wifi", false);
  pinMode(PIN_BOOT, INPUT_PULLUP);
  Preferences ui;
  ui.begin("ui", true);
  tzHours = ui.getInt("tz", 7);
  ui.end();
  prefs.remove("ssid2");  // hotspot saved by the removed Vietmap app
  prefs.remove("pass2");
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  if (ssid.length() == 0) {
    startPortal();
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setSleep(false);  // always powered: modem sleep made the access point drop the DHCP lease (IP 0.0.0.0) after a while
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid.c_str(), pass.c_str());
    connectStart = millis();
  }
}

void wifiConnect(const char *s, const char *p) {
  newSsid = s;
  newPass = p;
  attemptResult = 0;
  attempting = true;
  attemptStart = millis();
  WiFi.disconnect();
  WiFi.begin(s, p);
}

int wifiConnectResult() { return attemptResult; }

void wifiLoop() {
  if (portalOn) {
    dns.processNextRequest();
    server.handleClient();
  }
  // hold BOOT for 3 s to forget the saved network
  if (digitalRead(PIN_BOOT) == LOW) {
    if (!bootHeldAt) bootHeldAt = millis();
    if (millis() - bootHeldAt > 3000) {
      prefs.clear();
      ESP.restart();
    }
  } else {
    bootHeldAt = 0;
  }

  if (attempting) {
    uint32_t el = millis() - attemptStart;
    if (el > 800 && WiFi.status() == WL_CONNECTED && WiFi.SSID() == newSsid) {
      ssid = newSsid;
      pass = newPass;
      prefs.putString("ssid", ssid);
      prefs.putString("pass", pass);
      stopPortal();
      attempting = false;
      attemptResult = 1;
    } else if (el > ATTEMPT_TIMEOUT_MS) {
      attempting = false;
      attemptResult = -1;
      WiFi.disconnect();
      if (ssid.length()) WiFi.begin(ssid.c_str(), pass.c_str());  // back to the previous network
      connectStart = millis();
    }
  }

  // The status text is only read by the UI: rebuilding it (two String allocations) on every loop pass
  // churned the heap a couple of hundred times a second.
  static uint32_t statusAt = 0;
  if (millis() - statusAt < 250) return;
  statusAt = millis();
  if (WiFi.status() == WL_CONNECTED) {
    static uint32_t noIpSince = 0;
    if (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {  // linked but no address: time and weather cannot work; join again
      if (!noIpSince) noIpSince = millis();
      if (millis() - noIpSince > 20000 && ssid.length()) {
        noIpSince = 0;
        Serial.println("WIFI no IP address for 20 s: reconnecting");
        WiFi.disconnect();
        WiFi.begin(ssid.c_str(), pass.c_str());
        connectStart = millis();
      }
    } else {
      noIpSince = 0;
    }
    snprintf(statusBuf, sizeof(statusBuf), "%s  %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    if (!ntpStarted) startNtp();
  } else if (attempting) {
    snprintf(statusBuf, sizeof(statusBuf), tr("Connecting to %s..."), newSsid.c_str());
  } else if (portalOn) {
    snprintf(statusBuf, sizeof(statusBuf), tr("Join Wi-Fi %s, open 192.168.4.1"), AP_NAME);
  } else {
    snprintf(statusBuf, sizeof(statusBuf), tr("Connecting to %s..."), ssid.c_str());
    if (millis() - connectStart > CONNECT_TIMEOUT_MS) startPortal();
  }
}

// ---- scan ----
// Arduino's async scan gives up after max_ms_per_chan * 20 (6 s), but while connected a full scan takes a bit
// longer (the radio keeps returning to the home channel), so it always reported failure. Drive the IDF scan
// directly and wait for its done event instead.
static volatile bool scanDoneFlag = false;
static int scanState = -2;
static uint32_t scanAt = 0;
static wifi_ap_record_t *scanRecs = nullptr;
static uint16_t scanCount = 0;

bool wifiScanStart() {
  static bool hooked = false;
  if (!hooked) {
    hooked = true;
    WiFi.onEvent([](arduino_event_id_t, arduino_event_info_t) { scanDoneFlag = true; }, ARDUINO_EVENT_WIFI_SCAN_DONE);
  }
  if (scanState == -1) return true;
  wifi_scan_config_t c = {};
  c.scan_type = WIFI_SCAN_TYPE_ACTIVE;
  c.scan_time.active.min = 60;
  c.scan_time.active.max = 150;
  scanDoneFlag = false;
  if (esp_wifi_scan_start(&c, false) != ESP_OK) {
    scanState = -2;
    return false;
  }
  scanState = -1;
  scanAt = millis();
  return true;
}

int wifiScanResult() {
  if (scanState != -1) return scanState;
  if (scanDoneFlag) {
    free(scanRecs);
    scanRecs = nullptr;
    scanCount = 0;
    uint16_t n = 0;
    esp_wifi_scan_get_ap_num(&n);
    if (n) {
      scanRecs = (wifi_ap_record_t *)calloc(n, sizeof(wifi_ap_record_t));
      if (scanRecs && esp_wifi_scan_get_ap_records(&n, scanRecs) == ESP_OK) scanCount = n;
    }
    scanState = scanCount;
  } else if (millis() - scanAt > 15000) {
    esp_wifi_scan_stop();
    scanState = -2;
  }
  return scanState;
}

const wifi_ap_record_t *wifiScanAp(int i) { return (i >= 0 && i < scanCount) ? &scanRecs[i] : nullptr; }

void wifiSetTimezone(int hours) {
  tzHours = hours;
  Preferences ui;
  ui.begin("ui", false);
  ui.putInt("tz", hours);
  ui.end();
  if (ntpStarted) startNtp();
}

int wifiTimezone() { return tzHours; }
const char *wifiStatus() { return statusBuf; }
bool wifiConnected() { return WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0); }  // linked AND has an address
bool wifiPortalActive() { return portalOn; }
