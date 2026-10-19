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
static bool haveLocation = false;

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
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(10000);
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=") + String(lat, 3) +
               "&longitude=" + String(lon, 3) +
               "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,is_day&timezone=auto";
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
  }
  p.end();

  for (;;) {
    while (!wifiConnected()) vTaskDelay(pdMS_TO_TICKS(1000));
    vTaskDelay(pdMS_TO_TICKS(4000));  // DNS is not ready right after the link comes up

    bool ok;
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

void weatherBegin() { xTaskCreatePinnedToCore(task, "weather", 12288, nullptr, 1, nullptr, 0); }

WeatherData weatherGet() {
  WeatherData w;
  portENTER_CRITICAL(&dataLock);
  w = data;
  portEXIT_CRITICAL(&dataLock);
  return w;
}

void weatherRefresh() { refreshRequested = true; }

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
