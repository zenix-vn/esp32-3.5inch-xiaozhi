#pragma once
#include <esp_wifi_types.h>

void wifiBegin();
void wifiLoop();
const char *wifiStatus();
bool wifiConnected();
bool wifiPortalActive();

// Connect from the UI without restarting. Result: 0 = in progress, 1 = connected (saved), -1 = failed.
void wifiConnect(const char *ssid, const char *password);
int wifiConnectResult();

// Network scan for the Wi-Fi page (non-blocking). wifiScanResult(): -1 running, -2 failed, >= 0 networks found.
bool wifiScanStart();
int wifiScanResult();
const wifi_ap_record_t *wifiScanAp(int i);

// Time zone offset in whole hours (saved, applied to NTP).
void wifiSetTimezone(int hours);
int wifiTimezone();
