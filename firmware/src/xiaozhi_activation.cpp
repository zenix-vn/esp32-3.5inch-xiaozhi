// Xiaozhi device activation, following libs/xiaozhi-esp32/main/ota.cc:
//   POST <OTA_URL>           -> JSON with activation.code (6 digits) while the device is unbound
//   POST <OTA_URL>/activate  -> 202 while waiting for the user to enter the code, 200 once bound
#include "xiaozhi_activation.h"
#include "i18n.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <esp_random.h>
#include "wifi_setup.h"
#include "net_lock.h"

static const char *OTA_URL = "https://api.tenclass.net/xiaozhi/ota/";
static const char *APP_VERSION = "1.0.0";

static volatile XzState state = XZ_WAIT_WIFI;
static char code[16] = "";
static char message[96] = "Waiting for Wi-Fi";
static String uuid;

static String makeUuid() {
  uint8_t u[16];
  esp_fill_random(u, sizeof(u));
  u[6] = (u[6] & 0x0F) | 0x40;
  u[8] = (u[8] & 0x3F) | 0x80;
  char s[37];
  snprintf(s, sizeof(s), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", u[0], u[1],
           u[2], u[3], u[4], u[5], u[6], u[7], u[8], u[9], u[10], u[11], u[12], u[13], u[14], u[15]);
  return String(s);
}

static String macAddress() {
  String m = WiFi.macAddress();
  m.toLowerCase();
  return m;
}

static String systemInfoJson() {
  JsonDocument d;
  d["version"] = 2;
  d["language"] = "en-US";
  d["flash_size"] = ESP.getFlashChipSize();
  d["minimum_free_heap_size"] = String(ESP.getMinFreeHeap());
  d["mac_address"] = macAddress();
  d["uuid"] = uuid;
  d["chip_model_name"] = "esp32";
  JsonObject ci = d["chip_info"].to<JsonObject>();
  ci["model"] = 1;
  ci["cores"] = 2;
  ci["revision"] = ESP.getChipRevision();
  ci["features"] = 0;
  JsonObject app = d["application"].to<JsonObject>();
  app["name"] = "zin100";
  app["version"] = APP_VERSION;
  app["compile_time"] = String(__DATE__) + "T" + __TIME__ + "Z";
  app["idf_version"] = ESP.getSdkVersion();
  JsonObject disp = d["display"].to<JsonObject>();
  disp["monochrome"] = false;
  disp["width"] = 480;
  disp["height"] = 320;
  JsonObject board = d["board"].to<JsonObject>();
  board["type"] = "zin100-esp32-2432s035";
  board["name"] = "Zin100";
  board["ssid"] = WiFi.SSID();
  board["rssi"] = WiFi.RSSI();
  board["ip"] = WiFi.localIP().toString();
  board["mac"] = macAddress();
  String out;
  serializeJson(d, out);
  return out;
}

// POST with the headers firmware sends; returns HTTP status (or <0 on transport error).
static int postLocked(const String &url, const String &body, String &resp) {
  WiFiClientSecure client;
  client.setInsecure();  // TODO: pin the server CA instead of skipping verification
  HTTPClient http;
  http.setTimeout(10000);
  if (!http.begin(client, url)) return -1;
  http.addHeader("Activation-Version", "1");
  http.addHeader("Device-Id", macAddress());
  http.addHeader("Client-Id", uuid);
  http.addHeader("User-Agent", String("zin100/") + APP_VERSION);
  http.addHeader("Accept-Language", "en-US");
  http.addHeader("Content-Type", "application/json");
  IPAddress ip;
  Serial.printf("XZ heap=%u maxblock=%u dns=%d\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap(),
                WiFi.hostByName("api.tenclass.net", ip));
  int status = http.POST(body);
  Serial.printf("XZ stack left=%u\n", (unsigned)uxTaskGetStackHighWaterMark(nullptr));  // bytes never used
  if (status > 0) {
    resp = http.getString();
  } else {
    char err[100] = "";
    client.lastError(err, sizeof(err));
    Serial.printf("XZ http err=%d (%s) tls=%s\n", status, http.errorToString(status).c_str(), err);
  }
  http.end();
  return status;
}

static int post(const String &url, const String &body, String &resp) {
  xSemaphoreTake(netMutex(), portMAX_DELAY);
  int st = postLocked(url, body, resp);
  xSemaphoreGive(netMutex());
  return st;
}

static void setMsg(const char *m) { strlcpy(message, m, sizeof(message)); }

static void task(void *) {
  for (;;) {
    while (!wifiConnected()) {
      state = XZ_WAIT_WIFI;
      setMsg("Waiting for Wi-Fi");
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    state = XZ_REQUESTING;
    setMsg("Contacting Xiaozhi...");
    String resp;
    int status = post(OTA_URL, systemInfoJson(), resp);
    if (status != 200) {
      state = XZ_ERROR;
      snprintf(message, sizeof(message), tr("Server error %d, retrying"), status);
      Serial.printf("XZ ota status=%d\n", status);
      vTaskDelay(pdMS_TO_TICKS(10000));
      continue;
    }
    Serial.printf("XZ ota reply: %s\n", resp.c_str());

    JsonDocument doc;
    if (deserializeJson(doc, resp)) {
      state = XZ_ERROR;
      setMsg("Bad server reply, retrying");
      vTaskDelay(pdMS_TO_TICKS(10000));
      continue;
    }

    // save connection settings for the voice client
    Preferences p;
    p.begin("xiaozhi", false);
    if (doc["websocket"]["url"].is<const char *>()) {
      p.putString("ws_url", doc["websocket"]["url"].as<String>());
      p.putString("ws_token", doc["websocket"]["token"] | "");
    }
    p.end();

    const char *c = doc["activation"]["code"];
    if (!c) {  // no activation block: already bound to an account
      state = XZ_ACTIVATED;
      setMsg("Activated");
      Serial.println("XZ activated");
      vTaskDelete(nullptr);
    }
    strlcpy(code, c, sizeof(code));
    setMsg(doc["activation"]["message"] | "Enter the code at xiaozhi.me");
    state = XZ_CODE;
    Serial.printf("XZ activation code: %s\n", code);

    // wait for the user to enter the code (202 = pending, 200 = done), then re-check
    String url = String(OTA_URL);
    url += url.endsWith("/") ? "activate" : "/activate";
    bool done = false;
    for (int i = 0; i < 40 && !done; i++) {
      String r;
      int st = post(url, "{}", r);
      Serial.printf("XZ activate try %d status=%d\n", i + 1, st);
      if (st == 200) {
        done = true;
      } else {
        vTaskDelay(pdMS_TO_TICKS(st == 202 ? 3000 : 10000));
      }
    }
    if (done) {
      state = XZ_ACTIVATED;
      setMsg("Activated");
      Serial.println("XZ activated");
      vTaskDelete(nullptr);
    }
    // code expired: loop to request a fresh one
  }
}

void xiaozhiBegin() {
  Preferences p;
  p.begin("board", false);
  uuid = p.getString("uuid", "");
  if (uuid.length() == 0) {
    uuid = makeUuid();
    p.putString("uuid", uuid);
  }
  p.end();
  // 16 KB was generous for TLS + HTTP + JSON; 12 KB still leaves a margin ("XZ stack left" on the log)
  xTaskCreatePinnedToCore(task, "xz_act", 12288, nullptr, 1, nullptr, 0);
}

XzState xiaozhiState() { return state; }
const char *xiaozhiCode() { return code; }
const char *xiaozhiMessage() { return message; }
