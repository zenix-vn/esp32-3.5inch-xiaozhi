// Current weather from Open-Meteo (no API key). Location comes from IP geolocation (ip-api.com),
// cached in flash; falls back to Hanoi if it cannot be determined.
#include "weather.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "net_lock.h"
#include "wifi_setup.h"

static WeatherData data;
static portMUX_TYPE dataLock = portMUX_INITIALIZER_UNLOCKED;
static volatile bool refreshRequested = false;
static float lat = 21.0285f, lon = 105.8542f;
static char city[40] = "Hanoi";
static bool haveLocation = false, manualLoc = false;

// Location change requested from the web page; applied by the task so lat/lon/city never tear.
static portMUX_TYPE pendLock = portMUX_INITIALIZER_UNLOCKED;
static bool pendSet = false, pendAuto = false;
static float pendLat = 0, pendLon = 0;
static char pendCity[40] = "";

static void applyPending() {
  bool set, autoLoc;
  float la, lo;
  char c[40];
  portENTER_CRITICAL(&pendLock);
  set = pendSet;
  autoLoc = pendAuto;
  la = pendLat;
  lo = pendLon;
  strlcpy(c, pendCity, sizeof(c));
  pendSet = pendAuto = false;
  portEXIT_CRITICAL(&pendLock);
  if (!set && !autoLoc) return;
  Preferences p;
  p.begin("weather", false);
  if (set) {
    lat = la;
    lon = lo;
    strlcpy(city, c, sizeof(city));
    manualLoc = haveLocation = true;
    p.putFloat("lat", lat);
    p.putFloat("lon", lon);
    p.putString("city", city);
    p.putBool("manual", true);
  } else {
    manualLoc = haveLocation = false;
    p.clear();
  }
  p.end();
}

static bool locate() {
  HTTPClient http;
  WiFiClient client;
  http.setTimeout(8000);
  if (!http.begin(client, "http://ip-api.com/json/?fields=status,city,lat,lon")) return false;
  int st = http.GET();
  if (st != 200) {
    http.end();
    return false;
  }
  JsonDocument d;
  DeserializationError err = deserializeJson(d, http.getString());
  http.end();
  if (err || strcmp(d["status"] | "", "success") != 0) return false;
  lat = d["lat"];
  lon = d["lon"];
  strlcpy(city, d["city"] | "", sizeof(city));
  Preferences p;
  p.begin("weather", false);
  p.putFloat("lat", lat);
  p.putFloat("lon", lon);
  p.putString("city", city);
  p.end();
  Serial.printf("WX location %s (%.3f, %.3f)\n", city, lat, lon);
  return true;
}

static bool fetch() {
  WiFiClient client;  // plain HTTP: no TLS, which needs ~40 KB of contiguous RAM
  HTTPClient http;
  http.setTimeout(10000);
  String url = String("http://api.open-meteo.com/v1/forecast?latitude=") + String(lat, 3) +
               "&longitude=" + String(lon, 3) +
               "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,is_day"
               "&hourly=temperature_2m,weather_code,precipitation_probability&forecast_hours=12"
               "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max&forecast_days=5"
               "&timezone=auto";
  if (!http.begin(client, url)) return false;
  int st = http.GET();
  if (st != 200) {
    Serial.printf("WX http %d\n", st);
    http.end();
    return false;
  }
  JsonDocument d;
  DeserializationError err = deserializeJson(d, http.getString());
  http.end();
  if (err || !d["current"]["temperature_2m"].is<float>()) return false;
  WeatherData w;
  w.valid = true;
  w.temp = d["current"]["temperature_2m"];
  w.humidity = d["current"]["relative_humidity_2m"] | 0;
  w.code = d["current"]["weather_code"] | 0;
  w.wind = d["current"]["wind_speed_10m"] | 0.0f;
  w.isDay = (d["current"]["is_day"] | 1) != 0;
  strlcpy(w.city, city, sizeof(w.city));
  JsonArray ht = d["hourly"]["time"];
  for (size_t i = 0; i < ht.size() && w.nHours < 12; i++) {
    const char *s = ht[i] | "";
    if (strlen(s) < 13) continue;
    WxHour &h = w.hours[w.nHours++];
    h.hour = atoi(s + 11);
    h.temp = d["hourly"]["temperature_2m"][i] | 0.0f;
    h.code = d["hourly"]["weather_code"][i] | 0;
    h.pop = d["hourly"]["precipitation_probability"][i] | 0;
  }
  JsonArray dt = d["daily"]["time"];
  for (size_t i = 0; i < dt.size() && w.nDays < 5; i++) {
    int y = 0, m = 0, dd = 0;
    if (sscanf(dt[i] | "", "%d-%d-%d", &y, &m, &dd) != 3) continue;
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};  // Sakamoto's weekday
    int yy = m < 3 ? y - 1 : y;
    WxDay &x = w.days[w.nDays++];
    x.wday = (yy + yy / 4 - yy / 100 + yy / 400 + t[m - 1] + dd) % 7;
    x.mday = dd;
    x.mon = m;
    x.code = d["daily"]["weather_code"][i] | 0;
    x.tmax = d["daily"]["temperature_2m_max"][i] | 0.0f;
    x.tmin = d["daily"]["temperature_2m_min"][i] | 0.0f;
    x.pop = d["daily"]["precipitation_probability_max"][i] | 0;
  }
  portENTER_CRITICAL(&dataLock);
  data = w;
  portEXIT_CRITICAL(&dataLock);
  Serial.printf("WX %s %.1fC code=%d hum=%d\n", w.city, w.temp, w.code, w.humidity);
  return true;
}

