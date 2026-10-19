#pragma once
// One TLS/HTTP request at a time: the ESP32 has too little heap for two concurrent TLS sessions.
#include <Arduino.h>
#include <freertos/semphr.h>

inline SemaphoreHandle_t netMutex() {
  static SemaphoreHandle_t m = xSemaphoreCreateMutex();
  return m;
}
