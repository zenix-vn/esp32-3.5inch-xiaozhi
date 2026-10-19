#pragma once
#include <Arduino.h>

enum XzState : uint8_t {
  XZ_WAIT_WIFI,   // no network yet
  XZ_REQUESTING,  // contacting the OTA/activation server
  XZ_CODE,        // 6-digit code ready: show it, user enters it at xiaozhi.me
  XZ_ACTIVATED,   // device is bound to an account
  XZ_ERROR        // last request failed, retrying
};

void xiaozhiBegin();
XzState xiaozhiState();
const char *xiaozhiCode();     // valid in XZ_CODE
const char *xiaozhiMessage();  // human readable status / server message
