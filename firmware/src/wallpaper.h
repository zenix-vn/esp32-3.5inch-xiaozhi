#pragma once
// Wallpaper for the main screen (home / QR / apps tiles). The web page crops and compresses the picture to
// 480x320 RGB565 in the browser and uploads it into the unused "spiffs" flash partition; the firmware maps
// that partition into memory and hands it to LVGL as an ordinary image, so it costs no RAM and draws as
// fast as the built-in sprites. Layout: 16-byte header ("ZWP1", u16 width, u16 height, 8 reserved) + pixels.
#include <esp_partition.h>
#include <esp_spi_flash.h>

static const uint32_t WP_W = 480, WP_H = 320, WP_HDR = 16, WP_SIZE = WP_HDR + WP_W * WP_H * 2;
static spi_flash_mmap_handle_t wpHandle;
static const uint8_t *wpPtr = nullptr;
static lv_img_dsc_t wpDsc;
static uint32_t wpWritten = 0;
static bool wpError = false;

static const esp_partition_t *wpPartition() {
  return esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, nullptr);
}

static void wallpaperOff() {
  if (mainScr) lv_obj_set_style_bg_img_src(mainScr, nullptr, 0);
  if (wpPtr) {
    spi_flash_munmap(wpHandle);
    wpPtr = nullptr;
  }
}

static bool wallpaperOn() {
  wallpaperOff();
  const esp_partition_t *p = wpPartition();
  if (!p || p->size < WP_SIZE) return false;
  const void *ptr;
  if (esp_partition_mmap(p, 0, WP_SIZE, SPI_FLASH_MMAP_DATA, &ptr, &wpHandle) != ESP_OK) return false;
  wpPtr = (const uint8_t *)ptr;
  uint16_t w = wpPtr[4] | (wpPtr[5] << 8), h = wpPtr[6] | (wpPtr[7] << 8);
  if (memcmp(wpPtr, "ZWP1", 4) != 0 || w != WP_W || h != WP_H) {
    wallpaperOff();
    return false;
  }
  memset(&wpDsc, 0, sizeof(wpDsc));
  wpDsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  wpDsc.header.w = w;
  wpDsc.header.h = h;
  wpDsc.data_size = WP_W * WP_H * 2;
  wpDsc.data = wpPtr + WP_HDR;
  lv_obj_set_style_bg_img_src(mainScr, &wpDsc, 0);
  lv_obj_set_style_bg_img_opa(mainScr, LV_OPA_COVER, 0);
  lv_obj_invalidate(mainScr);
  return true;
}

// Upload stream (called by the web server): erase, then write the chunks in order.
static void wallpaperWriteBegin() {
  wallpaperOff();
  wpWritten = 0;
  wpError = false;
  const esp_partition_t *p = wpPartition();
  uint32_t eraseLen = (WP_SIZE + 4095) & ~4095u;
  if (!p || p->size < eraseLen || esp_partition_erase_range(p, 0, eraseLen) != ESP_OK) wpError = true;
}
static void wallpaperWrite(const uint8_t *buf, size_t len) {
  if (wpError) return;
  if (wpWritten + len > WP_SIZE || esp_partition_write(wpPartition(), wpWritten, buf, len) != ESP_OK) {
    wpError = true;
    return;
  }
  wpWritten += len;
}
static bool wallpaperWriteEnd() { return !wpError && wpWritten == WP_SIZE && wallpaperOn(); }

// =====================================================================================
// Set the wallpaper from a JPG on the memory card (Settings > Wallpaper)
// =====================================================================================
// Pick from a grid of thumbnails -> full-screen preview (exactly what will be stored: cropped, and dimmed when
// the switch is on) -> "Set as wallpaper". The picture is decoded with TJpgDec (baseline JPEG only), centre-cropped
// to 480x320 and written into the wallpaper partition in 16-row bands (one flash write per band).
static uint16_t *wpBand = nullptr;  // 16 rows x 480 px being filled
static int wpBandY = -1;            // first destination row of the band (multiple of 16), -1 = none yet
static int wpOx, wpOy, wpPadX, wpPadY;
static bool wpBandDone[20], wpFlashErr;
static int wpDimK = 158;     // 0..256 of the brightness kept (158 = about 62%); 256 = unchanged
static bool wpDimPref = true;  // "Dim the picture so the text stays readable", remembered

