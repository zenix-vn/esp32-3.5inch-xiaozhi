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
