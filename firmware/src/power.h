#pragma once
// Power saving. While nobody looks at the screen (backlight off, or the ambient clock) and nothing streams (radio,
// microphone), Wi-Fi goes into modem sleep and loop() sleeps longer between passes; the home screen also stops
// redrawing while the backlight is off (home.h). Everything returns to normal as the screen wakes.
// Modem sleep once made the router drop the DHCP lease: it is only used while idle, and wifiLoop() rejoins after 20 s
// without an address. The CPU stays at 240 MHz: setCpuFrequencyMhz() at run time hung this firmware.
// Included by main.cpp after radio.h, mic.h and ambient.h.

static bool pwrLow = false;

static void powerTick() {  // every loop pass: only a few flags are read
  bool want = (screenOff || ambOn) && !radioTaskH && !micUp;
  if (want == pwrLow) return;
  pwrLow = want;
  WiFi.setSleep(want);  // true = WIFI_PS_MIN_MODEM
  Serial.printf("POWER %s\n", want ? "low (modem sleep)" : "full");
}
