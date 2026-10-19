"""One-off patch: weather card on home, VietQR tile, 3-page dots."""
p = 'D:/Code/ESP32-Display/firmware/src/main.cpp'
s = open(p, encoding='utf-8').read()


def rep(a, b, cnt=1):
    global s
    assert s.count(a) == cnt, (s.count(a), a)
    s = s.replace(a, b)


rep('#include "xiaozhi_activation.h"\n', '#include "xiaozhi_activation.h"\n#include "weather.h"\n')
rep("static lv_obj_t *mainScr, *tv, *homeTile, *appsTile, *dots[2];",
    "static lv_obj_t *mainScr, *tv, *homeTile, *qrTile, *appsTile, *dots[3];")

weather_ui = r'''// ---------- weather card ----------
static const char DEG[] = {(char)0xC2, (char)0xB0, 0};  // UTF-8 degree sign
enum { WK_NONE, WK_CLEAR, WK_MOON, WK_PARTLY, WK_CLOUD, WK_FOG, WK_RAIN, WK_SNOW, WK_STORM };
static lv_obj_t *wCard, *wIcon, *wTemp, *wDesc;
static int wIconKey = -1;

static lv_obj_t *shape(lv_obj_t *p, int x, int y, int w, int h, uint32_t col, int radius = LV_RADIUS_CIRCLE,
                       int opa = LV_OPA_COVER) {
  lv_obj_t *o = lv_obj_create(p);
  lv_obj_remove_style_all(o);
  lv_obj_set_size(o, w, h);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_style_radius(o, radius, 0);
  lv_obj_set_style_bg_opa(o, opa, 0);
  lv_obj_set_style_bg_color(o, lv_color_hex(col), 0);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
  return o;
}
static void cloudShape(lv_obj_t *p, int dx, int dy, uint32_t col) {
  shape(p, dx + 2, dy + 12, 16, 16, col);
  shape(p, dx + 10, dy + 4, 22, 22, col);
  shape(p, dx + 24, dy + 11, 17, 17, col);
  shape(p, dx + 9, dy + 16, 24, 12, col, 6);
}
static void drawWeatherIcon(int kind) {
  lv_obj_clean(wIcon);
  switch (kind) {
    case WK_CLEAR:
      shape(wIcon, 2, 2, 40, 40, 0xFFC93C, LV_RADIUS_CIRCLE, LV_OPA_30);
      shape(wIcon, 9, 9, 26, 26, 0xFFC93C);
      break;
    case WK_MOON:
      shape(wIcon, 8, 8, 28, 28, 0xDDE3FF);
      shape(wIcon, 17, 4, 24, 24, 0x151B33);
      break;
    case WK_PARTLY:
      shape(wIcon, 0, 0, 24, 24, 0xFFC93C);
      cloudShape(wIcon, 3, 8, 0xDDE3FF);
      break;
    case WK_FOG:
      shape(wIcon, 4, 8, 36, 5, 0x9AA3C7, 3);
      shape(wIcon, 10, 19, 30, 5, 0x9AA3C7, 3);
      shape(wIcon, 4, 30, 36, 5, 0x9AA3C7, 3);
      break;
    case WK_RAIN:
      cloudShape(wIcon, 0, 0, 0x9AA3C7);
      shape(wIcon, 12, 31, 3, 9, 0x5AA9FF, 2);
      shape(wIcon, 21, 33, 3, 9, 0x5AA9FF, 2);
      shape(wIcon, 30, 31, 3, 9, 0x5AA9FF, 2);
      break;
    case WK_SNOW:
      cloudShape(wIcon, 0, 0, 0x9AA3C7);
      shape(wIcon, 11, 33, 5, 5, 0xFFFFFF);
      shape(wIcon, 21, 36, 5, 5, 0xFFFFFF);
      shape(wIcon, 30, 33, 5, 5, 0xFFFFFF);
      break;
    case WK_STORM: {
      cloudShape(wIcon, 0, 0, 0x7B84A8);
      lv_obj_t *z = lv_label_create(wIcon);
      lv_label_set_text(z, LV_SYMBOL_CHARGE);
      lv_obj_set_style_text_color(z, lv_color_hex(0xFFC93C), 0);
      lv_obj_set_pos(z, 16, 26);
      break;
    }
    case WK_CLOUD:
      cloudShape(wIcon, 0, 6, 0xB8C1E8);
      break;
    default:  // no data yet: dim cloud outline
      cloudShape(wIcon, 0, 6, 0x2A3359);
      break;
  }
}
static int iconKindFor(const WeatherData &w) {
  int c = w.code;
  if (c <= 1) return w.isDay ? WK_CLEAR : WK_MOON;
  if (c == 2) return WK_PARTLY;
  if (c == 45 || c == 48) return WK_FOG;
  if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82)) return WK_RAIN;
  if ((c >= 71 && c <= 77) || c == 85 || c == 86) return WK_SNOW;
  if (c >= 95) return WK_STORM;
  return WK_CLOUD;
}
static void refreshWeather() {
  WeatherData w = weatherGet();
  int key = WK_NONE;
  if (!w.valid) {
    lv_label_set_text(wTemp, "--");
    lv_label_set_text(wDesc, w.failed ? "Weather unavailable" : (wifiConnected() ? "Loading weather..." : "Waiting for network"));
  } else {
    key = iconKindFor(w);
    lv_label_set_text_fmt(wTemp, "%d%sC", (int)lroundf(w.temp), DEG);
    lv_label_set_text_fmt(wDesc, "%s - %s - %d%%", weatherText(w.code), w.city, w.humidity);
  }
  if (key != wIconKey) {
    drawWeatherIcon(key);
    wIconKey = key;
  }
}
static void weatherTapped(lv_event_t *) {
  weatherRefresh();
  lv_label_set_text(wDesc, "Updating...");
}

'''
rep("// One idempotent pass that makes the home screen reflect the current state.",
    weather_ui + "// One idempotent pass that makes the home screen reflect the current state.")
