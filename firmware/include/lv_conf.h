#if 1
#ifndef LV_CONF_H
#define LV_CONF_H
#include <stdint.h>
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0
#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
#define LV_MEM_CUSTOM_ALLOC malloc
#define LV_MEM_CUSTOM_FREE free
#define LV_MEM_CUSTOM_REALLOC realloc
#define LV_MEMCPY_MEMSET_STD 1       /* libc memcpy/memset (ROM, word-optimised) instead of LVGL's byte loops */
#define LV_DISP_DEF_REFR_PERIOD 20   /* up to 50 fps; frames are drawn and sent over DMA in parallel (main.cpp) */
#define LV_INDEV_DEF_READ_PERIOD 20  /* touch polled more often: swipes follow the finger more closely */
/* The screen background is a full-screen vertical gradient that every redrawn strip needs: cache the colour
   map (~1 KB each) instead of allocating and recomputing it for each 8-line strip. */
#define LV_GRAD_CACHE_DEF_SIZE 2048
/* Widgets with opa < 100% are composited in a temporary buffer; 24 KB (default) competes with TLS for the
   largest free block. 8 KB just renders big translucent widgets in more strips. */
#define LV_LAYER_SIMPLE_BUF_SIZE (8 * 1024)
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#define LV_USE_LOG 0
/* A failed assert (usually out of memory) restarts the device with a log line instead of hanging forever. */
#ifdef __cplusplus
extern "C" {
#endif
void lv_assert_fail(void);
#ifdef __cplusplus
}
#endif
#define LV_ASSERT_HANDLER lv_assert_fail();
#define LV_DRAW_COMPLEX 1
/* Hot drawing code (blending, image and font rendering) runs from IRAM instead of through the flash cache.
   Free IRAM cannot hold ordinary data on the ESP32, so this costs no heap. */
#include "esp_attr.h"
#define LV_ATTRIBUTE_FAST_MEM IRAM_ATTR
/* Shadow corner masks are expensive (a blur per strip, every frame of the control-center / toast slide).
   Cache one: corner = shadow width + radius, 44 px for the control center -> 48*48 = 2.3 KB. */
#define LV_SHADOW_CACHE_SIZE 48
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14
/* An infinite roller repeats its options this many times (default 7). 60 minutes x 37 px x 7 pages is past
   LVGL's 8191 px coordinate limit and draws nothing; 3 pages (6660 px) wraps 59 -> 00 -> 01 just as well. */
#define LV_ROLLER_INF_PAGES 3
#define LV_USE_ARC 1
#define LV_USE_SPINNER 1
#define LV_USE_LABEL 1
#define LV_USE_QRCODE 1
#endif
#endif
