#pragma once
// "Voi học chữ": the 29 letters of the Vietnamese alphabet for a 5-year-old, in three games:
//   - Học chữ: one big letter at a time (capital + small) with a word that starts with it; arrows or a swipe go on.
//   - Tìm chữ: find the letter shown among four, with look-alikes mixed in (b d đ p q, o ô ơ, a ă â, u ư...).
//   - Chữ hoa - chữ thường: a capital is shown, find its small letter.
// A right answer the first time earns a diamond (kept in NVS "kids"); ten questions make a round. Opening the app
// says the welcome (chao_mung). Big letters use
// kids_font (Nunito ExtraBold 88 px, tools/gen_font.swift). Voice comes from the memory card when the files are there
// (tools/gen_letter_voice.py makes them): hoc_<x> ("bờ... bé") in Học chữ when a letter is shown or tapped; in the
// quizzes tim_<x> / hoa_<x> ask the question, sai_<x> names a wrong pick, dung_1..3 praise a right one and ket_1..3
// end the round. <x> is Telex style: aw = ă, dd = đ... Without the files the games still work, with beeps.
// Included by main.cpp after clock_app.h and led.h.

struct KLetter {
  const char *lo, *up, *word, *snd;
};
static const KLetter KL[] = {
    {"a", "A", "áo", "chu_a"},     {"ă", "Ă", "ăn", "chu_aw"},   {"â", "Â", "ấm", "chu_aa"},   {"b", "B", "bé", "chu_b"},
    {"c", "C", "cá", "chu_c"},     {"d", "D", "dê", "chu_d"},    {"đ", "Đ", "đèn", "chu_dd"},  {"e", "E", "em", "chu_e"},
    {"ê", "Ê", "ếch", "chu_ee"},   {"g", "G", "gà", "chu_g"},    {"h", "H", "hoa", "chu_h"},   {"i", "I", "bi", "chu_i"},
    {"k", "K", "kẹo", "chu_k"},    {"l", "L", "lá", "chu_l"},    {"m", "M", "mèo", "chu_m"},   {"n", "N", "nơ", "chu_n"},
    {"o", "O", "ong", "chu_o"},    {"ô", "Ô", "ô tô", "chu_oo"}, {"ơ", "Ơ", "ớt", "chu_ow"},   {"p", "P", "pin", "chu_p"},
    {"q", "Q", "quả", "chu_q"},    {"r", "R", "rùa", "chu_r"},   {"s", "S", "sao", "chu_s"},   {"t", "T", "táo", "chu_t"},
    {"u", "U", "ủng", "chu_u"},    {"ư", "Ư", "sư tử", "chu_uw"}, {"v", "V", "voi", "chu_v"},  {"x", "X", "xe", "chu_x"},
    {"y", "Y", "y tá", "chu_y"},
};
static const int KL_N = sizeof(KL) / sizeof(KL[0]);
// letters a child mixes up, by index into KL
static const int8_t KL_LOOKALIKE[][6] = {
    {0, 1, 2, -1}, {16, 17, 18, -1}, {24, 25, -1}, {7, 8, -1}, {3, 5, 6, 19, 20, -1}, {14, 15, -1},
    {13, 23, 11, 12, 10, -1}, {4, 7, 16, -1}, {9, 20, 28, -1}, {22, 27, 26, -1}, {21, 15, -1},
};
static const uint32_t KL_COL[] = {0xFF6B8B, 0xFF9F43, 0xF7C531, 0x4CD27A, 0x2EC4D6, 0x4C8BF5, 0x9B6BFF, 0xE05BC8};

enum { KM_MENU, KM_LEARN, KM_FIND, KM_CASE };
static lv_obj_t *kScr, *kBody, *kGemLbl;
static int kMode = KM_MENU, kLearnIdx = 0;
static int kGems = 0;  // diamonds earned so far
// quiz state
static int kQ = 0, kTarget = 0, kOpt[4], kRoundGems = 0;
static bool kMissed = false;
static uint32_t kNextAt = 0;  // millis() of the next question (after a right answer), 0 = none pending
static lv_obj_t *kOptBtn[4], *kDots[10];

static void kGemsLoad() {
  Preferences p;
  p.begin("kids", true);
  kGems = p.getUInt("stars", 0);  // key from when they were stars
  p.end();
}
static void kGemsAdd(int n) {
  kGems += n;
  Preferences p;
  p.begin("kids", false);
  p.putUInt("stars", kGems);
  p.end();
  if (kGemLbl) setTextFmt(kGemLbl, "%d", kGems);
}