static void task(void *) {
  Preferences p;
  p.begin("weather", true);
  if (p.isKey("lat")) {
    lat = p.getFloat("lat", lat);
    lon = p.getFloat("lon", lon);
    p.getString("city", city, sizeof(city));
    haveLocation = true;
    manualLoc = p.getBool("manual", false);
  }
  p.end();

  for (;;) {
    while (!wifiConnected()) vTaskDelay(pdMS_TO_TICKS(1000));
    vTaskDelay(pdMS_TO_TICKS(4000));  // DNS is not ready right after the link comes up

    bool ok;
    applyPending();
    xSemaphoreTake(netMutex(), portMAX_DELAY);
    if (!haveLocation) haveLocation = locate();
    ok = fetch();
    xSemaphoreGive(netMutex());

    if (!ok) {
      portENTER_CRITICAL(&dataLock);
      data.failed = true;
      portEXIT_CRITICAL(&dataLock);
    } else {
      portENTER_CRITICAL(&dataLock);
      data.failed = false;
      portEXIT_CRITICAL(&dataLock);
    }
    refreshRequested = false;
    int wait = ok ? 15 * 60 : 60;  // seconds
    for (int i = 0; i < wait && !refreshRequested; i++) vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void weatherBegin() { xTaskCreatePinnedToCore(task, "weather", 6144, nullptr, 1, nullptr, 0); }  // plain HTTP: small stack

WeatherData weatherGet() {
  WeatherData w;
  portENTER_CRITICAL(&dataLock);
  w = data;
  portEXIT_CRITICAL(&dataLock);
  return w;
}

void weatherRefresh() { refreshRequested = true; }

void weatherSetLocation(float la, float lo, const char *c) {
  portENTER_CRITICAL(&pendLock);
  pendLat = la;
  pendLon = lo;
  strlcpy(pendCity, c, sizeof(pendCity));
  pendSet = true;
  pendAuto = false;
  portEXIT_CRITICAL(&pendLock);
  refreshRequested = true;
}

void weatherAutoLocate() {
  portENTER_CRITICAL(&pendLock);
  pendAuto = true;
  pendSet = false;
  portEXIT_CRITICAL(&pendLock);
  refreshRequested = true;
}

bool weatherIsManual() { return manualLoc; }

void weatherGetLocation(float &la, float &lo, char *c, size_t n) {
  la = lat;
  lo = lon;
  strlcpy(c, city, n);
}

const char *weatherText(int c) {
  if (c == 0) return "Clear";
  if (c == 1) return "Mostly clear";
  if (c == 2) return "Partly cloudy";
  if (c == 3) return "Overcast";
  if (c == 45 || c == 48) return "Fog";
  if (c >= 51 && c <= 57) return "Drizzle";
  if (c >= 61 && c <= 67) return "Rain";
  if (c >= 71 && c <= 77) return "Snow";
  if (c >= 80 && c <= 82) return "Showers";
  if (c == 85 || c == 86) return "Snow showers";
  if (c >= 95) return "Thunderstorm";
  return "Cloudy";
}
