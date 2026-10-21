// Host test for firmware/src/lunar_core.h + almanac.h.
//   g++ -std=c++17 -O2 -o /tmp/cal_test tools/test_calendar.cpp && /tmp/cal_test 1901 2100 > cpp.txt
// Prints one line per day (every 7th day also the solar terms); the same lines printed by a Swift program built
// from the LichSo project's Util/*.swift must be identical (diff cpp.txt swift.txt). The Swift, Kotlin
// (android/.../util/*.kt) and this code share one algorithm. Range 1901..2100: in 1900 the reference truncates a
// negative k toward zero (a new moon one month late), this code floors it.
// With no arguments it runs known-date checks (Tết dates from the LichSo backend tests) and exits non-zero on failure.
#include <stdio.h>
#include <stdlib.h>
#include "../firmware/src/almanac.h"

static int fails = 0;
static void expect(int dd, int mm, int yy, int ld, int lm, int ly, bool leap) {
  LunarDate l = solarToLunar(dd, mm, yy);
  bool ok = l.day == ld && l.month == lm && l.year == ly && l.leap == leap;
  if (!ok) fails++;
  printf("%s %02d/%02d/%d -> %d/%d/%d%s (want %d/%d/%d%s)\n", ok ? "ok  " : "FAIL", dd, mm, yy, l.day, l.month, l.year, l.leap ? " leap" : "", ld, lm, ly, leap ? " leap" : "");
}

int main(int argc, char **argv) {
  if (argc < 3) {
    expect(29, 1, 2025, 1, 1, 2025, false);   // Tết Ất Tỵ
    expect(12, 2, 2025, 15, 1, 2025, false);  // rằm tháng Giêng
    expect(7, 4, 2025, 10, 3, 2025, false);   // Giỗ Tổ
    expect(5, 3, 2026, 17, 1, 2026, false);
    expect(17, 2, 2026, 1, 1, 2026, false);   // Tết Bính Ngọ
    expect(21, 10, 2026, 12, 9, 2026, false);
    long jd = lunar::jdFromDate(21, 10, 2026);
    printf("21/10/2026 weekday=%d (2 = Wednesday)\n", alm::weekday(jd));
    if (alm::weekday(jd) != 2) fails++;
    return fails ? 1 : 0;
  }
  int y0 = atoi(argv[1]), y1 = atoi(argv[2]);
  long a = lunar::jdFromDate(1, 1, y0), b = lunar::jdFromDate(31, 12, y1);
  for (long jd = a; jd <= b; jd++) {
    int d, m, y;
    jdToDate(jd, d, m, y);
    LunarDate l = solarToLunar(d, m, y);
    printf("%ld %d/%d/%d L%d/%d/%d/%d len%d wd%d dc%d-%d mc%d-%d yc%d-%d truc%d sao%d gio%x moon%d", jd, d, m, y, l.day, l.month, l.year, l.leap ? 1 : 0,
           lunarMonthLength(l.start), alm::weekday(jd), alm::dayCan(jd), alm::dayChi(jd), alm::monthCan(l.month, l.year), alm::monthChi(l.month),
           alm::yearCan(l.year), alm::yearChi(l.year), alm::trucIndex(jd, l.month), alm::saoIndex(jd), alm::luckyHours(jd), alm::moonPhase(jd));
    if (jd % 7 == 0) {
      alm::TermInfo t = alm::solarTerm(jd, y);
      printf(" term %s>%s %ld", alm::TERMS[t.cur].name, alm::TERMS[t.next].name, t.nextJd - jd);
    }
    printf("\n");
  }
  return 0;
}