// The letter's recorded name ("chu_b"), or with its example word in Học chữ ("hoc_b"), if the card has the file.
static bool kVoice(const char *f) { return cfg.volume && audioHas(f) && audioFx(f, false); }
static bool kVoiceFor(const char *prefix, int i) {  // prefix + the letter's Telex name: "tim_b", "sai_dd"...
  char f[16];
  snprintf(f, sizeof(f), "%s%s", prefix, KL[i].snd + 4);
  return kVoice(f);
}
static bool kSay(int i, bool withWord = false) {
  char f[16];
  snprintf(f, sizeof(f), "%s%s", withWord ? "hoc" : "chu", KL[i].snd + 3);
  if (!cfg.volume) return false;
  if (audioHas(f)) return audioFx(f, false);
  return withWord && audioHas(KL[i].snd) && audioFx(KL[i].snd, false);
}

// the question of the current quiz, said again when the child taps the letter on the left
static void kAsk() {
  if (!kVoiceFor(kMode == KM_FIND ? "tim_" : "hoa_", kTarget)) kSay(kTarget);
}

// A cut diamond filling the object: a crown (trapezoid) over a pavilion (triangle), each with a lighter middle facet
// and a glint. Convex pieces only: LVGL draws convex polygons. lit = false draws an empty, dark one.
static void kGemDraw(lv_event_t *e) {
  lv_obj_t *o = lv_event_get_target(e);
  lv_area_t a;
  lv_obj_get_coords(o, &a);
  bool lit = (bool)(uintptr_t)lv_event_get_user_data(e);
  float x = a.x1, y = a.y1, w = a.x2 - a.x1 + 1, h = a.y2 - a.y1 + 1;
  auto P = [&](float fx, float fy) { return lv_point_t{(lv_coord_t)(x + fx * w), (lv_coord_t)(y + fy * h)}; };
  lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);
  lv_draw_rect_dsc_t d;
  lv_draw_rect_dsc_init(&d);
  auto poly = [&](uint32_t col, std::initializer_list<lv_point_t> pts) {
    lv_point_t p[4];
    int n = 0;
    for (auto q : pts) p[n++] = q;
    d.bg_color = lv_color_hex(col);
    lv_draw_polygon(ctx, &d, p, n);
  };
  const float top = 0.14f, mid = 0.40f, tip = 0.92f;
  poly(lit ? 0x2BB8E8 : 0x232B4D, {P(0.04f, mid), P(0.96f, mid), P(0.5f, tip)});               // pavilion
  poly(lit ? 0x7FE0FF : 0x2E3860, {P(0.32f, mid), P(0.68f, mid), P(0.5f, tip)});               // its middle facet
  poly(lit ? 0xA8EEFF : 0x343F6B, {P(0.28f, top), P(0.72f, top), P(0.96f, mid), P(0.04f, mid)});  // crown
  poly(lit ? 0xE4FAFF : 0x3C4878, {P(0.40f, top), P(0.60f, top), P(0.68f, mid), P(0.32f, mid)});  // table
  if (lit) {
    d.bg_color = lv_color_hex(0xFFFFFF);  // glint
    d.radius = LV_RADIUS_CIRCLE;
    lv_area_t g = {(lv_coord_t)(x + 0.24f * w), (lv_coord_t)(y + 0.22f * h), (lv_coord_t)(x + 0.24f * w + w / 9), (lv_coord_t)(y + 0.22f * h + w / 9)};
    lv_draw_rect(ctx, &d, &g);
  }
}
static lv_obj_t *kGem(lv_obj_t *parent, int size, bool lit) {
  lv_obj_t *o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_set_size(o, size, size);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(o, kGemDraw, LV_EVENT_DRAW_MAIN, (void *)(uintptr_t)lit);
  return o;
}