static void wpDimLoad() {
  Preferences p;
  p.begin("ui", true);
  wpDimPref = p.getBool("wpdim", true);
  p.end();
}
static void wpDimSave() {
  Preferences p;
  p.begin("ui", false);
  p.putBool("wpdim", wpDimPref);
  p.end();
}

// Where a JPEG of w x h lands on the 480x320 screen: the biggest 1/1..1/8 reduction that still covers it, then a
// centred crop (or black bars if the picture is smaller). Offsets are multiples of 16 so bands line up.
static uint8_t wpGeometry(uint16_t w, uint16_t h) {
  uint8_t sc = 1;
  for (uint8_t t : {8, 4, 2})
    if (w / t >= 480 && h / t >= 320) {
      sc = t;
      break;
    }
  int sw = (w + sc - 1) / sc, sh = (h + sc - 1) / sc;
  wpOx = sw > 480 ? ((sw - 480) / 2) & ~15 : 0;
  wpOy = sh > 320 ? ((sh - 320) / 2) & ~15 : 0;
  wpPadX = sw < 480 ? (480 - sw) / 2 : 0;
  wpPadY = sh < 320 ? ((320 - sh) / 2) & ~15 : 0;
  return sc;
}

static inline uint16_t wpDimPx(uint16_t v) {
  uint32_t R = ((v >> 11) & 31) * wpDimK >> 8, G = ((v >> 5) & 63) * wpDimK >> 8, B = (v & 31) * wpDimK >> 8;
  return (R << 11) | (G << 5) | B;
}

static void wpFlushBand() {
  if (wpBandY < 0) return;
  int rows = 320 - wpBandY < 16 ? 320 - wpBandY : 16;
  if (esp_partition_write(wpPartition(), WP_HDR + (uint32_t)wpBandY * WP_W * 2, wpBand, rows * WP_W * 2) != ESP_OK) wpFlashErr = true;
  wpBandDone[wpBandY / 16] = true;
  memset(wpBand, 0, 16 * WP_W * 2);
  wpBandY = -1;
}

static bool wpOut(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bmp) {
  for (int r = 0; r < h; r++) {
    int dy = y + r - wpOy + wpPadY;
    if (dy < 0) continue;
    if (dy >= 320) return true;
    int band = dy & ~15;
    if (band != wpBandY) {
      wpFlushBand();
      wpBandY = band;
    }
    uint16_t *row = wpBand + (dy - band) * WP_W;
    for (int c = 0; c < w; c++) {
      int dx = x + c - wpOx + wpPadX;
      if (dx >= 0 && dx < (int)WP_W) row[dx] = wpDimPx(bmp[r * w + c]);
    }
  }
  return true;
}

