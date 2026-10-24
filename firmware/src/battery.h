#pragma once
// Battery gauge for the status bar. The ESP32-2432S035 charges a LiPo on its BAT socket (TP4054) but does not
// wire the battery voltage to the ESP32, so it needs two small mods:
//   - BAT+ -> 100k -> BAT_ADC_PIN -> 100k -> GND   (divider halves the voltage: 4.2 V reads 2.1 V)
//   - optional: USB 5V -> 100k -> BAT_USB_PIN -> 100k -> GND   (HIGH while a charger is plugged in)
// Without BAT_USB_PIN the charger is guessed from the voltage: plugging in lifts the battery voltage by the
// charge current at once, unplugging drops it. That guess misses a plug-in on a full battery.
// Pins are build flags (platformio.ini: -DBAT_ADC_PIN=34 -DBAT_USB_PIN=32); -1 turns a pin off.
// Included by main.cpp before statusbar.h, which shows the level and reacts to the charger events.

#ifndef BAT_ADC_PIN
#define BAT_ADC_PIN 39  // ADC1 (works while Wi-Fi is on), input only, unused on this board
#endif
#ifndef BAT_USB_PIN
#define BAT_USB_PIN -1
#endif
#ifndef BAT_DIVIDER
#define BAT_DIVIDER 2.0f  // battery voltage / pin voltage
#endif

enum BatEvent { BAT_EV_NONE, BAT_EV_PLUG, BAT_EV_UNPLUG, BAT_EV_FULL };

static bool batPresent = false;  // a steady reading between 3.0 and 4.5 V
static bool batUsb = false;      // charger plugged in
static bool batFull = false;     // plugged in and the charger has stopped
static int batPct = -1;          // 0..100 shown, -1 = unknown
static int batMv = 0;            // smoothed battery voltage

// LiPo resting voltage -> charge (one cell, light load)
static int batMvToPct(int mv) {
  static const int16_t T[][2] = {{4180, 100}, {4100, 92}, {4020, 84}, {3960, 76}, {3910, 68}, {3870, 60}, {3840, 52},
                                 {3810, 44}, {3790, 36}, {3770, 28}, {3740, 20}, {3700, 12}, {3640, 6},  {3500, 0}};
  const int n = sizeof(T) / sizeof(T[0]);
  if (mv >= T[0][0]) return 100;
  for (int i = 1; i < n; i++)
    if (mv >= T[i][0]) return T[i][1] + (mv - T[i][0]) * (T[i - 1][1] - T[i][1]) / (T[i - 1][0] - T[i][0]);
  return 0;
}

static bool batNoisy = false;  // last read spread > 150 mV: nothing wired to the pin (floating input)

static int batReadMv() {
  uint32_t s = 0, lo = 9999, hi = 0;
  for (int i = 0; i < 16; i++) {
    uint32_t v = analogReadMilliVolts(BAT_ADC_PIN);  // factory-calibrated
    s += v;
    lo = v < lo ? v : lo;
    hi = v > hi ? v : hi;
  }
  batNoisy = (hi - lo) * BAT_DIVIDER > 150;
  return (int)(s / 16 * BAT_DIVIDER);
}

static void batBegin() {
  if (BAT_ADC_PIN >= 0) {
    analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);  // up to ~3.1 V at the pin
    batMv = batReadMv();
  }
  if (BAT_USB_PIN >= 0) {
    pinMode(BAT_USB_PIN, INPUT);
    batUsb = digitalRead(BAT_USB_PIN) == HIGH;
  }
  batPresent = !batNoisy && batMv > 3000 && batMv < 4500;
}

// Once a second (statusbar.h). Returns what changed with the charger.
static BatEvent batTick() {
  if (BAT_ADC_PIN < 0) return BAT_EV_NONE;
  static int base = 0;       // slow average: what the voltage "should" be without a charger change
  static uint32_t fullMs = 0;
  int mv = batReadMv();
  batMv = (batMv * 3 + mv) / 4;  // ~4 s
  if (!base) base = batMv;
  bool was = batUsb;
  if (BAT_USB_PIN >= 0) {
    batUsb = digitalRead(BAT_USB_PIN) == HIGH;
  } else if (batMv - base > 60) {
    batUsb = true;
    base = batMv;
  } else if (batMv - base < -60) {
    batUsb = false;
    base = batMv;
  }
  base += (batMv - base) / 16;  // follows the slow charge and discharge curve

  static uint8_t quiet = 0;  // seconds in a row with a steady reading
  quiet = batNoisy ? 0 : quiet < 10 ? quiet + 1 : 10;
  bool present = quiet >= 3 && batMv > 3000 && batMv < 4500;
  if (present != batPresent) {
    batPresent = present;
    batPct = -1;
  }
  if (!batPresent) return BAT_EV_NONE;

  // under charge current the voltage sits ~80 mV above resting; the TP4054 holds 4.2 V once nearly full
  int pct = batMvToPct(batUsb ? batMv - 80 : batMv);
  if (batPct < 0 || abs(pct - batPct) > 8) batPct = pct;
  else if (batUsb ? pct > batPct : pct < batPct) batPct = pct;  // no flicker: only up on charge, down off it

  BatEvent ev = BAT_EV_NONE;
  if (batUsb != was) {
    ev = batUsb ? BAT_EV_PLUG : BAT_EV_UNPLUG;
    batFull = false;
    fullMs = 0;
  } else if (batUsb && !batFull) {
    if (batMv >= 4150) {  // at the 4.2 V ceiling for 10 minutes: the charger has tapered off
      if (!fullMs) fullMs = millis();
      if (millis() - fullMs > 600000) {
        batFull = true;
        batPct = 100;
        ev = BAT_EV_FULL;
      }
    } else {
      fullMs = 0;
    }
  }
  return ev;
}