// A rounded, coloured tile; clickable when cb is given.
static lv_obj_t *kTile(lv_obj_t *parent, int x, int y, int w, int h, uint32_t col, lv_event_cb_t cb, void *ud = nullptr) {
  lv_obj_t *t = lv_obj_create(parent);
  lv_obj_remove_style_all(t);
  lv_obj_set_size(t, w, h);
  lv_obj_set_pos(t, x, y);
  lv_obj_set_style_radius(t, 22, 0);
  lv_obj_set_style_bg_color(t, lv_color_hex(col), 0);
  lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_grad_color(t, lv_color_darken(lv_color_hex(col), LV_OPA_20), 0);
  lv_obj_set_style_bg_grad_dir(t, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_border_color(t, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(t, 3, 0);
  lv_obj_set_style_border_opa(t, LV_OPA_40, 0);
  lv_obj_set_style_transform_width(t, -4, LV_STATE_PRESSED);  // squashes a little when pressed
  lv_obj_set_style_transform_height(t, -4, LV_STATE_PRESSED);
  lv_obj_clear_flag(t, LV_OBJ_FLAG_SCROLLABLE);
  if (cb) {
    lv_obj_add_flag(t, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(t, cb, LV_EVENT_CLICKED, ud);
  } else {
    lv_obj_clear_flag(t, LV_OBJ_FLAG_CLICKABLE);
  }
  return t;
}
static lv_obj_t *kBig(lv_obj_t *parent, const char *text, uint32_t col = 0xFFFFFF) {
  lv_obj_t *l = makeLabel(parent, "", &kids_font, lv_color_hex(col));
  lv_label_set_text(l, text);
  lv_obj_center(l);
  return l;
}

static void kShow(int mode);

// ---------- Học chữ ----------
static void kLearnDraw() {
  lv_obj_clean(kBody);
  const KLetter &L = KL[kLearnIdx];
  uint32_t col = KL_COL[kLearnIdx % 8];
  lv_obj_t *card = kTile(kBody, 76, 0, 304, 232, col, [](lv_event_t *e) {
    if (!kSay(kLearnIdx, true)) soundPlay(SND(SND_CHIRP), 0, false);  // no recording on the card: a chirp
    lv_obj_t *l = lv_obj_get_child(lv_event_get_target(e), 0);  // a little hop
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, l);
    lv_anim_set_exec_cb(&a, [](void *o, int32_t v) { lv_obj_set_style_translate_y((lv_obj_t *)o, v, 0); });
    lv_anim_set_values(&a, 0, -14);
    lv_anim_set_time(&a, 140);
    lv_anim_set_playback_time(&a, 180);
    lv_anim_start(&a);
  });
  char both[12];
  snprintf(both, sizeof(both), "%s%s", L.up, L.lo);
  lv_obj_t *big = kBig(card, both);
  lv_obj_align(big, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_t *w = makeLabel(card, "", F_HEADING, lv_color_hex(0xFFFFFF));
  lv_label_set_text(w, L.word);
  lv_obj_align(w, LV_ALIGN_BOTTOM_MID, 0, -20);
  lv_obj_t *n = makeLabel(kBody, "", F_BODY, C_MUTED);
  setTextFmt(n, "%d / %d", kLearnIdx + 1, KL_N);
  lv_obj_align(n, LV_ALIGN_BOTTOM_MID, 0, -2);
  for (int s = -1; s <= 1; s += 2) {  // big arrows either side
    lv_obj_t *b = kTile(kBody, s < 0 ? 0 : 396, 76, 60, 80, 0x34406A, [](lv_event_t *e) {
      int d = (int)(intptr_t)lv_event_get_user_data(e);
      kLearnIdx = (kLearnIdx + d + KL_N) % KL_N;
      kLearnDraw();
      kSay(kLearnIdx, true);
    }, (void *)(intptr_t)s);
    lv_obj_t *ar = makeLabel(b, s < 0 ? LV_SYMBOL_LEFT : LV_SYMBOL_RIGHT, F_HEADING, lv_color_hex(0xFFFFFF));
    lv_obj_center(ar);
  }
}

// ---------- the quizzes ----------
static void kQuizNew();

static void kQuizEnd() {
  lv_obj_clean(kBody);
  char f[8];
  snprintf(f, sizeof(f), "ket_%d", kRoundGems >= 9 ? 1 : kRoundGems >= 6 ? 2 : 3);
  if (!kVoice(f)) soundPlay(SND(SND_EVOLVE), 0, false);
  else ledFlash(LED_RAINBOW, 1, 1500);
  lv_obj_t *card = kTile(kBody, 48, 4, 360, 220, kRoundGems >= 7 ? 0xF7A531 : 0x4C8BF5, nullptr);
  lv_obj_t *t = makeLabel(card, "", F_HEADING, lv_color_hex(0xFFFFFF));
  lv_label_set_text(t, kRoundGems >= 9 ? "Giỏi quá!" : kRoundGems >= 6 ? "Con làm tốt lắm!" : "Cố lên nào!");
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 18);
  for (int i = 0; i < 5; i++) {  // up to five diamonds: two right answers each
    lv_obj_t *s = kGem(card, 44, i < (kRoundGems + 1) / 2);
    lv_obj_set_pos(s, 50 + i * 52, 72);
  }
  lv_obj_t *n = makeLabel(card, "", F_LABEL, lv_color_hex(0xFFFFFF));
  setTextFmt(n, "%d / 10", kRoundGems);
  lv_obj_align(n, LV_ALIGN_TOP_MID, 0, 128);
  lv_obj_t *again = ckBtn(card, "Chơi tiếp", lv_color_hex(0xFFFFFF), true, [](lv_event_t *) {
    kQ = kRoundGems = 0;
    kShow(kMode);
  }, 150, 44);
  lv_obj_align(again, LV_ALIGN_BOTTOM_MID, 0, -14);
}

static void kAnswer(lv_event_t *e) {
  int k = (int)(intptr_t)lv_event_get_user_data(e);
  if (kNextAt) return;  // already answered, the next one is coming
  lv_obj_t *b = kOptBtn[k];
  if (kOpt[k] == kTarget) {
    lv_obj_set_style_bg_color(b, lv_color_hex(0x2FC46B), 0);
    lv_obj_set_style_bg_grad_color(b, lv_color_hex(0x1E9A52), 0);
    char f[8];
    snprintf(f, sizeof(f), "dung_%d", (int)random(1, 4));
    if (!kVoice(f)) soundPlay(SND(SND_OK), 0, false);
    else ledFlash(0x20FF60, 1, 180);
    if (!kMissed) {
      kRoundGems++;
      kGemsAdd(1);
    }
    lv_obj_set_style_bg_color(kDots[kQ], kMissed ? lv_color_hex(0xFFB84A) : lv_color_hex(0x2FC46B), 0);
    kNextAt = millis() + 1500;  // time for the praise
  } else {
    kMissed = true;
    lv_obj_set_style_bg_color(b, lv_color_hex(0x5C668F), 0);
    lv_obj_set_style_bg_grad_color(b, lv_color_hex(0x434B70), 0);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_CLICKABLE);
    if (!kVoiceFor("sai_", kOpt[k])) soundPlay(SND(SND_ERROR), 0, false);  // "Đây là chữ dờ. Con thử lại nhé"
    else ledAlert(0xFF1010, 2, 140);
    lv_anim_t a;  // shake
    lv_anim_init(&a);
    lv_anim_set_var(&a, b);
    lv_anim_set_exec_cb(&a, [](void *o, int32_t v) { lv_obj_set_style_translate_x((lv_obj_t *)o, v, 0); });
    lv_anim_set_values(&a, -8, 8);
    lv_anim_set_time(&a, 60);
    lv_anim_set_playback_time(&a, 60);
    lv_anim_set_repeat_count(&a, 2);
    lv_anim_set_ready_cb(&a, [](lv_anim_t *x) { lv_obj_set_style_translate_x((lv_obj_t *)x->var, 0, 0); });
    lv_anim_start(&a);
  }
}

static bool kOptHas(int n, int v) {
  for (int i = 0; i < n; i++)
    if (kOpt[i] == v) return true;
  return false;
}

static void kQuizNew() {
  lv_obj_clean(kBody);
  kNextAt = 0;
  kMissed = false;
  int prev = kTarget;
  do kTarget = random(0, KL_N);
  while (kTarget == prev);
  // options: the answer, look-alikes first, then any letters
  kOpt[0] = kTarget;
  int n = 1;
  for (auto &g : KL_LOOKALIKE) {
    bool in = false;
    for (int j = 0; g[j] >= 0; j++) in |= g[j] == kTarget;
    if (!in) continue;
    for (int tries = 0; tries < 8 && n < 3; tries++) {
      int c = 0;
      while (g[c] >= 0) c++;
      int v = g[random(0, c)];
      if (!kOptHas(n, v)) kOpt[n++] = v;
    }
  }
  while (n < 4) {
    int v = random(0, KL_N);
    if (!kOptHas(n, v)) kOpt[n++] = v;
  }
  for (int i = 3; i > 0; i--) std::swap(kOpt[i], kOpt[random(0, i + 1)]);

  // progress dots
  for (int i = 0; i < 10; i++) {
    kDots[i] = shape(kBody, 16 + i * 16, 0, 10, 10, i < kQ ? 0x2FC46B : 0x2A3359);
  }
  for (int i = 0; i < kQ; i++) lv_obj_set_style_bg_color(kDots[i], lv_color_hex(0x2FC46B), 0);
  // target on the left: tap to hear it again
  uint32_t col = KL_COL[kTarget % 8];
  lv_obj_t *tg = kTile(kBody, 0, 18, 170, 214, col, [](lv_event_t *) { kAsk(); });
  lv_obj_t *q = makeLabel(tg, "", F_BODY, lv_color_hex(0xFFFFFF));
  lv_label_set_text(q, kMode == KM_FIND ? "Tìm chữ" : "Chữ thường của");
  lv_obj_align(q, LV_ALIGN_TOP_MID, 0, 12);
  kBig(tg, kMode == KM_FIND ? KL[kTarget].lo : KL[kTarget].up);
  lv_obj_t *arrow = makeLabel(kBody, LV_SYMBOL_RIGHT, F_HEADING, C_MUTED);
  lv_obj_set_pos(arrow, 178, 108);
  // four answers, 2 x 2
  for (int k = 0; k < 4; k++) {
    kOptBtn[k] = kTile(kBody, 206 + (k % 2) * 126, 18 + (k / 2) * 110, 118, 102, 0x34406A, kAnswer, (void *)(intptr_t)k);
    kBig(kOptBtn[k], KL[kOpt[k]].lo);
  }
  kAsk();
}

static void kTick(lv_timer_t *) {
  if (!kNextAt || (int32_t)(millis() - kNextAt) < 0) return;
  kNextAt = 0;
  if (++kQ >= 10) kQuizEnd();
  else kQuizNew();
}

// ---------- menu ----------
static void kShow(int mode) {
  kMode = mode;
  kNextAt = 0;
  lv_obj_clean(kBody);
  if (mode == KM_LEARN) {
    kLearnDraw();
    kSay(kLearnIdx, true);
    return;
  }
  if (mode == KM_FIND || mode == KM_CASE) {
    kQ = kRoundGems = 0;
    kQuizNew();
    return;
  }
  static const char *const names[3] = {"Học chữ", "Tìm chữ", "Chữ hoa\nchữ thường"};
  static const char *const big[3] = {"a", "ơ", "Aa"};
  static const uint32_t cols[3] = {0xFF6B8B, 0x4CD27A, 0x4C8BF5};
  for (int i = 0; i < 3; i++) {
    lv_obj_t *t = kTile(kBody, i * 152, 8, 144, 214, cols[i], [](lv_event_t *e) {
      soundPlay(SND(SND_TAP), 0, false);
      kShow(KM_LEARN + (int)(intptr_t)lv_event_get_user_data(e));
    }, (void *)(intptr_t)i);
    lv_obj_t *b = kBig(t, big[i]);
    lv_obj_align(b, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_t *l = makeLabel(t, "", F_LABEL, lv_color_hex(0xFFFFFF));
    lv_label_set_text(l, names[i]);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(l, LV_ALIGN_BOTTOM_MID, 0, -16);
  }
}

static void kBack(lv_event_t *e) {
  if (kMode != KM_MENU) kShow(KM_MENU);  // from a game back to the three choices
  else goBack(e);
}

static void pageKids() {
  kGemsLoad();
  lv_obj_t *c;
  kScr = pageBegin("Voi học chữ", &c, kBack);
  lv_obj_del(c);
  lv_obj_add_event_cb(kScr, [](lv_event_t *) {
    kScr = kBody = kGemLbl = nullptr;
    kMode = KM_MENU;
  }, LV_EVENT_DELETE, nullptr);
  lv_obj_t *st = kGem(kScr, 28, true);  // diamonds earned so far, top right
  lv_obj_align(st, LV_ALIGN_TOP_RIGHT, -64, SB_H + 12);
  kGemLbl = makeLabel(kScr, "", F_TITLE, lv_color_hex(0x9FEBFF));
  setTextFmt(kGemLbl, "%d", kGems);
  lv_obj_align(kGemLbl, LV_ALIGN_TOP_LEFT, 420, SB_H + 13);
  kBody = lv_obj_create(kScr);
  lv_obj_remove_style_all(kBody);
  lv_obj_set_size(kBody, 456, 320 - SB_H - 54);
  lv_obj_set_pos(kBody, 12, SB_H + 52);
  lv_obj_clear_flag(kBody, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);  // swipes end here
  lv_obj_add_event_cb(kBody, [](lv_event_t *) {  // swipe through the letters in Học chữ
    if (kMode != KM_LEARN) return;
    lv_indev_t *in = lv_indev_get_act();
    lv_dir_t d = lv_indev_get_gesture_dir(in);
    if (d != LV_DIR_LEFT && d != LV_DIR_RIGHT) return;
    lv_indev_wait_release(in);  // no click on the card where the swipe began
    kLearnIdx = (kLearnIdx + (d == LV_DIR_LEFT ? 1 : -1) + KL_N) % KL_N;
    kLearnDraw();
    kSay(kLearnIdx, true);
  }, LV_EVENT_GESTURE, nullptr);
  kShow(KM_MENU);
  kVoice("chao_mung");  // "Chào mừng Voi con đã quay lại với lớp học..."
  pageTimer(kScr, kTick, 100);
  pageShow(kScr);
}