// Returns nullptr on success, else a short reason.
static const char *wallpaperFromJpeg(const char *path, bool dim) {
  const esp_partition_t *part = wpPartition();
  uint16_t w = 0, h = 0;
  if (!part) return DL("Không có vùng lưu hình nền", "No wallpaper storage");
  if (TJpgDec.getSdJpgSize(&w, &h, path) != JDR_OK || !w || !h) return DL("Không đọc được ảnh (cần JPG baseline)", "Cannot read the picture (baseline JPG needed)");
  uint8_t sc = wpGeometry(w, h);
  wpDimK = dim ? 158 : 256;

  wpBand = (uint16_t *)calloc(16 * WP_W, 2);
  if (!wpBand) return DL("Không đủ bộ nhớ", "Out of memory");
  wallpaperWriteBegin();  // unmaps the old picture and erases the partition
  if (wpError) {
    free(wpBand);
    wpBand = nullptr;
    return DL("Không xoá được vùng hình nền", "Cannot erase the wallpaper area");
  }
  uint8_t hdr[WP_HDR] = {'Z', 'W', 'P', '1', 480 & 255, 480 >> 8, 320 & 255, 320 >> 8};
  wpFlashErr = esp_partition_write(part, 0, hdr, WP_HDR) != ESP_OK;
  memset(wpBandDone, 0, sizeof(wpBandDone));
  wpBandY = -1;
  tftIdle();  // the display's DMA must not run while the SD decoder works
  TJpgDec.setJpgScale(sc);
  TJpgDec.setCallback(wpOut);
  JRESULT jr = TJpgDec.drawSdJpg(0, 0, path);
  wpFlushBand();
  memset(wpBand, 0, 16 * WP_W * 2);
  for (int b = 0; b < 20 && !wpFlashErr; b++)  // rows the picture did not reach (letterbox): black
    if (!wpBandDone[b] && esp_partition_write(part, WP_HDR + (uint32_t)b * 16 * WP_W * 2, wpBand, 16 * WP_W * 2) != ESP_OK) wpFlashErr = true;
  free(wpBand);
  wpBand = nullptr;
  if (jr != JDR_OK) return DL("Ảnh lỗi hoặc không phải JPG baseline", "Broken picture or not a baseline JPG");
  if (wpFlashErr) return DL("Lỗi ghi flash", "Flash write failed");
  wpWritten = WP_SIZE;
  return wallpaperOn() ? nullptr : DL("Không hiển thị được hình nền", "Cannot show the wallpaper");
}

