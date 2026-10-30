#pragma once
// XPT2046 resistive touch (bit-banged on its own pins) with a 4-corner calibration wizard.
#include <Arduino.h>
#include <Preferences.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "i18n.h"
#include "vi_fonts.h"

extern TFT_eSPI tft;
static void tftIdle();  // main.cpp: wait for the display's DMA and release the shared SPI bus
static const int XPT_IRQ = 36;  // touch shares the display SPI bus; CS is TOUCH_CS (33)

struct TouchCal {
  int x0 = 200, x1 = 3700, y0 = 240, y1 = 3800;  // raw values at screen edges
  bool swap = false;                              // raw X is the vertical axis
};
static TouchCal cal;

static void xptInit() { pinMode(XPT_IRQ, INPUT); }

// Returns true while pressed; raw 12-bit values in rx/ry (read through TFT_eSPI on the shared bus).
static bool xptSample(int &rx, int &ry) {
  if (digitalRead(XPT_IRQ) != LOW) return false;
  tftIdle();
  uint16_t x = 0, y = 0;
  if (tft.getTouchRawZ() < 80) return false;
  tft.getTouchRaw(&x, &y);
  rx = x;
  ry = y;
  return rx > 100 && ry > 100 && rx < 4090 && ry < 4090;
}

// Resistive panels report noisy coordinates when the finger lands; LVGL would read that jump as a swipe
// and drop the tap. So: wait 40 ms for the contact to settle, smooth the position, and tolerate
// 30 ms of dropouts mid-press.
// Synthetic touch from the web dev tools: press at (x1,y1), slide to (x2,y2) over `dur` ms, then release.
static uint32_t fakeStart = 0, fakeDur = 0;
static int fakeX1, fakeY1, fakeX2, fakeY2;
static void touchInject(int x1, int y1, int x2, int y2, uint32_t dur) {
  fakeX1 = x1;
  fakeY1 = y1;
  fakeX2 = x2;
  fakeY2 = y2;
  fakeDur = dur < 60 ? 60 : dur;
  fakeStart = millis();
}

// While the screen is off the first touch only wakes it: it is swallowed until the finger lifts.
static volatile bool gTouchSwallow = false, gTouchWake = false;
static volatile uint32_t gTouchPresses = 0;  // counts presses (led.h blinks on each)

static void touchRead(lv_indev_drv_t *, lv_indev_data_t *data) {
  static int16_t lx = 0, ly = 0;          // smoothed position
  static int16_t ax = 0, ay = 0;          // anchor: where the press is reported until the finger really moves
  static uint32_t downAt = 0, lastSeen = 0;
  static bool down = false, reporting = false;
  int rx, ry;
  if (gTouchSwallow) {
    static uint32_t lastPress = 0;
    if (xptSample(rx, ry)) {
      gTouchWake = true;
      lastPress = millis();
    } else if (lastPress && millis() - lastPress > 150) {
      gTouchSwallow = false;
      lastPress = 0;
    }
    data->state = LV_INDEV_STATE_REL;
    return;
  }
  if (fakeDur) {
    uint32_t el = millis() - fakeStart;
    float t = el >= fakeDur ? 1.0f : (float)el / fakeDur;
    data->point.x = fakeX1 + (int)((fakeX2 - fakeX1) * t);
    data->point.y = fakeY1 + (int)((fakeY2 - fakeY1) * t);
    data->state = el >= fakeDur ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;
    if (el >= fakeDur) fakeDur = 0;
    return;
  }
  if (xptSample(rx, ry)) {
    int h = cal.swap ? ry : rx, v = cal.swap ? rx : ry;
    int nx = constrain(map(h, cal.x0, cal.x1, 0, 479), 0, 479);
    int ny = constrain(map(v, cal.y0, cal.y1, 0, 319), 0, 319);
    lastSeen = millis();
    if (!down) {
      down = true;
      gTouchPresses++;
      reporting = false;
      downAt = lastSeen;
    }
    if (!reporting) {
      lx = nx;  // settle window: follow the latest sample, nothing is reported yet
      ly = ny;
      if (lastSeen - downAt >= 25) {  // skip only the first (noisiest) sample
        reporting = true;
        ax = lx;
        ay = ly;
      }
    } else {
      lx = (lx + nx) / 2;
      ly = (ly + ny) / 2;
    }
  } else if (down && millis() - lastSeen >= 30) {
    down = false;
    reporting = false;
  }
  // inside a 14 px dead zone the press stays at the anchor, so jitter never turns a tap into a swipe
  static bool moved = false;
  if (!reporting) moved = false;
  if (reporting && !moved && (abs(lx - ax) >= 22 || abs(ly - ay) >= 22)) moved = true;  // real movement: follow it
  int px = (reporting && !moved) ? ax : lx;
  int py = (reporting && !moved) ? ay : ly;
  data->state = (down && reporting) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
  data->point.x = px;
  data->point.y = py;
}

