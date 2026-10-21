#pragma once
// Vietnamese lunar calendar (Hồ Ngọc Đức's algorithm, UTC+7), computed on the device - no network needed.
// Pure C++ (no Arduino / LVGL), so tools/test_calendar.cpp can check it on a computer against the reference.
// New moons use the Meeus series of the LichSo project (more accurate than the 1991 low-precision series: a new
// moon near local midnight no longer lands on the wrong day).
#include <math.h>

struct LunarDate {
  int day, month, year;
  bool leap;
  long start;  // Julian day number of the first day of this lunar month
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
  // periodic terms: Meeus, "Astronomical Algorithms" 2nd ed., ch. 49, in Julian centuries from J2000
  double Tj = (jd1 - 2451545.0) / 36525.0, Tj2 = Tj * Tj, Tj3 = Tj2 * Tj, Tj4 = Tj3 * Tj;
  double Msun = 357.5291092 + 35999.0502909 * Tj - 0.0001536 * Tj2 + Tj3 / 24490000.0;
  double Mmoon = 134.9634114 + 477198.8676313 * Tj + 0.008997 * Tj2 + Tj3 / 69699.0 - Tj4 / 14712000.0;
  double F = 93.2720993 + 483202.0175273 * Tj - 0.0034029 * Tj2 - Tj3 / 3526000.0 + Tj4 / 863310000.0;
  double Omega = 125.0445479 - 1934.1362891 * Tj + 0.0020754 * Tj2 + Tj3 / 467441.0 - Tj4 / 60616000.0;
  double E = 1 - 0.002516 * Tj - 0.0000074 * Tj2;
  double Ms = Msun * dr, Mm = Mmoon * dr, Fr = F * dr, Om = Omega * dr;
  double C1 = -0.40720 * sin(Mm);
  C1 += 0.17241 * E * sin(Ms);
  C1 += 0.01608 * sin(2 * Mm);
  C1 += 0.01039 * sin(2 * Fr);
  C1 += 0.00739 * E * sin(Mm - Ms);
  C1 -= 0.00514 * E * sin(Mm + Ms);
  C1 += 0.00208 * E * E * sin(2 * Ms);
  C1 -= 0.00111 * sin(Mm - 2 * Fr);
  C1 -= 0.00057 * sin(Mm + 2 * Fr);
  C1 += 0.00056 * E * sin(2 * Mm + Ms);
  C1 -= 0.00042 * sin(3 * Mm);
  C1 += 0.00042 * E * sin(Ms + 2 * Fr);
  C1 += 0.00038 * E * sin(Ms - 2 * Fr);
  C1 -= 0.00024 * E * sin(2 * Mm - Ms);
  C1 -= 0.00017 * sin(Om);
  C1 -= 0.00007 * sin(Mm + 2 * Ms);
  C1 += 0.00004 * sin(2 * Mm - 2 * Fr);
  C1 += 0.00004 * sin(3 * Ms);
  C1 += 0.00003 * sin(Mm + Ms - 2 * Fr);
  C1 += 0.00003 * sin(2 * Mm + 2 * Fr);
  C1 -= 0.00003 * sin(Mm + Ms + 2 * Fr);
  C1 += 0.00003 * sin(Mm - Ms + 2 * Fr);
  C1 -= 0.00002 * sin(Mm - Ms - 2 * Fr);
  C1 -= 0.00002 * sin(3 * Mm + Ms);
  C1 += 0.00002 * sin(4 * Mm);
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
  return {lunarDay, lunarMonth, lunarYear, leap, monthStart};
}


// Julian day number -> day, month, year.
static void jdToDate(long jd, int &dd, int &mm, int &yy) {
  long a, b, c;
  if (jd > 2299160) {
    a = jd + 32044;
    b = (4 * a + 3) / 146097;
    c = a - b * 146097 / 4;
  } else {
    b = 0;
    c = jd + 32082;
  }
  long d = (4 * c + 3) / 1461, e = c - 1461 * d / 4, m = (5 * e + 2) / 153;
  dd = (int)(e - (153 * m + 2) / 5 + 1);
  mm = (int)(m + 3 - 12 * (m / 10));
  yy = (int)(b * 100 + d - 4800 + m / 10);
}

// Number of days of the lunar month that starts on Julian day `start` (29 or 30).
static int lunarMonthLength(long start, double tz = 7) {
  long k = (long)floor((start - 2415021.076998695) / 29.530588853 + 0.5);
  return (int)(lunar::newMoonDay(k + 1, tz) - start);
}

static const char *const CAN[10] = {"Giáp", "Ất", "Bính", "Đinh", "Mậu", "Kỷ", "Canh", "Tân", "Nhâm", "Quý"};
static const char *const CHI[12] = {"Tý", "Sửu", "Dần", "Mão", "Thìn", "Tỵ", "Ngọ", "Mùi", "Thân", "Dậu", "Tuất", "Hợi"};
static const char *const CHI_EN[12] = {"Rat", "Ox", "Tiger", "Cat", "Dragon", "Snake", "Horse", "Goat", "Monkey", "Rooster", "Dog", "Pig"};
