#pragma once
#include <Arduino.h>

struct WeatherData {
  bool valid = false;   // at least one successful fetch
  bool failed = false;  // last attempt failed
  float temp = 0;
  int humidity = 0;
  int code = 0;  // WMO weather code
  float wind = 0;
  bool isDay = true;
  char city[40] = "";
};

void weatherBegin();
WeatherData weatherGet();
void weatherRefresh();  // fetch again soon (e.g. user tapped the card)
const char *weatherText(int wmoCode);