static bool touchLoadCal() {
  Preferences p;
  p.begin("touch", true);
  bool ok = p.getBool("ok", false);
  if (ok) {
    cal.x0 = p.getInt("x0");
    cal.x1 = p.getInt("x1");
    cal.y0 = p.getInt("y0");
    cal.y1 = p.getInt("y1");
    cal.swap = p.getBool("sw");
  }
  p.end();
  return ok;
}

static void waitRelease() {
  int a, b;
  uint32_t quiet = millis();
  while (millis() - quiet < 120) {
    if (xptSample(a, b)) quiet = millis();
    lv_timer_handler();
    delay(5);
  }
}

// Touch the four targets (inset 30 px from each corner); computes and stores calibration.
static void touchCalibrate() {
  lv_obj_t *ov = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(ov);
  lv_obj_set_size(ov, 480, 320);
  lv_obj_set_style_bg_opa(ov, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(ov, lv_color_hex(0x070B1A), 0);
  lv_obj_t *title = lv_label_create(ov);
  lv_label_set_text(title, tr("Touch calibration\nTap each circle"));
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xF2F5FF), 0);
  lv_obj_set_style_text_font(title, &vi_font_20, 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
  lv_obj_t *dot = lv_obj_create(ov);
  lv_obj_remove_style_all(dot);
  lv_obj_set_size(dot, 28, 28);
  lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(dot, 4, 0);
  lv_obj_set_style_border_color(dot, lv_color_hex(0x38E1FF), 0);

  const int tx[4] = {30, 450, 450, 30}, ty[4] = {30, 30, 290, 290};
  int rx[4], ry[4];
  waitRelease();
  for (int i = 0; i < 4; i++) {
    lv_obj_set_pos(dot, tx[i] - 14, ty[i] - 14);
    for (;;) {
      lv_timer_handler();
      delay(5);
      static uint32_t lastDiag = 0;
      if (millis() - lastDiag > 700) {  // diagnostics: raw reads whether or not touched
        lastDiag = millis();
        uint16_t dx = 0, dy = 0;
        tftIdle();
        tft.getTouchRaw(&dx, &dy);
        Serial.printf("XPT irq=%d x=%u y=%u z=%u\n", digitalRead(XPT_IRQ), dx, dy, tft.getTouchRawZ());
      }
      int a, b;
      if (!xptSample(a, b)) continue;
      delay(60);  // let the contact settle, then average
      long sa = 0, sb = 0;
      int n = 0;
      for (int k = 0; k < 10; k++) {
        if (xptSample(a, b)) {
          sa += a;
          sb += b;
          n++;
        }
        delay(10);
      }
      if (n >= 5) {
        rx[i] = sa / n;
        ry[i] = sb / n;
        break;
      }
    }
    Serial.printf("CAL %d raw x=%d y=%d\n", i, rx[i], ry[i]);
    waitRelease();
  }

  // TL->TR moves along screen X: whichever raw axis changes most is the horizontal one
  cal.swap = abs(rx[1] - rx[0]) < abs(ry[1] - ry[0]);
  const int *h = cal.swap ? ry : rx, *v = cal.swap ? rx : ry;
  float hl = (h[0] + h[3]) / 2.0f, hr = (h[1] + h[2]) / 2.0f;
  float vt = (v[0] + v[1]) / 2.0f, vb = (v[2] + v[3]) / 2.0f;
  float ph = (hr - hl) / 420.0f, pv = (vb - vt) / 260.0f;  // raw units per pixel
  cal.x0 = (int)(hl - 30 * ph);
  cal.x1 = (int)(hr + 29 * ph);
  cal.y0 = (int)(vt - 30 * pv);
  cal.y1 = (int)(vb + 29 * pv);

  Preferences p;
  p.begin("touch", false);
  p.putInt("x0", cal.x0);
  p.putInt("x1", cal.x1);
  p.putInt("y0", cal.y0);
  p.putInt("y1", cal.y1);
  p.putBool("sw", cal.swap);
  p.putBool("ok", true);
  p.end();
  Serial.printf("CAL done swap=%d x:%d..%d y:%d..%d\n", cal.swap, cal.x0, cal.x1, cal.y0, cal.y1);
  lv_obj_del(ov);
}
