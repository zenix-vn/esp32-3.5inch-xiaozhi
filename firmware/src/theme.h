#pragma once
// "Bầu trời Zin": screen background follows the time of day, in one of four colour themes.
// Included by main.cpp after config.h; styleScreen() paints every screen with the current palette.

struct ThemePal {
  uint32_t top, bot;
};
// [theme][slot]; slot 0 dawn 5-8h, 1 day 8-17h, 2 dusk 17-19h, 3 night 19-5h. Deep tones keep white text readable.
static const ThemePal THEMES[4][4] = {
    {{0x2B2F6B, 0x9A5A68}, {0x123A7A, 0x2F6FB8}, {0x3B1E5E, 0xA8503A}, {0x070B1A, 0x1D1146}},  // sky
    {{0x1F3B2E, 0x7A6A3E}, {0x0F3D30, 0x2C7A57}, {0x2E2A1E, 0x8A4E2A}, {0x06140F, 0x10291F}},  // forest
    {{0x3A1C4A, 0xA0503C}, {0x5A2048, 0xB0603A}, {0x2C0F3A, 0x9A3A2C}, {0x14081E, 0x2A0E2E}},  // sunset
    {{0x16181F, 0x2A2D38}, {0x1C1F27, 0x343845}, {0x16181F, 0x2A2D38}, {0x0A0B0F, 0x17181E}},  // mono
};
static int themeSlot = 3;

static int themeSlotNow() {
  struct tm t;
  if (time(nullptr) < 1700000000 || !getLocalTime(&t, 0)) return 3;
  int h = t.tm_hour;
  return h >= 5 && h < 8 ? 0 : h >= 8 && h < 17 ? 1 : h >= 17 && h < 19 ? 2 : 3;
}
static bool themeIsNight() { return themeSlot == 3; }

static void styleScreen(lv_obj_t *s) {
  const ThemePal &p = THEMES[cfg.theme & 3][themeSlot];
  lv_obj_set_style_bg_color(s, lv_color_hex(p.top), 0);
  lv_obj_set_style_bg_grad_color(s, lv_color_hex(p.bot), 0);
  lv_obj_set_style_bg_grad_dir(s, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
  lv_obj_clear_flag(s, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *mainScr;
// Re-paint when the time slot changes (called every few seconds from loop()).
static void themeTick(bool force = false) {
  int s = themeSlotNow();
  if (s == themeSlot && !force) return;
  themeSlot = s;
  if (mainScr) styleScreen(mainScr);
  if (lv_scr_act() != mainScr) styleScreen(lv_scr_act());
}
