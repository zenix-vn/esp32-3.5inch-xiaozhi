#pragma once
// Power saving. While nobody looks at the screen (backlight off, or the ambient clock) and nothing streams (radio,
// microphone), Wi-Fi goes into modem sleep and loop() sleeps longer between passes; the home screen also stops
// redrawing while the backlight is off (home.h). Everything returns to normal as the screen wakes.
// Modem sleep once made the router drop the DHCP lease: it is only used while idle, and wifiLoop() rejoins after 20 s
// without an address. The CPU stays at 240 MHz: setCpuFrequencyMhz() at run time hung this firmware.
// Included by main.cpp after radio.h, mic.h and ambient.h.
#include <esp_sleep.h>
#include <driver/gpio.h>
#ifndef LED_PIN_R  // led.h comes later; same defaults
#define LED_PIN_R 4
#endif
#ifndef LED_PIN_G
#define LED_PIN_G 16
#endif
#ifndef LED_PIN_B
#define LED_PIN_B 17
#endif
#ifndef LED_ACTIVE_LOW
#define LED_ACTIVE_LOW 1
#endif

static bool pwrLow = false;

static void powerTick() {  // every loop pass: only a few flags are read
  bool want = (screenOff || ambOn) && !radioTaskH && !micUp;
  if (want == pwrLow) return;
  pwrLow = want;
  WiFi.setSleep(want);  // true = WIFI_PS_MIN_MODEM
  Serial.printf("POWER %s\n", want ? "low (modem sleep)" : "full");
}

// Shut down (control center, Settings > System): the closest this board gets to off. The LiPo stays wired to the
// 3.3 V regulator, so the ESP32 goes into deep sleep with the panel, backlight and LED dark; a touch on the screen
// (XPT2046 PENIRQ on IO36, LOW while pressed) boots it again from the start. The pin holds set here are released
// at the top of setup().
static void doPowerOff() {
  Serial.println("POWER off (deep sleep, touch to wake)");
  soundStop();
  radioStop();
  micStop();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  tftIdle();
  tft.writecommand(0x28);  // ST7796: display off
  tft.writecommand(0x10);  // sleep in
  const int pins[4] = {PIN_BL, LED_PIN_R, LED_PIN_G, LED_PIN_B};
  for (int i = 0; i < 4; i++) {  // backlight low; LED dark (HIGH on the common-anode board)
    ledcDetachPin(pins[i]);
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], i && LED_ACTIVE_LOW ? HIGH : LOW);
    gpio_hold_en((gpio_num_t)pins[i]);
  }
  gpio_deep_sleep_hold_en();
  uint32_t t0 = millis();
  while (digitalRead(XPT_IRQ) == LOW && millis() - t0 < 3000) delay(10);  // the finger on "Confirm" must lift first
  delay(150);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)XPT_IRQ, 0);
  esp_deep_sleep_start();
}
static void askPowerOff(lv_event_t *) {
  confirmDialog("Shut down?", "The screen goes dark. Touch the screen to turn the device back on.", doPowerOff);
}
