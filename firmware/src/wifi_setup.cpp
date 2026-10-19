// Wi-Fi: saved-credentials connect, "Zin100-Setup" captive portal, in-app connect, NTP time zone.
#include "wifi_setup.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <time.h>

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

  int n = WiFi.scanNetworks();
  scanOptions = "";
  for (int i = 0; i < n; i++) scanOptions += "<option value=\"" + WiFi.SSID(i) + "\">";

  auto page = []() {
    String h = "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
               "<style>body{font-family:sans-serif;background:#0b1020;color:#eef;padding:24px;max-width:420px;margin:auto}"
               "input,button{width:100%;padding:12px;margin:8px 0;border-radius:10px;border:0;font-size:16px;box-sizing:border-box}"
               "button{background:#38e1ff;font-weight:bold}</style><h2>Zin100 Wi-Fi</h2>"
               "<form method=POST action=/save><input name=s list=n placeholder='Wi-Fi name' required>"
               "<datalist id=n>" + scanOptions + "</datalist>"
               "<input name=p type=password placeholder='Password'><button>Save &amp; connect</button></form>";
    server.send(200, "text/html", h);
  };
  server.on("/", page);
  server.on("/save", HTTP_POST, []() {
    prefs.putString("ssid", server.arg("s"));
    prefs.putString("pass", server.arg("p"));
    server.send(200, "text/html", "<meta name=viewport content='width=device-width'><h3>Saved. Restarting...</h3>");
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
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  if (ssid.length() == 0) {
    startPortal();
  } else {
    WiFi.mode(WIFI_STA);
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

  if (WiFi.status() == WL_CONNECTED) {
    snprintf(statusBuf, sizeof(statusBuf), "%s  %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    if (!ntpStarted) startNtp();
  } else if (attempting) {
    snprintf(statusBuf, sizeof(statusBuf), "Connecting to %s...", newSsid.c_str());
  } else if (portalOn) {
    snprintf(statusBuf, sizeof(statusBuf), "Join Wi-Fi %s, open 192.168.4.1", AP_NAME);
  } else {
    snprintf(statusBuf, sizeof(statusBuf), "Connecting to %s...", ssid.c_str());
    if (millis() - connectStart > CONNECT_TIMEOUT_MS) startPortal();
  }
}

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
bool wifiConnected() { return WiFi.status() == WL_CONNECTED; }
bool wifiPortalActive() { return portalOn; }
