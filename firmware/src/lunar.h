#pragma once
// Vietnamese lunar calendar (Hồ Ngọc Đức's algorithm, UTC+7), computed on the device - no network needed.
#include <math.h>

struct LunarDate {
  int day, month, year;
  bool leap;
};

namespace lunar {
static long jdFromDate(int dd, int mm, int yy) {
  long a = (14 - mm) / 12, y = yy + 4800 - a, m = mm + 12 * a - 3;
  long jd = dd + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
  if (jd < 2299161) jd = dd + (153 * m + 2) / 5 + 365 * y + y / 4 - 32083;
  return jd;
}
static long newMoonDay(long k, double tz) {
  double T = k / 1236.85, T2 = T * T, T3 = T2 * T, dr = M_PI / 180;
  double jd1 = 2415020.75933 + 29.53058868 * k + 0.0001178 * T2 - 0.000000155 * T3;
  jd1 += 0.00033 * sin((166.56 + 132.87 * T - 0.009173 * T2) * dr);
  double M = 359.2242 + 29.10535608 * k - 0.0000333 * T2 - 0.00000347 * T3;
  double Mpr = 306.0253 + 385.81691806 * k + 0.0107306 * T2 + 0.00001236 * T3;
  double F = 21.2964 + 390.67050646 * k - 0.0016528 * T2 - 0.00000239 * T3;
  double C1 = (0.1734 - 0.000393 * T) * sin(M * dr) + 0.0021 * sin(2 * dr * M);
  C1 = C1 - 0.4068 * sin(Mpr * dr) + 0.0161 * sin(dr * 2 * Mpr);
  C1 = C1 - 0.0004 * sin(dr * 3 * Mpr);
  C1 = C1 + 0.0104 * sin(dr * 2 * F) - 0.0051 * sin(dr * (M + Mpr));
  C1 = C1 - 0.0074 * sin(dr * (M - Mpr)) + 0.0004 * sin(dr * (2 * F + M));
  C1 = C1 - 0.0004 * sin(dr * (2 * F - M)) - 0.0006 * sin(dr * (2 * F + Mpr));
  C1 = C1 + 0.0010 * sin(dr * (2 * F - Mpr)) + 0.0005 * sin(dr * (2 * Mpr + M));
  double dt = T < -11 ? 0.001 + 0.000839 * T + 0.0002261 * T2 - 0.00000845 * T3 - 0.000000081 * T * T3
                      : -0.000278 + 0.000265 * T + 0.000262 * T2;
  return (long)floor(jd1 + C1 - dt + 0.5 + tz / 24);
}
static int sunLongitude(long jdn, double tz) {
  double T = (jdn - 2451545.5 - tz / 24) / 36525, T2 = T * T, dr = M_PI / 180;
  double M = 357.52910 + 35999.05030 * T - 0.0001559 * T2 - 0.00000048 * T * T2;
  double L0 = 280.46645 + 36000.76983 * T + 0.0003032 * T2;
  double DL = (1.914600 - 0.004817 * T - 0.000014 * T2) * sin(dr * M);
  DL = DL + (0.019993 - 0.000101 * T) * sin(dr * 2 * M) + 0.000290 * sin(dr * 3 * M);
  double L = (L0 + DL) * dr;
  L = L - M_PI * 2 * floor(L / (M_PI * 2));
  return (int)floor(L / M_PI * 6);
}
static long lunarMonth11(int yy, double tz) {
  long off = jdFromDate(31, 12, yy) - 2415021;
  long k = (long)floor(off / 29.530588853);
  long nm = newMoonDay(k, tz);
  if (sunLongitude(nm, tz) >= 9) nm = newMoonDay(k - 1, tz);
  return nm;
}
static int leapMonthOffset(long a11, double tz) {
  long k = (long)floor((a11 - 2415021.076998695) / 29.530588853 + 0.5);
  int i = 1, last, arc = sunLongitude(newMoonDay(k + i, tz), tz);
  do {
    last = arc;
    i++;
    arc = sunLongitude(newMoonDay(k + i, tz), tz);
  } while (arc != last && i < 14);
  return i - 1;
}
}  // namespace lunar

static LunarDate solarToLunar(int dd, int mm, int yy, double tz = 7) {
  using namespace lunar;
  long dayNumber = jdFromDate(dd, mm, yy);
  long k = (long)floor((dayNumber - 2415021.076998695) / 29.530588853);
  long monthStart = newMoonDay(k + 1, tz);
  if (monthStart > dayNumber) monthStart = newMoonDay(k, tz);
  long a11 = lunarMonth11(yy, tz), b11 = a11;
  int lunarYear;
  if (a11 >= monthStart) {
    lunarYear = yy;
    a11 = lunarMonth11(yy - 1, tz);
  } else {
    lunarYear = yy + 1;
    b11 = lunarMonth11(yy + 1, tz);
  }
  int lunarDay = dayNumber - monthStart + 1;
  int diff = (int)floor((monthStart - a11) / 29.0);
  bool leap = false;
  int lunarMonth = diff + 11;
  if (b11 - a11 > 365) {
    int leapDiff = leapMonthOffset(a11, tz);
    if (diff >= leapDiff) {
      lunarMonth = diff + 10;
      if (diff == leapDiff) leap = true;
    }
  }
  if (lunarMonth > 12) lunarMonth -= 12;
  if (lunarMonth >= 11 && diff < 4) lunarYear -= 1;
  return {lunarDay, lunarMonth, lunarYear, leap};
}

static const char *const CAN[10] = {"Giáp", "Ất", "Bính", "Đinh", "Mậu", "Kỷ", "Canh", "Tân", "Nhâm", "Quý"};
static const char *const CHI[12] = {"Tý", "Sửu", "Dần", "Mão", "Thìn", "Tỵ", "Ngọ", "Mùi", "Thân", "Dậu", "Tuất", "Hợi"};
static const char *const CHI_EN[12] = {"Rat", "Ox", "Tiger", "Cat", "Dragon", "Snake", "Horse", "Goat", "Monkey", "Rooster", "Dog", "Pig"};

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
