#pragma once
// Lunar date text for the home and ambient screens. The algorithm lives in lunar_core.h.
#include <Arduino.h>
#include "lunar_core.h"

// "Âm lịch 10/9 Bính Ngọ" / "Lunar 10/9, Year of the Horse" (cached for the day)
static String lunarText(int dd, int mm, int yy) {
  static int cKey = 0;
  static String cText;
  int key = yy * 10000 + mm * 100 + dd + (isVi() ? 0 : 50000000);
  if (key == cKey) return cText;
  LunarDate l = solarToLunar(dd, mm, yy);
  char buf[64];
  if (isVi())
    snprintf(buf, sizeof(buf), "Âm lịch %d/%d%s %s %s", l.day, l.month, l.leap ? " nhuận" : "", CAN[(l.year + 6) % 10], CHI[(l.year + 8) % 12]);
  else
    snprintf(buf, sizeof(buf), "Lunar %d/%d%s, Year of the %s", l.day, l.month, l.leap ? " (leap)" : "", CHI_EN[(l.year + 8) % 12]);
  cKey = key;
  cText = buf;
  return cText;
}
