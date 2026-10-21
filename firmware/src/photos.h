#pragma once
// Photo frame (V2.4): JPG files from /photos on the microSD card, shown full screen as a slideshow.
// Tap the right/left half for next/previous, hold to leave. Pictures are decoded straight to the display
// (TJpg_Decoder) on an empty black LVGL screen; if LVGL repaints anything the picture is drawn again.
#include <TJpg_Decoder.h>

static const char *PHOTO_DIR = "/photos";
static lv_obj_t *phScr = nullptr, *phHint = nullptr;
static String *phFiles = nullptr;
static int phCount = 0, phIdx = 0;
static uint32_t phNext = 0, phHintUntil = 0, phDrawnFlush = 0;
static bool phDirty = false;

static bool phOut(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bmp) {
  if (y >= 320) return false;
  tft.pushImage(x, y, w, h, bmp);
  return true;
}

// JPEG files of one directory (not recursive), appended to names[] as full paths.
static bool phIsJpeg(File &f, const String &low) {
  if (low.endsWith(".jpg") || low.endsWith(".jpeg")) return true;
  if (low.indexOf('.') >= 0 || f.size() < 4 || f.size() > 8UL * 1024 * 1024) return false;
  uint8_t m[2] = {0, 0};  // a name without an extension (some tools drop it): look at the JPEG signature
  File t = SD.open(String("/") + f.name(), FILE_READ);
  if (t) {
    t.read(m, 2);
    t.close();
  }
  return m[0] == 0xFF && m[1] == 0xD8;
}
static void phScanDir(const char *path, String *names, int &n, int max) {
  if (!SD.exists(path)) return;  // avoids a log line per missing folder
  File dir = SD.open(path);
  if (!dir || !dir.isDirectory()) return;
  for (File f = dir.openNextFile(); f && n < max; f = dir.openNextFile()) {
    String nm = f.name();
    int slash = nm.lastIndexOf('/');
    if (slash >= 0) nm = nm.substring(slash + 1);
    String low = nm;
    low.toLowerCase();
    String full = String(path) + (path[1] ? "/" : "") + nm;
    if (!f.isDirectory() && !low.startsWith(".") && !low.startsWith("_") && phIsJpeg(f, low)) names[n++] = full;
    f.close();
  }
  dir.close();
}

static void phScan() {
  delete[] phFiles;
  phFiles = nullptr;
  phCount = 0;
  String names[100];
  int n = 0;
  // the folder this app documents first, then the usual alternatives, then the card's root
  static const char *const DIRS[] = {PHOTO_DIR, "/photo", "/pictures", "/images", "/DCIM", "/"};
  for (const char *d : DIRS) phScanDir(d, names, n, 100);
  for (int i = 1; i < n; i++)  // sort by path
    for (int j = i; j > 0 && names[j] < names[j - 1]; j--) {
      String t = names[j];
      names[j] = names[j - 1];
      names[j - 1] = t;
    }
  if (!n) return;
  phFiles = new String[n];
  for (int i = 0; i < n; i++) phFiles[i] = names[i];
  phCount = n;
}

// How many pictures the Photos app would show (used by the Storage page).
static int photosCount() {
  if (!sdOk) return 0;
  phScan();
  int n = phCount;
  delete[] phFiles;
  phFiles = nullptr;
  phCount = 0;
  return n;
}

static void phDraw() {
  const char *path = phFiles[phIdx].c_str();
  uint16_t w = 0, h = 0;
  tftIdle();
  tft.fillScreen(TFT_BLACK);
  if (TJpgDec.getSdJpgSize(&w, &h, path) != JDR_OK || !w || !h) return;
  uint8_t sc = 1;
  while (sc < 8 && (w / sc > 480 || h / sc > 320)) sc <<= 1;
  TJpgDec.setJpgScale(sc);
  tft.setSwapBytes(true);
  TJpgDec.drawSdJpg((480 - w / sc) / 2, (320 - h / sc) / 2, path);
  tft.setSwapBytes(false);
}

static void phExit() {
  if (!phScr) return;
  lv_obj_t *s = phScr;
  phScr = nullptr;
  delete[] phFiles;
  phFiles = nullptr;
  statusbarShow(true);
  lv_scr_load(mainScr);
  lv_obj_del(s);
}

// Called from loop() right after lv_timer_handler().
static void photosLoop() {
  if (!phScr || lv_scr_act() != phScr) return;
  uint32_t now = millis();
  if (phHint && now > phHintUntil) {
    lv_obj_del(phHint);  // LVGL repaints the black screen, then the picture is drawn
    phHint = nullptr;
    phDirty = true;
    return;
  }
  if (phHint) return;
  if (phCount && now >= phNext) {
    phIdx = (phIdx + 1) % phCount;
    phDirty = true;
  }
  if (gFlushCount != phDrawnFlush) phDirty = true;  // LVGL drew over the picture
  if (phDirty && phCount) {
    phDirty = false;
    phDraw();
    phDrawnFlush = gFlushCount;
    phNext = millis() + (uint32_t)cfg.photoSec * 1000UL;
  }
}

static void pagePhotos() {
  if (sdOk) phScan();
  if (!phCount) {
    lv_obj_t *c;
    lv_obj_t *s = pageBegin("Photos", &c);
    lv_obj_t *card = makeCard(c, 456, 150);
    lv_obj_t *t = makeLabel(card, sdOk ? DL("Chưa có ảnh", "No photos yet") : DL("Cần thẻ nhớ microSD", "A microSD card is required"), F_TITLE, C_WARN);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_t *h = makeLabel(card, DL("Chép ảnh JPG vào thư mục /photos (hoặc /photo, hoặc ngay thư mục gốc) của thẻ nhớ, nên cỡ 480x320, dạng baseline, rồi mở lại Ảnh.",
                                     "Copy JPG pictures into the /photos folder (or /photo, or the root) of the card, ideally 480x320 baseline, then open Photos again."),
                              F_BODY, C_MUTED);
    lv_obj_set_width(h, 420);
    lv_label_set_long_mode(h, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(h, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(h, LV_ALIGN_TOP_MID, 0, 56);
    pageShow(s);
    return;
  }
  TJpgDec.setCallback(phOut);
  phScr = lv_obj_create(nullptr);
  lv_obj_remove_style_all(phScr);
  lv_obj_set_style_bg_color(phScr, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(phScr, LV_OPA_COVER, 0);
  lv_obj_add_flag(phScr, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(
      phScr,
      [](lv_event_t *) {
        if (phHint) return;
        lv_point_t p;
        lv_indev_get_point(lv_indev_get_act(), &p);
        phIdx = (phIdx + (p.x >= 240 ? 1 : phCount - 1)) % phCount;
        phDirty = true;
      },
      LV_EVENT_SHORT_CLICKED, nullptr);
  lv_obj_add_event_cb(phScr, [](lv_event_t *) { phExit(); }, LV_EVENT_LONG_PRESSED, nullptr);
  phHint = makeLabel(phScr, "", F_LABEL, C_TEXT);
  lv_label_set_text_fmt(phHint, isVi() ? "%d ảnh\n\nChạm phải / trái để chuyển ảnh\nChạm giữ để thoát"
                                       : "%d photos\n\nTap right / left for next / previous\nHold to leave",
                        phCount);
  lv_obj_set_style_text_align(phHint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(phHint);
  phHintUntil = millis() + 2500;
  phIdx = 0;
  phDirty = true;
  statusbarShow(false);
  lv_scr_load(phScr);
}