// ---------- thumbnails: 96x64, 8-bit with a fixed 3-3-2 palette (7 KB each instead of 12 KB) ----------
static const int TH_W = 96, TH_H = 64, TH_PER_PAGE = 8;
struct Thumb {
  lv_img_dsc_t dsc;
  uint8_t *data;  // 256-entry palette (1 KB) followed by the pixels
};
static Thumb wpThumbs[TH_PER_PAGE];
static uint16_t wpThumbWH[TH_PER_PAGE][2];  // picture size, shown under the name
static lv_obj_t *wpThumbImg[TH_PER_PAGE], *wpThumbSub[TH_PER_PAGE];
static int wpPage = 0, wpLoadIdx = 0;
static int thCx0, thCy0, thCw, thCh;  // crop of the (reduced) picture that fills the thumbnail
static uint8_t *thPix;
static const uint8_t BAYER4[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

static bool thOut(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bmp) {
  for (int r = 0; r < h; r++) {
    int sy = y + r - thCy0;
    if (sy < 0 || sy >= thCh) continue;
    int ty0 = sy * TH_H / thCh, ty1 = (sy + 1) * TH_H / thCh;
    if (ty1 <= ty0) ty1 = ty0 + 1;
    for (int c = 0; c < w; c++) {
      int sx = x + c - thCx0;
      if (sx < 0 || sx >= thCw) continue;
      int tx0 = sx * TH_W / thCw, tx1 = (sx + 1) * TH_W / thCw;
      if (tx1 <= tx0) tx1 = tx0 + 1;
      uint16_t v = bmp[r * w + c];
      int R = ((v >> 11) & 31) << 3, G = ((v >> 5) & 63) << 2, B = (v & 31) << 3;
      for (int ty = ty0; ty < ty1 && ty < TH_H; ty++)
        for (int tx = tx0; tx < tx1 && tx < TH_W; tx++) {
          int d = (BAYER4[(ty & 3) * 4 + (tx & 3)] - 8) * 2;  // ordered dithering hides the 3-3-2 banding
          int r3 = (R + d) < 0 ? 0 : (R + d) > 255 ? 7 : (R + d) >> 5, g3 = (G + d) < 0 ? 0 : (G + d) > 255 ? 7 : (G + d) >> 5,
              b2 = (B + d) < 0 ? 0 : (B + d) > 255 ? 3 : (B + d) >> 6;
          thPix[ty * TH_W + tx] = (r3 << 5) | (g3 << 2) | b2;
        }
    }
  }
  return true;
}

static void wpThumbFree() {
  for (int i = 0; i < TH_PER_PAGE; i++) {
    free(wpThumbs[i].data);
    wpThumbs[i].data = nullptr;
  }
}

// Decode picture `path` into thumbnail slot i; false = the picture cannot be read (left as a grey card).
static bool wpThumbMake(int i, const char *path) {
  uint16_t w = 0, h = 0;
  if (TJpgDec.getSdJpgSize(&w, &h, path) != JDR_OK || !w || !h) return false;
  wpThumbWH[i][0] = w;
  wpThumbWH[i][1] = h;
  uint8_t sc = 1;
  for (uint8_t t : {8, 4, 2})
    if (w / t >= TH_W && h / t >= TH_H) {
      sc = t;
      break;
    }
  int sw = (w + sc - 1) / sc, sh = (h + sc - 1) / sc;
  thCw = sw < sh * TH_W / TH_H ? sw : sh * TH_W / TH_H;  // largest 3:2 crop
  thCh = thCw * TH_H / TH_W;
  thCx0 = (sw - thCw) / 2;
  thCy0 = (sh - thCh) / 2;
  uint8_t *buf = (uint8_t *)heap_caps_malloc(1024 + TH_W * TH_H, MALLOC_CAP_8BIT);
  if (!buf) return false;
  for (int k = 0; k < 256; k++) {  // palette: index = rrrgggbb
    lv_color32_t *e = (lv_color32_t *)(buf + k * 4);
    e->ch.red = ((k >> 5) & 7) * 255 / 7;
    e->ch.green = ((k >> 2) & 7) * 255 / 7;
    e->ch.blue = (k & 3) * 255 / 3;
    e->ch.alpha = 255;
  }
  thPix = buf + 1024;
  memset(thPix, 0, TH_W * TH_H);
  tftIdle();
  TJpgDec.setJpgScale(sc);
  TJpgDec.setCallback(thOut);
  if (TJpgDec.drawSdJpg(0, 0, path) != JDR_OK) {
    free(buf);
    return false;
  }
  wpThumbs[i].data = buf;
  memset(&wpThumbs[i].dsc, 0, sizeof(lv_img_dsc_t));
  wpThumbs[i].dsc.header.cf = LV_IMG_CF_INDEXED_8BIT;
  wpThumbs[i].dsc.header.w = TH_W;
  wpThumbs[i].dsc.header.h = TH_H;
  wpThumbs[i].dsc.data_size = 1024 + TH_W * TH_H;
  wpThumbs[i].dsc.data = buf;
  return true;
}

// ---------- full-screen preview (drawn straight to the panel like the photo frame) ----------
static const int PV_BAR_Y = 268;  // the control bar covers y >= 268; the picture is drawn above it
static lv_obj_t *pvScr = nullptr, *pvSwitch;
static String pvPath;
static bool pvDirty = false, pvFail = false;
static uint32_t pvDrawnFlush = 0;
static void pageWallpaper();

static bool pvOut(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bmp) {
  int dx0 = x - wpOx + wpPadX, dy0 = y - wpOy + wpPadY;
  int cx0 = dx0 < 0 ? -dx0 : 0, cy0 = dy0 < 0 ? -dy0 : 0;
  int cx1 = dx0 + w > 480 ? 480 - dx0 : w, cy1 = dy0 + h > PV_BAR_Y ? PV_BAR_Y - dy0 : h;
  if (cx0 >= cx1 || cy0 >= cy1) return true;
  uint16_t tmp[16 * 16];
  int vw = cx1 - cx0, vh = cy1 - cy0;
  for (int r = 0; r < vh; r++)
    for (int c = 0; c < vw; c++) tmp[r * vw + c] = wpDimPx(bmp[(cy0 + r) * w + cx0 + c]);
  tft.pushImage(dx0 + cx0, dy0 + cy0, vw, vh, tmp);
  return true;
}

static void pvDraw() {
  tftIdle();
  tft.fillRect(0, 0, 480, PV_BAR_Y, TFT_BLACK);
  uint16_t w = 0, h = 0;
  pvFail = TJpgDec.getSdJpgSize(&w, &h, pvPath.c_str()) != JDR_OK || !w || !h;
  if (!pvFail) {
    uint8_t sc = wpGeometry(w, h);
    wpDimK = wpDimPref ? 158 : 256;
    TJpgDec.setJpgScale(sc);
    TJpgDec.setCallback(pvOut);
    tft.setSwapBytes(true);
    pvFail = TJpgDec.drawSdJpg(0, 0, pvPath.c_str()) != JDR_OK;
    tft.setSwapBytes(false);
  }
  if (pvFail) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(isVi() ? "Khong doc duoc anh (can JPG baseline)" : "Cannot read the picture (baseline JPG needed)", 24, 120, 2);
  }
}