rep("  if (listening) lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN);\n  else lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);\n}",
    "  if (listening) lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN);\n  else lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);\n  refreshWeather();\n}")

# home: replace the swipe hint with the weather card
a = s.index('  lv_obj_t *hint = makeLabel(tile, "Swipe left for apps')
b = s.index("}\n", a)
card = '''  wCard = makeCard(tile, 214, 62);
  lv_obj_align(wCard, LV_ALIGN_TOP_LEFT, 246, 218);
  lv_obj_add_flag(wCard, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(wCard, weatherTapped, LV_EVENT_CLICKED, nullptr);
  wIcon = lv_obj_create(wCard);
  lv_obj_remove_style_all(wIcon);
  lv_obj_set_size(wIcon, 44, 44);
  lv_obj_align(wIcon, LV_ALIGN_LEFT_MID, 12, 0);
  lv_obj_clear_flag(wIcon, LV_OBJ_FLAG_CLICKABLE);
  wTemp = makeLabel(wCard, "--", F_HEADING, C_TEXT);
  lv_obj_align(wTemp, LV_ALIGN_TOP_LEFT, 66, 5);
  wDesc = makeLabel(wCard, "", F_CAPTION, C_MUTED);
  lv_obj_set_width(wDesc, 140);
  lv_label_set_long_mode(wDesc, LV_LABEL_LONG_DOT);
  lv_obj_align(wDesc, LV_ALIGN_TOP_LEFT, 66, 40);
'''
s = s[:a] + card + s[b:]

