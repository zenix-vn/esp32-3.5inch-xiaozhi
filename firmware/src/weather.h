#pragma once
#include <Arduino.h>

struct WxHour {
  int8_t hour, pop;  // local hour, precipitation probability %
  int16_t code;
  float temp;
};
struct WxDay {
  int8_t wday, mday, mon, pop;
  int16_t code;
  float tmax, tmin;
};

struct WeatherData {
  bool valid = false;   // at least one successful fetch
  bool failed = false;  // last attempt failed
  float temp = 0;
  int humidity = 0;
  int code = 0;  // WMO weather code
  float wind = 0;
  bool isDay = true;
  char city[40] = "";
  WxHour hours[12];  // forecast from the current hour
  uint8_t nHours = 0;
  WxDay days[5];  // today + 4
  uint8_t nDays = 0;
};

void weatherBegin();
WeatherData weatherGet();
void weatherRefresh();  // fetch again soon (e.g. user tapped the card)
const char *weatherText(int wmoCode);

// Location: automatic (IP geolocation, the default) or set by hand from the web page.
void weatherSetLocation(float lat, float lon, const char *city);  // applied by the weather task, then refreshes
void weatherAutoLocate();                                         // forget the manual location
bool weatherIsManual();
void weatherGetLocation(float &lat, float &lon, char *city, size_t cityLen);
