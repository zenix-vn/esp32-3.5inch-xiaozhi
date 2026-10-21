#pragma once
// Almanac data for the Calendar app: Can Chi of day / month / year, 24 solar terms, Trực, Sao, lucky hours,
// moon phase, Vietnamese holidays, "should / should not do" lists and lucky directions.
// Ported unchanged from the LichSo project (ios/LichSo/LichSo/Util/*.swift); tools/test_calendar.cpp checks
// this file and lunar_core.h against that code for every day from 1900 to 2100.
// Pure C++ (no Arduino / LVGL). Day of week: Julian day % 7 -> 0 = Monday ... 6 = Sunday.
#include <stdint.h>
#include "lunar_core.h"

namespace alm {

enum Rating { R_GOOD, R_MID, R_BAD };

// ---------- Can Chi ----------
static int yearCan(int lunarYear) { return (lunarYear + 6) % 10; }
static int yearChi(int lunarYear) { return (lunarYear + 8) % 12; }
static int monthCan(int lunarMonth, int lunarYear) { return (lunarYear * 12 + lunarMonth + 3) % 10; }
static int monthChi(int lunarMonth) { return (lunarMonth + 1) % 12; }
static int dayCan(long jd) { return (int)((jd + 9) % 10); }
static int dayChi(long jd) { return (int)((jd + 1) % 12); }
static int weekday(long jd) { return (int)(jd % 7); }  // 0 = Monday ... 6 = Sunday

// ---------- Trực (12 duty days) ----------
static const char *const TRUC[12] = {"Kiến", "Trừ", "Mãn", "Bình", "Định", "Chấp", "Phá", "Nguy", "Thành", "Thu", "Khai", "Bế"};
static const Rating TRUC_RATING[12] = {R_MID, R_GOOD, R_GOOD, R_GOOD, R_GOOD, R_MID, R_BAD, R_BAD, R_GOOD, R_MID, R_GOOD, R_BAD};
static int trucIndex(long jd, int lunarMonth) { return (int)(((dayChi(jd) - (lunarMonth - 1)) % 12 + 12) % 12); }

// ---------- Sao (28 mansions) ----------
static const char *const SAO[28] = {"Giác", "Cang", "Đê",  "Phòng", "Tâm", "Vĩ",  "Cơ",    "Đẩu",   "Ngưu",  "Nữ",     "Hư",  "Nguy", "Thất", "Bích",
                                    "Khuê", "Lâu",  "Vị",  "Mão",   "Tất", "Chủy", "Sâm",  "Tỉnh",  "Quỷ",   "Liễu",   "Tinh", "Trương", "Dực", "Chẩn"};
static const Rating SAO_RATING[28] = {R_GOOD, R_BAD, R_BAD, R_GOOD, R_BAD,  R_GOOD, R_GOOD, R_GOOD, R_BAD,  R_BAD, R_BAD, R_MID, R_GOOD, R_GOOD,
                                      R_BAD,  R_GOOD, R_GOOD, R_BAD, R_GOOD, R_BAD,  R_GOOD, R_GOOD, R_BAD,  R_BAD, R_GOOD, R_GOOD, R_GOOD, R_GOOD};
static int saoIndex(long jd) { return (int)((jd + 15) % 28); }

// ---------- lucky hours (giờ hoàng đạo) ----------
static const char *const GIO_TIME[12] = {"23h-1h", "1h-3h", "3h-5h", "5h-7h", "7h-9h", "9h-11h", "11h-13h", "13h-15h", "15h-17h", "17h-19h", "19h-21h", "21h-23h"};
// Bit i set = the hour of Chi i (0 = Tý ... 11 = Hợi) is lucky; days with an even Chi use {0,1,4,5,8,9}, odd {2,3,6,7,10,11}.
static unsigned luckyHours(long jd) { return dayChi(jd) % 2 == 0 ? 0x333u : 0xCCCu; }

// ---------- moon ----------
static const char *const MOON_VI[] = {"Trăng mới", "Trăng lưỡi liềm đầu", "Bán nguyệt đầu", "Trăng khuyết đầu", "Trăng tròn", "Trăng khuyết cuối", "Bán nguyệt cuối", "Trăng lưỡi liềm cuối"};
static const char *const MOON_EN[] = {"New moon", "Waxing crescent", "First quarter", "Waxing gibbous", "Full moon", "Waning gibbous", "Last quarter", "Waning crescent"};
// Phase index into MOON_*; `age` = days since the new moon. (The reference names the phases by age bands.)
static int moonPhase(long jd, double *age = nullptr) {
  long k = (long)((jd - 2415021.076998695) / 29.530588853);
  long nm = lunar::newMoonDay(k, 7);
  if (nm > jd) nm = lunar::newMoonDay(--k, 7);
  double a = (double)(jd - nm);
  if (age) *age = a;
  return a < 1.5 ? 0 : a < 7.4 ? 1 : a < 8.4 ? 2 : a < 14.4 ? 3 : a < 15.8 ? 4 : a < 21.8 ? 5 : a < 22.8 ? 6 : a < 28.5 ? 7 : 0;
}

// ---------- 24 solar terms ----------
struct Term {
  const char *name;
  int lon;
};
static const Term TERMS[24] = {{"Xuân Phân", 0},   {"Thanh Minh", 15},   {"Cốc Vũ", 30},      {"Lập Hạ", 45},   {"Tiểu Mãn", 60},   {"Mang Chủng", 75},
                               {"Hạ Chí", 90},    {"Tiểu Thử", 105},    {"Đại Thử", 120},    {"Lập Thu", 135}, {"Xử Thử", 150},    {"Bạch Lộ", 165},
                               {"Thu Phân", 180}, {"Hàn Lộ", 195},      {"Sương Giáng", 210}, {"Lập Đông", 225}, {"Tiểu Tuyết", 240}, {"Đại Tuyết", 255},
                               {"Đông Chí", 270}, {"Tiểu Hàn", 285},    {"Đại Hàn", 300},    {"Lập Xuân", 315}, {"Vũ Thủy", 330},   {"Kinh Trập", 345}};

static double sunLongitudeDegree(long jdn, double tz = 7) {
  double T = (jdn - 2451545.5 - tz / 24.0) / 36525.0, T2 = T * T, dr = M_PI / 180.0;
  double M = 357.52910 + 35999.05030 * T - 0.0001559 * T2 - 0.00000048 * T * T2;
  double L0 = 280.46645 + 36000.76983 * T + 0.0003032 * T2;
  double DL = (1.914600 - 0.004817 * T - 0.000014 * T2) * sin(dr * M);
  DL += (0.019993 - 0.000101 * T) * sin(dr * 2 * M) + 0.000290 * sin(dr * 3 * M);
  double L = fmod(L0 + DL, 360.0);
  if (L < 0) L += 360.0;
  return L;
}

// Julian day on which the sun reaches ecliptic longitude `lon` in `year`.
static long termDate(int year, int lon) {
  long est = lunar::jdFromDate(20, 3, year) + (long)floor(lon * (365.25 / 360.0));
  for (long jd = est - 5; jd <= est + 5; jd++) {
    double l1 = sunLongitudeDegree(jd), l2 = sunLongitudeDegree(jd + 1);
    if (l2 < l1) l2 += 360.0;
    double t = lon;
    if ((l1 <= t && t < l2) || (l1 <= t + 360 && t + 360 < l2)) return jd;
  }
  return est;
}

// The term in force on jd (index into TERMS, starting on curJd) and the next one.
struct TermInfo {
  int cur, next;
  long curJd, nextJd;
};
static TermInfo solarTerm(long jd, int year) {
  TermInfo r = {-1, -1, 0, 0};
  long best = -1, nxt = 0x7fffffff;
  for (int y = year - 1; y <= year + 1; y++)
    for (int i = 0; i < 24; i++) {
      long d = termDate(y, TERMS[i].lon);
      if (d <= jd && d > best) {
        best = d;
        r.cur = i;
        r.curJd = d;
      }
      if (d > jd && d < nxt) {
        nxt = d;
        r.next = i;
        r.nextJd = d;
      }
    }
  return r;
}

// ---------- holidays ----------
struct Holiday {
  uint8_t d, m;
  const char *vi, *en;
  bool off;  // public day off
};
static const Holiday SOLAR_HOL[] = {
    {1, 1, "Tết Dương lịch", "New Year's Day", true},
    {14, 2, "Valentine", "Valentine's Day", false},
    {8, 3, "Quốc tế Phụ nữ", "Women's Day", false},
    {30, 4, "Giải phóng miền Nam", "Reunification Day", true},
    {1, 5, "Quốc tế Lao động", "Labour Day", true},
    {19, 5, "Sinh nhật Bác Hồ", "Ho Chi Minh's birthday", false},
    {1, 6, "Quốc tế Thiếu nhi", "Children's Day", false},
    {27, 7, "Thương binh Liệt sĩ", "War Invalids Day", false},
    {2, 9, "Quốc khánh", "National Day", true},
    {20, 10, "Phụ nữ Việt Nam", "Vietnamese Women's Day", false},
    {20, 11, "Nhà giáo Việt Nam", "Teachers' Day", false},
    {22, 12, "Quân đội Nhân dân VN", "Army Day", false},
    {24, 12, "Giáng sinh", "Christmas Eve", false},
    {25, 12, "Giáng sinh", "Christmas", false},
};
static const Holiday LUNAR_HOL[] = {
    {1, 1, "Tết Nguyên Đán", "Lunar New Year", true},
    {2, 1, "Mùng 2 Tết", "Lunar New Year, day 2", true},
    {3, 1, "Mùng 3 Tết", "Lunar New Year, day 3", true},
    {15, 1, "Rằm tháng Giêng", "Full moon of the 1st month", false},
    {3, 3, "Tết Hàn thực", "Cold Food Festival", false},
    {10, 3, "Giỗ Tổ Hùng Vương", "Hung Kings' Festival", true},
    {15, 4, "Phật Đản", "Buddha's Birthday", false},
    {5, 5, "Tết Đoan Ngọ", "Mid-year Festival", false},
    {15, 7, "Rằm tháng Bảy (Vu Lan)", "Vu Lan (Ghost Festival)", false},
    {15, 8, "Tết Trung Thu", "Mid-Autumn Festival", false},
    {9, 9, "Tết Trùng Dương", "Double Ninth Festival", false},
    {10, 10, "Tết cơm mới", "New Rice Festival", false},
    {15, 10, "Rằm tháng Mười", "Full moon of the 10th month", false},
    {23, 12, "Ông Táo về trời", "Kitchen Gods' Day", false},
};
static const Holiday TAT_NIEN = {0, 12, "Tất niên", "Lunar New Year's Eve", false};

// Holidays on a day (solar first, then lunar), up to 3. Lunar holidays never match a leap month;
// "Tất niên" is the last day of the 12th month, which is the 29th or the 30th.
static int holidaysOn(int dd, int mm, int ld, int lm, bool leap, int lunarMonthLen, const Holiday **out) {
  int n = 0;
  for (const Holiday &h : SOLAR_HOL)
    if (h.d == dd && h.m == mm && n < 3) out[n++] = &h;
  if (!leap) {
    for (const Holiday &h : LUNAR_HOL)
      if (h.d == ld && h.m == lm && n < 3) out[n++] = &h;
    if (lm == 12 && ld == lunarMonthLen && n < 3) out[n++] = &TAT_NIEN;
  }
  return n;
}

// ---------- should / should not do, lucky directions ----------
static const char *const NEN_LAM[12][4] = {
    {"Giao dịch, ký kết", "Xuất hành", "Cưới hỏi", "Khai trương"},  {"Cầu tài, cầu phúc", "Nhập học", "An sàng", "Dựng cột"},
    {"Động thổ, xây dựng", "Nhập trạch", "Giao dịch", "Cầu tài"},   {"Cầu an, giải hạn", "Tu sửa", "Nhập học", "Cưới hỏi"},
    {"Xuất hành", "Khai trương", "Giao dịch", "Ký kết"},            {"Cầu phúc, cúng tế", "An táng", "Nhập trạch", "Tu sửa"},
    {"Giao dịch, ký kết", "Xuất hành", "Khai trương", "Nhập học"},  {"Cầu tài", "An sàng", "Tu sửa", "Nhập trạch"},
    {"Động thổ", "Dựng cột", "Giao dịch", "Xuất hành"},             {"Cầu an", "Tu sửa", "Nhập học", "Cầu tài"},
    {"Cưới hỏi", "Xuất hành", "Ký kết", "Khai trương"},             {"Cầu phúc", "An sàng", "Nhập trạch", "Giao dịch"}};
static const char *const KHONG_NEN[12][2] = {{"An táng", "Kiện cáo"},   {"Xuất hành xa", "Phá thổ"}, {"Cưới hỏi", "Kiện cáo"}, {"Khai trương", "An táng"},
                                             {"An táng", "Tu sửa"},     {"Xuất hành", "Kiện cáo"},   {"An táng", "Phá thổ"},   {"Khai trương", "Kiện cáo"},
                                             {"Cưới hỏi", "An táng"},   {"Xuất hành xa", "Phá thổ"}, {"An táng", "Kiện cáo"},  {"Xuất hành", "Tu sửa"}};
static const char *const THAN_TAI[10] = {"Đông-Nam", "Đông", "Bắc", "Bắc", "Đông-Bắc", "Đông", "Tây-Nam", "Tây", "Bắc", "Nam"};
static const char *const HY_THAN[10] = {"Đông-Bắc", "Tây-Bắc", "Tây-Nam", "Nam", "Đông-Nam", "Đông-Bắc", "Tây-Bắc", "Tây-Nam", "Nam", "Đông-Nam"};
static const char *const HUNG_THAN[12] = {"Nam", "Đông", "Bắc", "Tây", "Nam", "Đông", "Bắc", "Tây", "Nam", "Đông", "Bắc", "Tây"};

static bool isNguyetKy(int lunarDay) { return lunarDay == 5 || lunarDay == 14 || lunarDay == 23; }
static bool isTamNuong(int lunarDay) { return lunarDay == 3 || lunarDay == 7 || lunarDay == 13 || lunarDay == 18 || lunarDay == 22 || lunarDay == 27; }

}  // namespace alm