qr = r'''// ---------- payment QR (VietQR, MB Bank) ----------
static const char *PAY_QR =
    "00020101021138570010A000000727012700069704220113VQRQAHIZT41140208QRIBFTTA53037045802VN62150107NPS686908006304C8A9";
static const char *PAY_NAME = "HO KINH DOANH ZPLUS";
static const char *PAY_ACCOUNT = "1122100288";

static void buildQr(lv_obj_t *tile) {
  // white card = quiet zone around the code; 225 px = 45 modules x 5 px, so every module is crisp
  lv_obj_t *card = lv_obj_create(tile);
  lv_obj_remove_style_all(card);
  lv_obj_set_size(card, 249, 249);
  lv_obj_align(card, LV_ALIGN_LEFT_MID, 24, -6);
  lv_obj_set_style_radius(card, 18, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *qr = lv_qrcode_create(card, 225, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
  lv_qrcode_update(qr, PAY_QR, strlen(PAY_QR));
  lv_obj_center(qr);

  lv_obj_t *t = makeLabel(tile, "Scan to pay", F_HEADING, C_TEXT);
  lv_obj_set_width(t, 184);
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 290, 36);
  lv_obj_t *bank = makeLabel(tile, "VietQR  |  MB Bank", F_BODY, C_ACCENT);
  lv_obj_align(bank, LV_ALIGN_TOP_LEFT, 290, 76);

  lv_obj_t *k1 = makeLabel(tile, "Account name", F_CAPTION, C_MUTED);
  lv_obj_align(k1, LV_ALIGN_TOP_LEFT, 290, 118);
  lv_obj_t *v1 = makeLabel(tile, PAY_NAME, F_BODY, C_TEXT);
  lv_obj_set_width(v1, 184);
  lv_obj_align(v1, LV_ALIGN_TOP_LEFT, 290, 136);

  lv_obj_t *k2 = makeLabel(tile, "Account number", F_CAPTION, C_MUTED);
  lv_obj_align(k2, LV_ALIGN_TOP_LEFT, 290, 172);
  lv_obj_t *v2 = makeLabel(tile, PAY_ACCOUNT, F_TITLE, C_TEXT);
  lv_obj_set_style_text_letter_space(v2, 1, 0);
  lv_obj_align(v2, LV_ALIGN_TOP_LEFT, 290, 190);

  lv_obj_t *h = makeLabel(tile, "Open your banking app and scan the code", F_CAPTION, C_MUTED);
  lv_obj_set_width(h, 184);
  lv_obj_align(h, LV_ALIGN_TOP_LEFT, 290, 238);
}

'''
rep("// ---------- launcher ----------", qr + "// ---------- launcher ----------")

a = s.index("// ---------- page dots ----------")
b = s.index("static void buildUI()")
dots = '''// ---------- page dots ----------
static void updateDots(int active) {
  for (int i = 0; i < 3; i++) {
    bool on = (i == active);
    lv_obj_set_size(dots[i], on ? 22 : 8, 8);
    lv_obj_set_style_bg_color(dots[i], on ? C_ACCENT : C_MUTED, 0);
    lv_obj_set_style_bg_opa(dots[i], on ? LV_OPA_COVER : LV_OPA_50, 0);
  }
}
static void tvEvent(lv_event_t *) {
  lv_obj_t *act = lv_tileview_get_tile_act(tv);
  updateDots(act == homeTile ? 0 : act == qrTile ? 1 : 2);
}

'''
s = s[:a] + dots + s[b:]
rep("  homeTile = lv_tileview_add_tile(tv, 0, 0, LV_DIR_RIGHT);\n  appsTile = lv_tileview_add_tile(tv, 1, 0, LV_DIR_LEFT);\n  buildHome(homeTile);\n  buildLauncher(appsTile);",
    "  homeTile = lv_tileview_add_tile(tv, 0, 0, LV_DIR_RIGHT);\n  qrTile = lv_tileview_add_tile(tv, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);\n  appsTile = lv_tileview_add_tile(tv, 2, 0, LV_DIR_LEFT);\n  buildHome(homeTile);\n  buildQr(qrTile);\n  buildLauncher(appsTile);")
a = s.index("  for (int i = 0; i < 2; i++) {\n    dots[i] = lv_obj_create(mainScr);")
b = s.index("  lv_timer_create(homeTick")
s = s[:a] + '''  lv_obj_t *dotRow = lv_obj_create(mainScr);
  lv_obj_remove_style_all(dotRow);
  lv_obj_set_size(dotRow, LV_SIZE_CONTENT, 8);
  lv_obj_set_flex_flow(dotRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(dotRow, 6, 0);
  lv_obj_clear_flag(dotRow, LV_OBJ_FLAG_CLICKABLE);
  for (int i = 0; i < 3; i++) {
    dots[i] = lv_obj_create(dotRow);
    lv_obj_remove_style_all(dots[i]);
    lv_obj_set_style_radius(dots[i], 4, 0);
    lv_obj_clear_flag(dots[i], LV_OBJ_FLAG_CLICKABLE);
  }
  lv_obj_align(dotRow, LV_ALIGN_BOTTOM_MID, 0, -12);
  updateDots(0);

''' + s[b:]
rep("  xiaozhiBegin();\n}", "  xiaozhiBegin();\n  weatherBegin();\n}")
open(p, 'w', encoding='utf-8').write(s)
print("patched")
