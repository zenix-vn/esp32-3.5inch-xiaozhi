#pragma once
// Sleep sounds: looping rain, stream, wind, storm, ocean, night forest, campfire, a music-box lullaby and white
// noise, played from /sounds on the memory card (tools/gen_sounds.py makes them) with a timer that fades the sound
// out. The sound keeps playing when the page is closed; the interface sounds are mixed over it (audio.h).

struct SleepSnd {
  const char *file, *vi, *en;
  uint32_t col;
};
static const SleepSnd SLEEP_SND[] = {
    {"rain", "Mưa rơi", "Rain", 0x3C7BD9},         {"stream", "Suối chảy", "Stream", 0x1FA39A},   {"wind", "Gió", "Wind", 0x7B8AAE},
    {"storm", "Bão", "Storm", 0x7155D6},           {"ocean", "Sóng biển", "Ocean", 0x2F6FD0},     {"forest", "Rừng đêm", "Night forest", 0x38A05A},
    {"fire_camp", "Lửa trại", "Campfire", 0xE0702A}, {"lullaby", "Nhạc ru", "Lullaby", 0xD0689A}, {"whitenoise", "Ồn trắng", "White noise", 0x8E98B8},
};
static const int SLEEP_N = sizeof(SLEEP_SND) / sizeof(SLEEP_SND[0]);
static const uint8_t SLEEP_MIN[5] = {15, 30, 60, 90, 0};  // timer choices in minutes, 0 = until stopped
static int sleepSel = 1;                                   // index into SLEEP_MIN
static lv_obj_t *slCard[SLEEP_N], *slLeft;

static void sleepPrefLoad() {
  Preferences p;
  p.begin("ui", true);
  sleepSel = constrain(p.getUChar("slp", 1), 0, 4);
  p.end();
}

// Cards and the "time left" text follow what the audio task is really playing.
static void sleepRefresh(lv_timer_t * = nullptr) {
  const char *now = audioBgPlaying();
  for (int i = 0; i < SLEEP_N; i++) {
    bool on = now && !strcmp(now, SLEEP_SND[i].file);
    lv_obj_set_style_bg_opa(slCard[i], on ? LV_OPA_60 : LV_OPA_20, 0);
    lv_obj_set_style_border_width(slCard[i], on ? 2 : 1, 0);
    lv_obj_set_style_border_color(slCard[i], on ? C_TEXT : lv_color_hex(SLEEP_SND[i].col), 0);
    setText(lv_obj_get_child(lv_obj_get_child(slCard[i], 0), 0), on ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
  }
  uint32_t left = audioBgLeftSec();
  if (now && left) setTextFmt(slLeft, isVi() ? "Còn %02u:%02u" : "%02u:%02u left", (unsigned)(left / 60), (unsigned)(left % 60));
  else if (now) setText(slLeft, DL("Phát liên tục", "Playing"));
  else setText(slLeft, "");
}

static void sleepCardTap(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  const char *now = audioBgPlaying();
  if (now && !strcmp(now, SLEEP_SND[i].file)) {
    audioBgStopNow();
  } else if (cfg.volume == 0) {
    uiToast(DL("Âm lượng đang tắt: kéo thanh âm lượng ở Trung tâm điều khiển", "Volume is off: raise it in the control center"), C_WARN);
    return;
  } else if (!audioBg(SLEEP_SND[i].file, SLEEP_MIN[sleepSel])) {
    char b[80];
    snprintf(b, sizeof(b), isVi() ? "Chưa có /sounds/%s.wav trên thẻ nhớ" : "/sounds/%s.wav is not on the card", SLEEP_SND[i].file);
    uiToast(b, C_WARN);
    return;
  }
  soundPlay(SND(SND_TAP), 0, false);
  sleepRefresh();
}

static void pageSleep() {
  sleepPrefLoad();
  lv_obj_t *c;
  lv_obj_t *s = pageBegin(DL("Ru ngủ", "Sleep sounds"), &c);
  lv_obj_del(c);
  // 3 x 3 cards
  lv_obj_t *grid = lv_obj_create(s);
  lv_obj_remove_style_all(grid);
  lv_obj_set_size(grid, 456, 3 * 52 + 2 * 6);
  lv_obj_set_pos(grid, 12, SB_H + 50);
  lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
  for (int i = 0; i < SLEEP_N; i++) {
    uint32_t col = SLEEP_SND[i].col;
    lv_obj_t *card = lv_obj_create(grid);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, 146, 52);
    lv_obj_set_pos(card, (i % 3) * 155, (i / 3) * 58);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(col), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_50, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(card, lv_color_hex(col), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *dot = shape(card, 10, 10, 32, 32, col);
    lv_obj_t *sym = makeLabel(dot, LV_SYMBOL_PLAY, F_BODY, lv_color_hex(0xFFFFFF));
    lv_obj_center(sym);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_t *nm = makeLabel(card, "", F_BODY, C_TEXT);
    lv_obj_set_size(nm, 92, 20);
    lv_label_set_long_mode(nm, LV_LABEL_LONG_DOT);
    lv_label_set_text(nm, isVi() ? SLEEP_SND[i].vi : SLEEP_SND[i].en);
    lv_obj_align(nm, LV_ALIGN_LEFT_MID, 50, 0);
    lv_obj_add_event_cb(card, sleepCardTap, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    slCard[i] = card;
  }

  // timer row: how long it plays before fading out
  lv_obj_t *tl = makeLabel(s, "", F_CAPTION, C_MUTED);
  lv_label_set_text(tl, DL("TỰ TẮT SAU (PHÚT)", "FADE OUT AFTER (MIN)"));
  lv_obj_set_pos(tl, 16, SB_H + 50 + 3 * 58 + 6);
  static const char *timerMap[6];
  timerMap[0] = "15";
  timerMap[1] = "30";
  timerMap[2] = "60";
  timerMap[3] = "90";
  timerMap[4] = DL("Mãi", "Always");
  timerMap[5] = "";
  lv_obj_t *chips = ckChips(s, timerMap, true, [](lv_event_t *e) {
    uint32_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
    if (id > 4) return;
    sleepSel = id;
    Preferences p;
    p.begin("ui", false);
    p.putUChar("slp", id);
    p.end();
    if (audioBgPlaying()) aBgEndsAt = SLEEP_MIN[id] ? millis() + SLEEP_MIN[id] * 60000UL : 0;  // applies to what is playing now
  });
  lv_btnmatrix_set_btn_ctrl(chips, sleepSel, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_set_size(chips, 290, 34);
  lv_obj_set_pos(chips, 12, SB_H + 50 + 3 * 58 + 26);
  slLeft = makeLabel(s, "", F_BODY, C_ACCENT);
  lv_obj_set_size(slLeft, 90, 20);
  lv_label_set_long_mode(slLeft, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(slLeft, 312, SB_H + 50 + 3 * 58 + 33);
  lv_obj_t *stop = smallBtn(s, DL("Dừng", "Stop"), C_DANGER, [](lv_event_t *) {
    audioBgStopNow();
    sleepRefresh();
  }, nullptr, 62);
  lv_obj_set_pos(stop, 406, SB_H + 50 + 3 * 58 + 26);

  sleepRefresh();
  pageTimer(s, sleepRefresh, 500);
  pageShow(s);
}