// Called from loop(): the picture is drawn once LVGL has painted the screen, and again if LVGL paints over it.
static void wpPreviewLoop() {
  if (!pvScr || lv_scr_act() != pvScr) return;
  if (gFlushTop != pvDrawnFlush) pvDirty = true;
  if (!pvDirty) return;
  pvDirty = false;
  pvDraw();
  pvDrawnFlush = gFlushTop;
}

static void wpApply() {
  lv_obj_t *ov = lv_obj_create(lv_layer_top());  // busy card: decoding and flashing take a few seconds
  lv_obj_remove_style_all(ov);
  lv_obj_set_size(ov, 480, 320);
  lv_obj_set_style_bg_color(ov, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(ov, LV_OPA_COVER, 0);
  lv_obj_add_flag(ov, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *l = makeLabel(ov, DL("Đang đặt hình nền...", "Setting the wallpaper..."), F_TITLE, C_TEXT);
  lv_obj_center(l);
  lv_refr_now(nullptr);
  const char *err = wallpaperFromJpeg(pvPath.c_str(), wpDimPref);
  lv_obj_del(ov);
  cfg.wallOn = err ? wallpaperOn() : true;  // after a failed erase the old picture is gone: keep the flag honest
  cfgSave();
  if (err) {
    uiToast(err, C_DANGER);
    pvDirty = true;
    return;
  }
  statusbarShow(true);
  goMain();  // straight to the main screen to see it
  uiToast(DL("Đã đặt hình nền", "Wallpaper set"), C_OK);
}

static void wpPreviewOpen(const char *path) {
  pvPath = path;
  pvScr = lv_obj_create(nullptr);
  lv_obj_remove_style_all(pvScr);
  lv_obj_set_style_bg_color(pvScr, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(pvScr, LV_OPA_COVER, 0);
  lv_obj_clear_flag(pvScr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(pvScr, [](lv_event_t *) {
    pvScr = nullptr;
    statusbarShow(true);
  }, LV_EVENT_DELETE, nullptr);

  lv_obj_t *bar = lv_obj_create(pvScr);  // opaque: LVGL does not know what is under it (the picture is drawn behind its back)
  lv_obj_remove_style_all(bar);
  lv_obj_set_size(bar, 480, 320 - PV_BAR_Y);
  lv_obj_set_pos(bar, 0, PV_BAR_Y);
  lv_obj_set_style_bg_color(bar, lv_color_hex(0x0B1024), 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  pvSwitch = lv_switch_create(bar);
  lv_obj_set_size(pvSwitch, 46, 26);
  lv_obj_align(pvSwitch, LV_ALIGN_LEFT_MID, 12, 0);
  lv_obj_set_style_bg_color(pvSwitch, C_BORDER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(pvSwitch, C_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
  if (wpDimPref) lv_obj_add_state(pvSwitch, LV_STATE_CHECKED);
  lv_obj_add_event_cb(pvSwitch, [](lv_event_t *e) {
    wpDimPref = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    wpDimSave();
    pvDirty = true;  // redraw the picture with / without the dimming
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_t *dl = makeLabel(bar, DL("Làm tối ảnh để chữ dễ đọc", "Dim the picture so text stays readable"), F_CAPTION, C_TEXT);
  lv_obj_set_width(dl, 150);
  lv_label_set_long_mode(dl, LV_LABEL_LONG_WRAP);
  lv_obj_align(dl, LV_ALIGN_LEFT_MID, 66, 0);
  lv_obj_t *cancel = smallBtn(bar, DL("Huỷ", "Cancel"), C_MUTED, [](lv_event_t *) {
    pageReplace = true;  // back to the thumbnails (the preview is replaced)
    pageWallpaper();
    pageReplace = false;
  }, nullptr, 84);
  lv_obj_align(cancel, LV_ALIGN_RIGHT_MID, -138, 0);
  lv_obj_t *ok = smallBtn(bar, DL("Đặt hình nền", "Set wallpaper"), C_OK, [](lv_event_t *) { wpApply(); }, nullptr, 124);
  lv_obj_align(ok, LV_ALIGN_RIGHT_MID, -8, 0);
  lv_obj_set_height(cancel, 38);
  lv_obj_set_height(ok, 38);

  statusbarShow(false);
  pvDirty = true;
  pvDrawnFlush = gFlushTop;
  lv_scr_load_anim(pvScr, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);  // deletes the thumbnail page
}

// ---------- the thumbnail picker ----------
static lv_obj_t *wpPageLbl, *wpPickScr;

static void wpLoadTick(lv_timer_t *) {  // one thumbnail per tick: the page appears at once, pictures fill in
  int base = wpPage * TH_PER_PAGE;
  while (wpLoadIdx < TH_PER_PAGE && base + wpLoadIdx >= phCount) wpLoadIdx++;
  if (wpLoadIdx >= TH_PER_PAGE) return;
  int i = wpLoadIdx++;
  if (wpThumbMake(i, phFiles[base + i].c_str())) {
    lv_img_set_src(wpThumbImg[i], &wpThumbs[i].dsc);
    lv_obj_t *sub = wpThumbSub[i];
    lv_label_set_text_fmt(sub, "%ux%u", wpThumbWH[i][0], wpThumbWH[i][1]);
  } else {
    lv_label_set_text(wpThumbSub[i], DL("không đọc được", "unreadable"));
  }
}

static void wpShowPage(int page);

static void pageWallpaper() {
  wpDimLoad();
  lv_obj_t *c;
  wpPickScr = pageBegin(DL("Chọn hình nền", "Choose a wallpaper"), &c);
  lv_obj_del(c);
  lv_obj_t *s = wpPickScr;
  lv_obj_add_event_cb(s, [](lv_event_t *) {
    wpThumbFree();
    wpPickScr = nullptr;
  }, LV_EVENT_DELETE, nullptr);
  if (sdOk) phScan();
  if (!sdOk || !phCount) {
    lv_obj_t *card = makeCard(s, 456, LV_SIZE_CONTENT);
    lv_obj_set_pos(card, 12, SB_H + 56);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_t *t = makeLabel(card, "", F_BODY, C_WARN);
    lv_obj_set_width(t, 428);
    lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
    lv_label_set_text(t, !sdOk ? DL("Cần thẻ nhớ microSD.", "A microSD card is required.")
                               : DL("Không thấy ảnh JPG nào trên thẻ (thư mục /photos, /photo hoặc thư mục gốc).", "No JPG pictures found on the card (/photos, /photo or the root)."));
    pageShow(s);
    return;
  }
  lv_obj_set_width(lv_obj_get_child(s, 1), 190);  // the title shares the row with the pager
  int pages = (phCount + TH_PER_PAGE - 1) / TH_PER_PAGE;
  if (wpPage >= pages) wpPage = pages - 1;
  if (pages > 1) {  // pager: soft arrows around "1/3"
    auto arrow = [&](const char *sym, int x, int delta) {
      lv_obj_t *b = lv_obj_create(s);
      lv_obj_remove_style_all(b);
      lv_obj_set_size(b, 40, 38);
      lv_obj_set_pos(b, x, SB_H + 6);
      lv_obj_set_style_radius(b, 19, 0);
      lv_obj_set_style_bg_color(b, C_ACCENT, 0);
      lv_obj_set_style_bg_opa(b, LV_OPA_30, LV_STATE_PRESSED);
      lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_t *l = makeLabel(b, sym, F_LABEL, C_ACCENT);
      lv_obj_center(l);
      lv_obj_add_event_cb(b, [](lv_event_t *e) {
        int p = wpPage + (int)(intptr_t)lv_event_get_user_data(e), n = (phCount + TH_PER_PAGE - 1) / TH_PER_PAGE;
        if (p >= 0 && p < n) wpShowPage(p);
      }, LV_EVENT_CLICKED, (void *)(intptr_t)delta);
    };
    arrow(LV_SYMBOL_LEFT, 280, -1);
    arrow(LV_SYMBOL_RIGHT, 428, 1);
    wpPageLbl = makeLabel(s, "", F_LABEL, C_TEXT);
    lv_obj_set_width(wpPageLbl, 68);
    lv_obj_set_style_text_align(wpPageLbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(wpPageLbl, 346, SB_H + 14);
  } else {
    wpPageLbl = nullptr;
  }
  lv_obj_t *grid = lv_obj_create(s);
  lv_obj_remove_style_all(grid);
  lv_obj_set_size(grid, 456, 232);
  lv_obj_set_pos(grid, 12, SB_H + 50);
  lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
  for (int i = 0; i < TH_PER_PAGE; i++) {
    lv_obj_t *card = makeCard(grid, 108, 112);
    lv_obj_set_pos(card, (i % 4) * 116, (i / 4) * 120);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_border_color(card, C_ACCENT, LV_STATE_PRESSED);
    lv_obj_t *img = lv_img_create(card);
    lv_obj_set_pos(img, 5, 5);
    lv_obj_set_size(img, TH_W, TH_H);
    lv_obj_set_style_bg_color(img, lv_color_hex(0x1F2747), 0);
    lv_obj_set_style_bg_opa(img, LV_OPA_COVER, 0);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(img, LV_OBJ_FLAG_EVENT_BUBBLE);
    wpThumbImg[i] = img;
    lv_obj_t *nm = makeLabel(card, "", F_CAPTION, C_TEXT);
    lv_obj_set_size(nm, 98, 16);
    lv_label_set_long_mode(nm, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(nm, 5, 74);
    lv_obj_t *sub = makeLabel(card, "", F_CAPTION, C_MUTED);
    lv_obj_set_size(sub, 98, 16);
    lv_label_set_long_mode(sub, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(sub, 5, 91);
    wpThumbSub[i] = sub;
    lv_obj_add_event_cb(card, [](lv_event_t *e) {
      int i = (int)(intptr_t)lv_event_get_user_data(e), idx = wpPage * TH_PER_PAGE + i;
      if (idx < phCount) wpPreviewOpen(phFiles[idx].c_str());
    }, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    wpThumbs[i].data = nullptr;
    (void)nm;
  }
  wpShowPage(wpPage);
  lv_timer_t *t = lv_timer_create(wpLoadTick, 30, nullptr);
  lv_obj_add_event_cb(s, [](lv_event_t *e) { lv_timer_del((lv_timer_t *)lv_event_get_user_data(e)); }, LV_EVENT_DELETE, t);
  pageShow(s);
}

// Fill the 8 cards with page `page`'s names; thumbnails then load one by one.
static void wpShowPage(int page) {
  wpPage = page;
  wpThumbFree();
  wpLoadIdx = 0;
  lv_obj_t *grid = lv_obj_get_child(wpPickScr, -1);
  for (int i = 0; i < TH_PER_PAGE; i++) {
    lv_obj_t *card = lv_obj_get_child(grid, i);
    int idx = page * TH_PER_PAGE + i;
    if (idx >= phCount) {
      lv_obj_add_flag(card, LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    lv_obj_clear_flag(card, LV_OBJ_FLAG_HIDDEN);
    lv_img_set_src(wpThumbImg[i], nullptr);
    String nm = phFiles[idx];
    int sl = nm.lastIndexOf('/');
    lv_label_set_text(lv_obj_get_child(card, 1), (sl >= 0 ? nm.substring(sl + 1) : nm).c_str());
    lv_label_set_text(wpThumbSub[i], "...");
  }
  if (wpPageLbl) lv_label_set_text_fmt(wpPageLbl, "%d/%d", page + 1, (phCount + TH_PER_PAGE - 1) / TH_PER_PAGE);
}
