#pragma once
// LVGL's heap functions (lv_conf.h). They are malloc/realloc, except that when the heap is exhausted while the radio
// plays, the radio is stopped to give its ~45 KB back and the allocation is tried again (main.cpp), instead of an
// LVGL assert restarting the board.
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
void *lvMemAlloc(size_t size);
void *lvMemRealloc(void *p, size_t size);
#ifdef __cplusplus
}
#endif
