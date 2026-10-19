#pragma once

void wifiBegin();
void wifiLoop();
const char *wifiStatus();
bool wifiConnected();
bool wifiPortalActive();

// Connect from the UI without restarting. Result: 0 = in progress, 1 = connected (saved), -1 = failed.
void wifiConnect(const char *ssid, const char *password);
int wifiConnectResult();

// Time zone offset in whole hours (saved, applied to NTP).
void wifiSetTimezone(int hours);
int wifiTimezone();
