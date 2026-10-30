#pragma once
#include "hd_img.h"
// Games hub with three games for children made to be played for months:
//
//  * "Lật hình" (memory match): 30 levels from 3 to 12 pairs. The cards show the pixel-art dragons and treats of the
//    dragon game; they are shown for a moment, then turned down. 1 to 3 stars by the number of flips; a level opens
//    when the one before it is done. Cards are drawn at random every time, so a level is never the same twice.
//  * "Đếm cùng rồng" (counting and sums): ten steps, from counting 1-3 objects up to the times tables, that adapt to the
//    child (three misses in a row go one step back, ten right answers go one step up). Every 10 right answers give a
//    sticker of a dragon for the album (10 stickers) and 5 coins for the real dragon.
//  * "Mê cung" (maze): 30 levels from 5x3 to 24x16 cells. Levels 11-20 hide gems to pick up before the exit opens,
//    levels 21-30 are dark (only the walls near the dragon and the way walked are shown). Stars by steps walked.
//
// Progress is kept in NVS namespace "games". Included by main.cpp after dragon.h and home.h (sprites, coins).
#include <algorithm>

// ---------- saved progress ----------
static uint8_t gmStars[30];      // stars per memory level (0 = not done)
static uint8_t mzStars[30];      // stars per maze level
static uint8_t gmCLevel = 1;     // counting step 1..10
static uint8_t gmCOk = 0;        // right answers on this step
static uint32_t gmCTotal = 0;    // right answers ever (10 of them = a sticker)

static void gamesLoad() {
  Preferences p;
  p.begin("games", true);
  memset(gmStars, 0, sizeof(gmStars));
  p.getBytes("ms", gmStars, sizeof(gmStars));
  memset(mzStars, 0, sizeof(mzStars));
  p.getBytes("mz", mzStars, sizeof(mzStars));
  gmCLevel = constrain(p.getUChar("cl", 1), 1, 10);
  gmCOk = p.getUChar("co", 0);
  gmCTotal = p.getUInt("ct", 0);
  p.end();
  for (auto &s : gmStars) s = s > 3 ? 3 : s;
  for (auto &s : mzStars) s = s > 3 ? 3 : s;
}
static void gamesSave() {
  Preferences p;
  p.begin("games", false);
  p.putBytes("ms", gmStars, sizeof(gmStars));
  p.putBytes("mz", mzStars, sizeof(mzStars));
  p.putUChar("cl", gmCLevel);
  p.putUChar("co", gmCOk);
  p.putUInt("ct", gmCTotal);
  p.end();
}
static int gmStarTotal(const uint8_t *stars = gmStars) {
  int n = 0;
  for (int i = 0; i < 30; i++) n += stars[i];
  return n;
}
static int gmStickers() { return gmCTotal / 10 > 10 ? 10 : (int)(gmCTotal / 10); }

// Coins for the real dragon (when there is one).
static void gmCoins(int n) {
  if (n <= 0 || !dgOk) return;
  dg.coins += n;
  dgSave();
  char b[40];
  snprintf(b, sizeof(b), isVi() ? "+%d xu cho rồng" : "+%d coins for the dragon", n);
  uiToast(b, C_OK);
}

// ---------- shared pieces ----------
static lv_obj_t *gmHub = nullptr;

static lv_obj_t *gmScreen() {
  lv_obj_t *s = lv_obj_create(nullptr);
  styleScreen(s);
  return s;
}

// Back button + title on the top bar (the status bar of the system sits above, y < SB_H).
static lv_obj_t *gmBar(lv_obj_t *s, const char *title, lv_event_cb_t back) {
  lv_obj_t *b = smallBtn(s, LV_SYMBOL_LEFT, C_ACCENT, back, nullptr, 48);
  lv_obj_set_size(b, 48, 34);
  lv_obj_set_pos(b, 8, SB_H + 4);
  lv_obj_t *t = makeLabel(s, "", F_TITLE, C_TEXT);
  lv_label_set_text(t, title);
  lv_obj_set_pos(t, 64, SB_H + 9);
  return t;
}

// Earned stars as gold dots (n of 3).
static void gmStarDots(lv_obj_t *parent, int x, int y, int n, int size) {
  for (int i = 0; i < 3; i++) {
    lv_obj_t *d = shape(parent, x + i * (size + 3), y, size, size, i < n ? 0xFFC93C : 0x3A4470);
    lv_obj_set_style_border_width(d, 1, 0);
    lv_obj_set_style_border_color(d, i < n ? lv_color_hex(0xFFF1B0) : lv_color_hex(0x2A3359), 0);
  }
}

static lv_obj_t *gmOverlay(lv_obj_t *s) {  // dim layer that swallows touches
  lv_obj_t *ov = lv_obj_create(s);
  lv_obj_remove_style_all(ov);
  lv_obj_set_size(ov, 480, 320);
  lv_obj_set_style_bg_color(ov, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(ov, LV_OPA_60, 0);
  lv_obj_add_flag(ov, LV_OBJ_FLAG_CLICKABLE);
  return ov;
}

// Sprite scaled to fit a box, with whole or half steps (pixel art stays crisp).
static lv_obj_t *gmSprite(lv_obj_t *parent, const lv_img_dsc_t *src, int box) {
  lv_obj_t *img = lv_img_create(parent);
  lv_img_set_src(img, src);
  int maxdim = src->header.w > src->header.h ? src->header.w : src->header.h;
  int zoom = 256 * box / maxdim;
  zoom = zoom / 128 * 128;
  if (zoom < 256) zoom = 256;
  lv_img_set_antialias(img, false);
  lv_img_set_pivot(img, src->header.w / 2, src->header.h / 2);
  lv_img_set_zoom(img, zoom);
  lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
  return img;
}

// Put a (zoomed) sprite so that its centre is at (cx, cy) of the parent.
static void gmCentre(lv_obj_t *img, int cx, int cy) {
  const lv_img_dsc_t *d = (const lv_img_dsc_t *)lv_img_get_src(img);
  lv_obj_set_pos(img, cx - d->header.w / 2, cy - d->header.h / 2);
}

// Level result: stars, one line, and Next / Again / Levels. play(level) and list() replace the game screen.
static int gmWinLevel;
static void (*gmWinPlay)(int);
static void (*gmWinList)();
static void gmWinCard(lv_obj_t *scr, int level, int stars, const char *line, void (*play)(int), void (*list)()) {
  gmWinLevel = level;
  gmWinPlay = play;
  gmWinList = list;
  SFX("win", SND_OK);
  lv_obj_t *ov = gmOverlay(scr);
  lv_obj_t *card = pxPanel(ov, 360, 196);
  lv_obj_center(card);
  lv_obj_t *t = pxText(card, DL("Hoan hô!", "Well done!"), F_HEADING);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 10);
  gmStarDots(card, 360 / 2 - 40 - 10, 52, stars, 26);
  lv_obj_t *l = pxText(card, line, F_BODY);
  lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 92);
  lv_obj_t *next = pxButton(card, 108, 40, nullptr, DL("Màn sau", "Next"), [](lv_event_t *) { gmWinPlay(gmWinLevel + 1); }, nullptr);
  lv_obj_align(next, LV_ALIGN_BOTTOM_LEFT, 10, -10);
  if (level >= 29) lv_obj_add_flag(next, LV_OBJ_FLAG_HIDDEN);
  lv_obj_t *again = pxButton(card, 108, 40, nullptr, DL("Chơi lại", "Again"), [](lv_event_t *) { gmWinPlay(gmWinLevel); }, nullptr);
  lv_obj_align(again, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_t *lst = pxButton(card, 108, 40, nullptr, DL("Danh sách", "Levels"), [](lv_event_t *) { gmWinList(); }, nullptr);
  lv_obj_align(lst, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
}

// Grid of 30 levels with their stars; a level opens when the one before it is done. play(level) replaces this screen.
static const uint8_t *gmListStars;
static void (*gmListPlay)(int);
static void gmLevelList(const char *title, const uint8_t *stars, void (*play)(int)) {
  gamesLoad();
  gmListStars = stars;
  gmListPlay = play;
  lv_obj_t *s = gmScreen();
  gmBar(s, title, goBack);
  char b[32];
  snprintf(b, sizeof(b), isVi() ? "%d/90 sao" : "%d/90 stars", gmStarTotal(stars));
  lv_obj_t *tot = makeLabel(s, b, F_LABEL, lv_color_hex(0xFFC93C));
  lv_obj_set_pos(tot, 368, SB_H + 11);

  lv_obj_t *grid = lv_obj_create(s);
  lv_obj_remove_style_all(grid);
  lv_obj_set_size(grid, 472, 320 - (SB_H + 44) - 2);
  lv_obj_set_pos(grid, 4, SB_H + 44);
  lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_row(grid, 8, 0);
  lv_obj_set_style_pad_column(grid, 8, 0);
  lv_obj_set_style_pad_all(grid, 4, 0);
  lv_obj_set_scroll_dir(grid, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);
  int next = 0;  // the first level not done yet
  while (next < 29 && stars[next]) next++;
  lv_obj_t *focus = nullptr;
  for (int i = 0; i < 30; i++) {
    bool open = i == 0 || stars[i - 1] > 0;
    lv_obj_t *btn = lv_obj_create(grid);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, 68, 58);
    lv_obj_set_style_radius(btn, 14, 0);
    lv_obj_set_style_bg_color(btn, open ? (i == next ? lv_color_hex(0x6E55D6) : C_CARD) : lv_color_hex(0x1A2038), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, i == next ? 2 : 1, 0);
    lv_obj_set_style_border_color(btn, i == next ? C_ACCENT : C_BORDER, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(btn, LV_OPA_70, LV_STATE_PRESSED);
    char n[6];
    snprintf(n, sizeof(n), "%d", i + 1);
    lv_obj_t *num = makeLabel(btn, n, F_TITLE, open ? C_TEXT : lv_color_hex(0x4A5378));
    lv_obj_align(num, LV_ALIGN_TOP_MID, 0, 5);
    if (open) gmStarDots(btn, 68 / 2 - 20, 38, stars[i], 10);
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
      int i = (int)(intptr_t)lv_event_get_user_data(e);
      if (i == 0 || gmListStars[i - 1] > 0) gmListPlay(i);
      else {
        SFX("error", SND_ERROR);
        uiToast(DL("Hoàn thành màn trước để mở màn này", "Finish the previous level to open this one"), C_WARN);
      }
    }, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    if (i == next) focus = btn;
  }
  lv_obj_update_layout(grid);
  if (focus) lv_obj_scroll_to_view(focus, LV_ANIM_OFF);
  pageShow(s);
}

// =====================================================================================
// Game 1: memory match
// =====================================================================================
static const uint8_t MEM_PAIRS[30] = {3, 3, 3, 4, 4, 4, 6, 6, 6, 6, 8, 8, 8, 8, 8, 10, 10, 10, 10, 10, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12};
static const lv_img_dsc_t *gmFaces[16];
static int gmFaceCount = 0;
static void gmFacesInit() {
  if (gmFaceCount) return;
  for (int i = 0; i < 8; i++) gmFaces[gmFaceCount++] = DRAGON_SPRITES[i][0];
  gmFaces[gmFaceCount++] = &egg_warm;
  gmFaces[gmFaceCount++] = &fx_gem;
  gmFaces[gmFaceCount++] = &fx_heart;
  gmFaces[gmFaceCount++] = &fx_coin;
  gmFaces[gmFaceCount++] = &fx_food0;
  gmFaces[gmFaceCount++] = &fx_food1;
  gmFaces[gmFaceCount++] = &fx_food2;
}

struct MemCard {
  lv_obj_t *obj, *face, *back;
  uint8_t pair;
  bool open, done;
};
enum { MP_PEEK, MP_PLAY, MP_RESOLVE, MP_WIN };
static MemCard mc[24];
static int mcLevel, mcCount, mcPairs, mcMoves, mcFound, mcFirst, mcSecond, mcPhase;
static uint32_t mcAt;
static lv_obj_t *mcScr, *mcMovesLbl;

static void memList();
static void memPlay(int level);
static void memPlayReplace(int level) {  // the screen it comes from (win card, level list) is deleted
  pageReplace = true;
  memPlay(level);
  pageReplace = false;
}
static void memListReplace() {
  pageReplace = true;
  memList();
  pageReplace = false;
}

static void mcShow(int i, bool up) {
  MemCard &c = mc[i];
  c.open = up;
  lv_obj_set_style_bg_color(c.obj, up ? lv_color_hex(0xF4F1FF) : lv_color_hex(0x6E55D6), 0);
  if (up) {
    lv_obj_add_flag(c.back, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(c.face, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(c.back, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(c.face, LV_OBJ_FLAG_HIDDEN);
  }
}

static int mcStarsFor(int moves) {
  return moves <= mcPairs * 8 / 5 + 1 ? 3 : moves <= mcPairs * 12 / 5 + 1 ? 2 : 1;
}

static void mcWin() {
  int stars = mcStarsFor(mcMoves), old = gmStars[mcLevel];
  Serial.printf("GAME mem win level %d moves %d stars %d\n", mcLevel + 1, mcMoves, stars);
  if (stars > old) {
    gmStars[mcLevel] = stars;
    gamesSave();
    gmCoins((stars - old) * 2);
  }
  char b[64];
  snprintf(b, sizeof(b), isVi() ? "Màn %d  -  %d lượt lật" : "Level %d  -  %d flips", mcLevel + 1, mcMoves);
  gmWinCard(mcScr, mcLevel, stars, b, memPlayReplace, memListReplace);
}

static void mcTick(lv_timer_t *) {
  uint32_t now = millis();
  if (mcPhase == MP_PEEK && now >= mcAt) {  // the preview is over: turn everything down
    for (int i = 0; i < mcCount; i++) mcShow(i, false);
    mcPhase = MP_PLAY;
  } else if (mcPhase == MP_RESOLVE && now >= mcAt) {  // not a pair: turn the two back
    mcShow(mcFirst, false);
    mcShow(mcSecond, false);
    mcFirst = mcSecond = -1;
    mcPhase = MP_PLAY;
  } else if (mcPhase == MP_WIN && now >= mcAt) {
    mcPhase = -1;
    mcWin();
  }
}

static void mcCardTap(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (mcPhase != MP_PLAY || mc[i].open || mc[i].done) return;
  mcShow(i, true);
  SFX("tap", SND_TAP);
  if (mcFirst < 0) {
    mcFirst = i;
    return;
  }
  mcSecond = i;
  mcMoves++;
  setTextFmt(mcMovesLbl, isVi() ? "%d lượt" : "%d flips", mcMoves);
  if (mc[mcFirst].pair == mc[mcSecond].pair) {
    for (int k : {mcFirst, mcSecond}) {
      mc[k].done = true;
      lv_obj_set_style_bg_color(mc[k].obj, lv_color_hex(0xC8F5D4), 0);
    }
    mcFirst = mcSecond = -1;
    mcFound++;
    Serial.printf("GAME mem match %d/%d moves %d\n", mcFound, mcPairs, mcMoves);
    SFX("gem", SND_OK);
    if (mcFound == mcPairs) {
      mcPhase = MP_WIN;
      mcAt = millis() + 700;
    }
  } else {
    SFX("gem_miss", SND_ERROR);
    mcPhase = MP_RESOLVE;
    mcAt = millis() + 800;
  }
}

static void memPlay(int level) {
  gmFacesInit();
  mcLevel = level;
  mcPairs = MEM_PAIRS[level];
  mcCount = mcPairs * 2;
  mcMoves = mcFound = 0;
  mcFirst = mcSecond = -1;
  static const uint8_t DIMS[13][2] = {{0, 0}, {0, 0}, {0, 0}, {3, 2}, {4, 2}, {0, 0}, {4, 3}, {0, 0}, {4, 4}, {0, 0}, {5, 4}, {0, 0}, {6, 4}};
  int cols = DIMS[mcPairs][0], rows = DIMS[mcPairs][1];

  lv_obj_t *s = gmScreen();
  mcScr = s;
  lv_obj_add_event_cb(s, [](lv_event_t *) { mcScr = nullptr; }, LV_EVENT_DELETE, nullptr);
  char b[40];
  snprintf(b, sizeof(b), isVi() ? "Màn %d" : "Level %d", level + 1);
  gmBar(s, b, [](lv_event_t *) { memListReplace(); });
  mcMovesLbl = makeLabel(s, "", F_LABEL, C_ACCENT);
  lv_label_set_text(mcMovesLbl, isVi() ? "0 lượt" : "0 flips");
  lv_obj_set_pos(mcMovesLbl, 380, SB_H + 11);

  // pick the faces and shuffle the cards
  uint8_t pick[16], order[24];
  for (int i = 0; i < gmFaceCount; i++) pick[i] = i;
  for (int i = gmFaceCount - 1; i > 0; i--) std::swap(pick[i], pick[random(0, i + 1)]);
  for (int i = 0; i < mcCount; i++) order[i] = pick[i / 2];
  for (int i = mcCount - 1; i > 0; i--) std::swap(order[i], order[random(0, i + 1)]);

  const int GAP = 6, AX = 4, AY = SB_H + 44, AW = 472, AH = 320 - AY - 4;
  int cw = (AW - (cols - 1) * GAP) / cols, ch = (AH - (rows - 1) * GAP) / rows;
  for (int i = 0; i < mcCount; i++) {
    MemCard &c = mc[i];
    c.pair = order[i];
    c.done = false;
    int r = i / cols, k = i % cols;
    c.obj = lv_obj_create(s);
    lv_obj_remove_style_all(c.obj);
    lv_obj_set_size(c.obj, cw, ch);
    lv_obj_set_pos(c.obj, AX + k * (cw + GAP), AY + r * (ch + GAP));
    lv_obj_set_style_radius(c.obj, 14, 0);
    lv_obj_set_style_bg_opa(c.obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(c.obj, 2, 0);
    lv_obj_set_style_border_color(c.obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_opa(c.obj, LV_OPA_40, 0);
    lv_obj_clear_flag(c.obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(c.obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(c.obj, mcCardTap, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    c.back = makeLabel(c.obj, "?", F_HEADING, lv_color_hex(0xFFFFFF));
    lv_obj_center(c.back);
    c.face = gmSprite(c.obj, gmFaces[c.pair], (cw < ch ? cw : ch) - 18);
    lv_obj_center(c.face);
    mcShow(i, true);  // everything is shown first: the child gets a moment to remember
  }
  mcPhase = MP_PEEK;
  mcAt = millis() + (level < 6 ? 3200 : level < 15 ? 2600 : level < 25 ? 2000 : 1500);
  pageTimer(s, mcTick, 40);
  pageShow(s);
}

static void memList() { gmLevelList(DL("Lật hình", "Memory match"), gmStars, memPlayReplace); }

// =====================================================================================
// Game 2: counting with the dragon
// =====================================================================================
static const uint8_t CG_NEED[10] = {8, 8, 8, 10, 10, 10, 10, 10, 10, 12};  // right answers to go up a step
static int cgA, cgB, cgKind, cgAnswer, cgTries, cgWrongRun, cgOpt[4], cgOptN;
static const lv_img_dsc_t *cgSpriteA, *cgSpriteB;
static lv_obj_t *cgScr, *cgArea, *cgEq, *cgPrompt, *cgBtn[4], *cgStepLbl, *cgBar, *cgStickLbl, *cgMascot, *cgSay;
static uint32_t cgNextAt = 0;

static void cgNew();
static void cgAlbum();

static const lv_img_dsc_t *const CG_TOYS[6] = {&fx_gem, &fx_heart, &fx_coin, &fx_food0, &fx_food1, &fx_food2};
static int cgRnd(int lo, int hi) { return random(lo, hi + 1); }

// Question kinds: 0 count, 1 add with objects, 2 take away with objects, 3 equation add/sub, 4 missing number,
// 5 two digits, 6 times tables.
static void cgMake() {
  int lv = gmCLevel;
  if (lv >= 10) lv = 7 + random(0, 4);  // after the last step: a mix of the hard ones, forever
  cgSpriteA = CG_TOYS[random(0, 6)];
  do cgSpriteB = CG_TOYS[random(0, 6)];
  while (cgSpriteB == cgSpriteA);
  switch (lv) {
    case 1: cgKind = 0; cgA = cgRnd(1, 3); cgAnswer = cgA; break;
    case 2: cgKind = 0; cgA = cgRnd(1, 5); cgAnswer = cgA; break;
    case 3: cgKind = 0; cgA = cgRnd(3, 10); cgAnswer = cgA; break;
    case 4: cgKind = 1; cgA = cgRnd(1, 4); cgB = cgRnd(1, 5 - cgA); cgAnswer = cgA + cgB; break;
    case 5: cgKind = 1; cgA = cgRnd(1, 8); cgB = cgRnd(1, 10 - cgA); cgAnswer = cgA + cgB; break;
    case 6: cgKind = 2; cgA = cgRnd(2, 10); cgB = cgRnd(1, cgA - 1); cgAnswer = cgA - cgB; break;
    case 7:
      cgKind = 3;
      if (random(0, 2)) { cgA = cgRnd(2, 15); cgB = cgRnd(1, 20 - cgA); cgAnswer = cgA + cgB; }
      else { cgA = cgRnd(5, 20); cgB = cgRnd(1, cgA - 1); cgAnswer = cgA - cgB; cgB = -cgB; }
      break;
    case 8: cgKind = 4; cgAnswer = cgRnd(1, 15); cgA = cgRnd(1, 19 - cgAnswer); cgB = cgA + cgAnswer; break;  // cgA + ? = cgB
    case 9:
      cgKind = 5;
      if (random(0, 2)) { cgA = cgRnd(10, 40); cgB = cgRnd(2, 50 - cgA); cgAnswer = cgA + cgB; }
      else { cgA = cgRnd(20, 50); cgB = cgRnd(2, cgA - 5); cgAnswer = cgA - cgB; cgB = -cgB; }
      break;
    default: {
      static const uint8_t T[3] = {2, 5, 10};
      cgKind = 6; cgA = T[random(0, 3)]; cgB = cgRnd(1, 10); cgAnswer = cgA * cgB;
    }
  }
  // answer buttons: the right one and some near misses
  cgOptN = lv <= 2 ? 3 : 4;
  int opts[4] = {cgAnswer, 0, 0, 0}, n = 1, guard = 0;
  while (n < cgOptN && guard++ < 60) {
    int d = cgRnd(1, lv >= 9 ? 12 : 3) * (random(0, 2) ? 1 : -1), v = cgAnswer + d;
    if (lv == 10) v = cgAnswer + (random(0, 2) ? cgA : -cgA) * cgRnd(1, 2);  // times tables: off by a multiple
    if (v < 0 || v == cgAnswer || (lv <= 3 && (v > 10 || v < 1))) continue;
    bool dup = false;
    for (int i = 0; i < n; i++) dup |= opts[i] == v;
    if (!dup) opts[n++] = v;
  }
  while (n < cgOptN) opts[n++] = cgAnswer + 11 + n;  // never short of answers
  for (int i = cgOptN - 1; i > 0; i--) std::swap(opts[i], opts[random(0, i + 1)]);
  for (int i = 0; i < cgOptN; i++) cgOpt[i] = opts[i];
  Serial.printf("GAME count step %d kind %d answer %d options %d,%d,%d,%d\n", (int)gmCLevel, cgKind, cgAnswer, cgOpt[0], cgOpt[1], cgOpt[2], cgOptN > 3 ? cgOpt[3] : -1);
}

static void cgMood(int expr, const char *say) {  // the dragon at the corner: 0 normal, 1 happy, 2 sad
  int st = dgOk && dg.stage > ST_EGG ? dg.stage - 1 : 0;
  lv_img_set_src(cgMascot, DRAGON_SPRITES[st][expr]);
  lv_label_set_text(cgSay, say);
}

static void cgStatus() {
  setTextFmt(cgStepLbl, isVi() ? "Bậc %d" : "Step %d", gmCLevel);
  lv_bar_set_range(cgBar, 0, CG_NEED[gmCLevel - 1]);
  lv_bar_set_value(cgBar, gmCOk > CG_NEED[gmCLevel - 1] ? CG_NEED[gmCLevel - 1] : gmCOk, LV_ANIM_ON);
  setTextFmt(cgStickLbl, "%d/10", gmStickers());
}

static void cgSticker() {  // a new dragon sticker, every 10 right answers
  int idx = gmStickers() - 1;
  lv_obj_t *ov = gmOverlay(cgScr);
  lv_obj_t *card = pxPanel(ov, 300, 244);
  lv_obj_center(card);
  lv_obj_t *t = pxText(card, DL("Sticker mới!", "New sticker!"), F_HEADING);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 8);
  static const lv_img_dsc_t *const ST[10] = {&hd_egg, &hd_baby, &hd_lv1, &hd_lv2, &hd_lv3, &hd_adult, &hd_king, &hd_emperor, &hd_divine, &hd_egg_cold};
  lv_obj_t *img = lv_img_create(card);
  lv_img_set_src(img, ST[idx < 0 ? 0 : idx]);
  lv_img_set_pivot(img, 80, 80);
  lv_img_set_zoom(img, 160);
  lv_obj_set_pos(img, 300 / 2 - 80, 22 + 40 - 80 + 12);
  lv_obj_t *ok = pxButton(card, 120, 38, nullptr, DL("Tuyệt vời!", "Great!"), [](lv_event_t *e) {
    lv_obj_del_async(lv_obj_get_parent(lv_obj_get_parent(lv_event_get_target(e))));  // button -> card -> overlay
    cgNextAt = millis() + 200;
  }, nullptr);
  lv_obj_align(ok, LV_ALIGN_BOTTOM_MID, 0, -8);
  SFX("achievement", SND_COIN);
  gmCoins(5);
}

static void cgAfterRight() {
  gmCTotal++;
  gmCOk++;
  cgWrongRun = 0;
  bool up = false;
  if (gmCLevel < 10 && gmCOk >= CG_NEED[gmCLevel - 1]) {
    gmCLevel++;
    gmCOk = 0;
    up = true;
  }
  gamesSave();
  cgStatus();
  if (gmCTotal % 10 == 0) {
    cgNextAt = 0xFFFFFFFF;  // the sticker card decides when to go on
    cgSticker();
    return;
  }
  if (up) uiToast(isVi() ? "Lên bậc mới!" : "Next step!", C_OK);
  cgNextAt = millis() + 900;
}

static void cgAnswerTap(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (cgNextAt) return;  // already answered: wait
  lv_obj_t *b = cgBtn[i];
  if (cgOpt[i] == cgAnswer) {
    lv_obj_set_style_bg_color(b, C_OK, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    SFX("ok", SND_OK);
    static const char *const PRAISE_VI[4] = {"Giỏi quá!", "Chính xác!", "Hay lắm!", "Tuyệt!"};
    static const char *const PRAISE_EN[4] = {"Great!", "Correct!", "Well done!", "Super!"};
    cgMood(1, isVi() ? PRAISE_VI[random(0, 4)] : PRAISE_EN[random(0, 4)]);
    cgNextAt = millis() + 5000;  // placeholder so a double tap is ignored; cgAfterRight sets the real time
    cgAfterRight();
  } else {
    lv_obj_set_style_bg_color(b, C_DANGER, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_50, 0);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_CLICKABLE);
    SFX("error", SND_ERROR);
    cgMood(2, DL("Thử lại nhé!", "Try again!"));
    if (++cgTries >= 2) {  // show the answer and go on
      cgWrongRun++;
      for (int k = 0; k < cgOptN; k++)
        if (cgOpt[k] == cgAnswer) {
          lv_obj_set_style_bg_color(cgBtn[k], C_OK, 0);
          lv_obj_set_style_bg_opa(cgBtn[k], LV_OPA_COVER, 0);
        }
      if (cgWrongRun >= 3 && gmCLevel > 1) {  // three in a row: go back one step, gently
        gmCLevel--;
        gmCOk = 0;
        cgWrongRun = 0;
        gamesSave();
        cgStatus();
        uiToast(DL("Mình ôn lại bậc dễ hơn nhé", "Let's practise an easier step"), C_ACCENT);
      }
      cgNextAt = millis() + 1500;
    }
  }
}

static void cgTick(lv_timer_t *) {
  if (cgNextAt && cgNextAt != 0xFFFFFFFF && (int32_t)(millis() - cgNextAt) >= 0) {
    cgNextAt = 0;
    cgNew();
  }
}

// Draw the question: objects (kinds 0-2) and/or the equation.
static void cgNew() {
  cgMake();
  cgTries = 0;
  cgNextAt = 0;
  lv_obj_clean(cgArea);
  cgMood(0, DL("Cùng đếm nào!", "Let's count!"));
  char b[48];
  // up to 10 objects in one row, 35 px apart
  auto draw = [&](int n, int first, const lv_img_dsc_t *spr, int dimFrom) {
    for (int i = 0; i < n; i++) {
      int idx = first + i;
      lv_obj_t *o = gmSprite(cgArea, spr, 32);
      gmCentre(o, 20 + idx * 35, 44);
      if (dimFrom >= 0 && i >= dimFrom) lv_obj_set_style_img_opa(o, LV_OPA_30, 0);
    }
  };
  bool objects = cgKind <= 2;
  if (cgKind == 0) draw(cgA, 0, cgSpriteA, -1);
  else if (cgKind == 1) {
    draw(cgA, 0, cgSpriteA, -1);
    draw(cgB, cgA, cgSpriteB, -1);
  } else if (cgKind == 2) draw(cgA, 0, cgSpriteA, cgA - cgB);  // the last B are the ones taken away
  switch (cgKind) {
    case 0: snprintf(b, sizeof(b), "%s", DL("Có bao nhiêu?", "How many?")); break;
    case 1: snprintf(b, sizeof(b), "%d + %d = ?", cgA, cgB); break;
    case 2: snprintf(b, sizeof(b), "%d - %d = ?", cgA, cgB); break;
    case 3: snprintf(b, sizeof(b), cgB >= 0 ? "%d + %d = ?" : "%d - %d = ?", cgA, cgB >= 0 ? cgB : -cgB); break;
    case 4: snprintf(b, sizeof(b), "%d + ? = %d", cgA, cgB); break;
    case 5: snprintf(b, sizeof(b), cgB >= 0 ? "%d + %d = ?" : "%d - %d = ?", cgA, cgB >= 0 ? cgB : -cgB); break;
    default: snprintf(b, sizeof(b), "%d x %d = ?", cgA, cgB);
  }
  lv_label_set_text(cgEq, b);
  lv_obj_set_style_text_font(cgEq, cgKind == 0 ? F_HEADING : F_DISPLAY, 0);
  lv_obj_set_width(cgEq, 360);
  lv_obj_set_style_text_align(cgEq, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(cgEq, 8, cgKind == 0 ? SB_H + 150 : objects ? SB_H + 140 : SB_H + 96);
  for (int i = 0; i < 4; i++) {
    if (i >= cgOptN) {
      lv_obj_add_flag(cgBtn[i], LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    lv_obj_clear_flag(cgBtn[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cgBtn[i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(cgBtn[i], lv_color_hex(0x6E55D6), 0);
    lv_obj_set_style_bg_opa(cgBtn[i], LV_OPA_COVER, 0);
    char n[8];
    snprintf(n, sizeof(n), "%d", cgOpt[i]);
    lv_label_set_text(lv_obj_get_child(cgBtn[i], 0), n);
    int w = cgOptN == 3 ? 142 : 108;
    lv_obj_set_size(cgBtn[i], w, 68);
    lv_obj_set_pos(cgBtn[i], 8 + i * (w + (cgOptN == 3 ? 10 : 12)), 236);
  }
  cgStatus();
}

static void cgPlay() {
  gamesLoad();
  lv_obj_t *s = gmScreen();
  cgScr = s;
  lv_obj_add_event_cb(s, [](lv_event_t *) { cgScr = nullptr; }, LV_EVENT_DELETE, nullptr);
  cgStepLbl = gmBar(s, "", goBack);
  lv_obj_set_width(cgStepLbl, 90);
  cgBar = lv_bar_create(s);
  lv_obj_set_size(cgBar, 150, 12);
  lv_obj_set_pos(cgBar, 150, SB_H + 16);
  lv_obj_set_style_bg_color(cgBar, C_BORDER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(cgBar, lv_color_hex(0xFFC93C), LV_PART_INDICATOR);
  cgStickLbl = makeLabel(s, "", F_LABEL, lv_color_hex(0xFFC93C));
  lv_obj_set_pos(cgStickLbl, 392, SB_H + 11);
  lv_obj_t *album = smallBtn(s, LV_SYMBOL_IMAGE, C_ACCENT2, [](lv_event_t *) { cgAlbum(); }, nullptr, 44);
  lv_obj_set_size(album, 44, 34);
  lv_obj_set_pos(album, 326, SB_H + 4);

  cgArea = lv_obj_create(s);  // objects live here: one row of up to ten
  lv_obj_remove_style_all(cgArea);
  lv_obj_set_size(cgArea, 370, 96);
  lv_obj_set_pos(cgArea, 4, SB_H + 44);
  lv_obj_clear_flag(cgArea, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  cgEq = makeLabel(s, "", F_DISPLAY, C_TEXT);
  cgMascot = lv_img_create(s);
  lv_img_set_src(cgMascot, DRAGON_SPRITES[0][0]);
  lv_img_set_antialias(cgMascot, false);
  lv_img_set_pivot(cgMascot, 20, 20);
  lv_img_set_zoom(cgMascot, 512);
  gmCentre(cgMascot, 428, SB_H + 86);
  cgSay = makeLabel(s, "", F_CAPTION, C_TEXT);
  lv_obj_set_width(cgSay, 104);
  lv_obj_set_style_text_align(cgSay, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(cgSay, 376, SB_H + 136);
  for (int i = 0; i < 4; i++) {
    lv_obj_t *b = lv_obj_create(s);
    lv_obj_remove_style_all(b);
    lv_obj_set_style_radius(b, 18, 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_opa(b, LV_OPA_40, 0);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *l = makeLabel(b, "", F_HEADING, lv_color_hex(0xFFFFFF));
    lv_obj_center(l);
    lv_obj_add_event_cb(b, cgAnswerTap, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    cgBtn[i] = b;
  }
  cgWrongRun = 0;
  cgNew();
  pageTimer(s, cgTick, 100);
  pageShow(s);
}

// The sticker album: ten dragons, shown as shadows until they are won.
static void cgAlbum() {
  lv_obj_t *c;
  lv_obj_t *s = pageBegin(DL("Sưu tầm rồng", "Dragon stickers"), &c, [](lv_event_t *) {
    if (cgScr) lv_scr_load_anim(cgScr, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 250, 0, true);  // back to the game, the album is deleted
    else goBack(nullptr);
  });
  lv_obj_del(c);
  static const lv_img_dsc_t *const ST[10] = {&hd_egg, &hd_baby, &hd_lv1, &hd_lv2, &hd_lv3, &hd_adult, &hd_king, &hd_emperor, &hd_divine, &hd_egg_cold};
  int have = gmStickers();
  for (int i = 0; i < 10; i++) {
    lv_obj_t *card = makeCard(s, 86, 108);
    lv_obj_set_pos(card, 12 + (i % 5) * 91, SB_H + 52 + (i / 5) * 116);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *img = lv_img_create(card);
    lv_img_set_src(img, ST[i]);
    lv_img_set_pivot(img, 0, 0);
    lv_img_set_zoom(img, 128);
    lv_obj_set_pos(img, 2, 2);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
    bool got = i < have;
    if (!got) {
      lv_obj_set_style_img_recolor(img, lv_color_hex(0x0B1024), 0);
      lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, 0);
    }
    lv_obj_t *l = makeLabel(card, "", F_CAPTION, got ? C_TEXT : C_MUTED);
    lv_obj_set_size(l, 80, 16);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    char b[24];
    if (got) snprintf(b, sizeof(b), "%s", i < 9 ? dgStageName(i) : DL("Trứng lạnh", "Cold egg"));
    else snprintf(b, sizeof(b), "%d", (i + 1) * 10);
    lv_label_set_text(l, b);
    lv_obj_align(l, LV_ALIGN_BOTTOM_MID, 0, -6);
  }
  lv_obj_t *h = makeLabel(s, "", F_CAPTION, C_MUTED);
  lv_obj_set_width(h, 456);
  lv_obj_set_style_text_align(h, LV_TEXT_ALIGN_CENTER, 0);
  char b[96];
  snprintf(b, sizeof(b), isVi() ? "Trả lời đúng %u câu. Cứ 10 câu đúng được một sticker (số dưới hình là số câu cần)." : "%u right answers. Every 10 give a sticker (the number is the answers needed).", (unsigned)gmCTotal);
  lv_label_set_text(h, b);
  lv_obj_set_pos(h, 12, SB_H + 52 + 2 * 116 + 4);
  pageShow(s);
}

// =====================================================================================
// Game 3: maze
// =====================================================================================
// Every level is the same maze each time (seeded by its number), so it can be played again for more stars. The goal is
// the cell farthest from the start. Move with the arrow pad (hold to keep going) or swipe on the maze: a swipe runs
// along the corridor up to the next turn. The maze is drawn in one draw callback (no object per wall: a 24x16 maze
// has ~400 walls, far too many LVGL objects for the RAM).
static const int MZ_MAXC = 24, MZ_MAXR = 16, MZ_CELLS = MZ_MAXC * MZ_MAXR;
static const int MZ_AW = 360, MZ_AH = 250;  // maze area, left of the arrow pad
static const int8_t MZ_DX[4] = {0, 1, 0, -1}, MZ_DY[4] = {-1, 0, 1, 0};  // N E S W; wall bit = 1 << dir
static uint8_t mzWall[MZ_CELLS], mzSeen[MZ_CELLS / 8];
static uint16_t mzDist[MZ_CELLS], mzQ[MZ_CELLS];
static int mzLevel, mzCols, mzRows, mzCell, mzOx, mzOy, mzT, mzPos, mzGoal, mzGem[3], mzGemN, mzGemLeft, mzSteps, mzPar;
static int mzRun = -1, mzHeld = -1;  // corridor run in progress / arrow held down (direction, -1 = none)
static bool mzDark, mzDone;
static uint32_t mzNextAt, mzRnd;
static lv_obj_t *mzScr, *mzArea, *mzMe, *mzExit, *mzGemObj[3], *mzInfo;

static void mzPlay(int level);
static void mzPlayReplace(int level) {
  pageReplace = true;
  mzPlay(level);
  pageReplace = false;
}
static void mzList() { gmLevelList(DL("Mê cung", "Maze"), mzStars, mzPlayReplace); }
static void mzListReplace() {
  pageReplace = true;
  mzList();
  pageReplace = false;
}

static int mzRand(int n) {
  mzRnd = mzRnd * 1664525u + 1013904223u;
  return (mzRnd >> 8) % n;
}
static bool mzOpen(int c, int d) { return !(mzWall[c] & (1 << d)); }
static int mzNext(int c, int d) { return c + MZ_DY[d] * mzCols + MZ_DX[d]; }
static bool mzSeenAt(int c) { return mzSeen[c >> 3] & (1 << (c & 7)); }

// Breadth-first distances from one cell (0xFFFF = not reached); returns the farthest cell.
static int mzBfs(int from) {
  int n = mzCols * mzRows, head = 0, tail = 0, last = from;
  for (int i = 0; i < n; i++) mzDist[i] = 0xFFFF;
  mzDist[from] = 0;
  mzQ[tail++] = from;
  while (head < tail) {
    int c = last = mzQ[head++];
    for (int d = 0; d < 4; d++) {
      if (!mzOpen(c, d)) continue;
      int nc = mzNext(c, d);
      if (mzDist[nc] != 0xFFFF) continue;
      mzDist[nc] = mzDist[c] + 1;
      mzQ[tail++] = nc;
    }
  }
  return last;
}

// Recursive backtracker (with an explicit stack), then a few walls knocked out from level 6 on so there are loops.
static void mzGenerate() {
  int n = mzCols * mzRows;
  memset(mzWall, 15, n);
  for (int i = 0; i < n; i++) mzDist[i] = 0;  // used as "visited" here
  int sp = 0;
  mzQ[sp++] = 0;
  mzDist[0] = 1;
  while (sp) {
    int c = mzQ[sp - 1], x = c % mzCols, y = c / mzCols, opts[4], k = 0;
    for (int d = 0; d < 4; d++) {
      int nx = x + MZ_DX[d], ny = y + MZ_DY[d];
      if (nx >= 0 && ny >= 0 && nx < mzCols && ny < mzRows && !mzDist[ny * mzCols + nx]) opts[k++] = d;
    }
    if (!k) {
      sp--;
      continue;
    }
    int d = opts[mzRand(k)], nc = mzNext(c, d);
    mzWall[c] &= ~(1 << d);
    mzWall[nc] &= ~(1 << ((d + 2) & 3));
    mzDist[nc] = 1;
    mzQ[sp++] = nc;
  }
  if (mzLevel >= 5) {
    for (int i = n / 14; i > 0; i--) {
      int c = mzRand(n), d = mzRand(2) + 1;  // east or south
      int x = c % mzCols, y = c / mzCols;
      if ((d == 1 && x == mzCols - 1) || (d == 2 && y == mzRows - 1)) continue;
      mzWall[c] &= ~(1 << d);
      mzWall[mzNext(c, d)] &= ~(1 << ((d + 2) & 3));
    }
  }
}

static void mzPlace(lv_obj_t *o, int c) {  // centre an object on a cell
  int x = mzOx + (c % mzCols) * mzCell + mzCell / 2, y = mzOy + (c / mzCols) * mzCell + mzCell / 2;
  lv_obj_set_pos(o, x - lv_obj_get_width(o) / 2, y - lv_obj_get_height(o) / 2);
}

// Redraw the cells around one cell (r cells each side), e.g. after the dragon moved.
static void mzInval(int c, int r) {
  lv_area_t a;
  lv_obj_get_coords(mzArea, &a);
  int x = c % mzCols, y = c / mzCols;
  lv_area_t z = {(lv_coord_t)(a.x1 + mzOx + (x - r) * mzCell - mzT), (lv_coord_t)(a.y1 + mzOy + (y - r) * mzCell - mzT),
                 (lv_coord_t)(a.x1 + mzOx + (x + r + 1) * mzCell + mzT), (lv_coord_t)(a.y1 + mzOy + (y + r + 1) * mzCell + mzT)};
  lv_obj_invalidate_area(mzArea, &z);
}

// Dark levels: a cell shows when it was walked or is within 2 cells of the dragon.
static bool mzLit(int c) {
  if (!mzDark || mzSeenAt(c)) return true;
  return abs(c % mzCols - mzPos % mzCols) <= 2 && abs(c / mzCols - mzPos / mzCols) <= 2;
}

static void mzDraw(lv_event_t *e) {
  lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);
  const lv_area_t *clip = ctx->clip_area;
  lv_area_t a;
  lv_obj_get_coords(mzArea, &a);
  int ox = a.x1 + mzOx, oy = a.y1 + mzOy, cs = mzCell, t = mzT;
  int fx = clip->x1 - ox - t, fy = clip->y1 - oy - t;
  int x0 = fx < 0 ? 0 : fx / cs, y0 = fy < 0 ? 0 : fy / cs;
  int x1 = (clip->x2 - ox + t) / cs, y1 = (clip->y2 - oy + t) / cs;
  if (x1 >= mzCols) x1 = mzCols - 1;
  if (y1 >= mzRows) y1 = mzRows - 1;
  lv_draw_rect_dsc_t wall, crumb;
  lv_draw_rect_dsc_init(&wall);
  wall.bg_color = lv_color_hex(mzDark ? 0x7E8CC8 : 0x8FB4FF);
  wall.radius = t / 2;
  lv_draw_rect_dsc_init(&crumb);
  crumb.bg_color = lv_color_hex(0x3A4A86);
  crumb.radius = LV_RADIUS_CIRCLE;
  int cr = cs / 6 < 2 ? 2 : cs / 6, h = t / 2;
  auto rect = [&](lv_draw_rect_dsc_t *d, int x1, int y1, int x2, int y2) {
    lv_area_t r = {(lv_coord_t)x1, (lv_coord_t)y1, (lv_coord_t)x2, (lv_coord_t)y2};
    lv_draw_rect(ctx, d, &r);
  };
  for (int y = y0; y <= y1; y++) {
    for (int x = x0; x <= x1; x++) {
      int c = y * mzCols + x, X = ox + x * cs, Y = oy + y * cs;
      if (!mzLit(c)) continue;
      if (mzSeenAt(c)) rect(&crumb, X + cs / 2 - cr, Y + cs / 2 - cr, X + cs / 2 + cr - 1, Y + cs / 2 + cr - 1);  // way walked
      uint8_t w = mzWall[c];
      if (w & 1) rect(&wall, X - h, Y - h, X + cs - h + t - 1, Y - h + t - 1);
      if (w & 8) rect(&wall, X - h, Y - h, X - h + t - 1, Y + cs - h + t - 1);
      if ((w & 2) && x == mzCols - 1) rect(&wall, X + cs - h, Y - h, X + cs - h + t - 1, Y + cs - h + t - 1);
      if ((w & 4) && y == mzRows - 1) rect(&wall, X - h, Y + cs - h, X + cs - h + t - 1, Y + cs - h + t - 1);
    }
  }
}

static void mzStatus() {
  if (mzGemLeft) setTextFmt(mzInfo, isVi() ? "%d bước\nCòn %d ngọc" : "%d steps\n%d gems left", mzSteps, mzGemLeft);
  else setTextFmt(mzInfo, isVi() ? "%d bước" : "%d steps", mzSteps);
}

static void mzShowLit() {  // dark levels: gems and the exit show up once lit
  if (!mzDark) return;
  if (mzLit(mzGoal)) lv_obj_clear_flag(mzExit, LV_OBJ_FLAG_HIDDEN);
  for (int i = 0; i < mzGemN; i++)
    if (mzGem[i] >= 0 && mzLit(mzGem[i])) lv_obj_clear_flag(mzGemObj[i], LV_OBJ_FLAG_HIDDEN);
}

static void mzWin() {
  int stars = mzSteps <= mzPar * 115 / 100 + 2 ? 3 : mzSteps <= mzPar * 2 + 4 ? 2 : 1, old = mzStars[mzLevel];
  Serial.printf("GAME maze win level %d steps %d par %d stars %d\n", mzLevel + 1, mzSteps, mzPar, stars);
  if (stars > old) {
    mzStars[mzLevel] = stars;
    gamesSave();
    gmCoins((stars - old) * 2);
  }
  char b[64];
  snprintf(b, sizeof(b), isVi() ? "Màn %d  -  %d bước" : "Level %d  -  %d steps", mzLevel + 1, mzSteps);
  gmWinCard(mzScr, mzLevel, stars, b, mzPlayReplace, mzListReplace);
}

// One step; false if a wall is in the way.
static bool mzStep(int d) {
  if (mzDone || !mzOpen(mzPos, d)) return false;
  int old = mzPos;
  mzPos = mzNext(mzPos, d);
  mzSeen[mzPos >> 3] |= 1 << (mzPos & 7);
  mzSteps++;
  mzPlace(mzMe, mzPos);
  mzInval(old, mzDark ? 3 : 0);
  mzInval(mzPos, mzDark ? 3 : 0);
  mzShowLit();
  for (int i = 0; i < mzGemN; i++) {
    if (mzGem[i] != mzPos) continue;
    mzGem[i] = -1;
    lv_obj_add_flag(mzGemObj[i], LV_OBJ_FLAG_HIDDEN);
    mzGemLeft--;
    SFX("gem", SND_COIN);
    if (!mzGemLeft) {  // the exit opens
      lv_obj_set_style_bg_color(mzExit, lv_color_hex(0xFFC93C), 0);
      lv_obj_set_style_border_color(mzExit, lv_color_hex(0xFFF1B0), 0);
      uiToast(DL("Cửa ra đã mở!", "The exit is open!"), C_OK);
    }
  }
  mzStatus();
  if (mzPos == mzGoal) {
    if (mzGemLeft) {
      SFX("error", SND_ERROR);
      uiToast(DL("Nhặt hết ngọc để mở cửa ra", "Pick up all the gems to open the exit"), C_WARN);
    } else {
      mzDone = true;
      mzRun = mzHeld = -1;
      mzNextAt = millis() + 500;
    }
  }
  return true;
}

// A corridor run stops at a junction or dead end, at a gem and at the exit.
static bool mzStopHere() {
  if (mzPos == mzGoal) return true;
  for (int i = 0; i < mzGemN; i++)
    if (mzGem[i] == mzPos) return true;
  int open = 0;
  for (int d = 0; d < 4; d++) open += mzOpen(mzPos, d);
  return open != 2;
}

static void mzTick(lv_timer_t *) {
  uint32_t now = millis();
  if (now < mzNextAt) return;
  if (mzDone) {
    mzNextAt = 0xFFFFFFFF;  // once
    mzWin();
    return;
  }
  if (mzRun >= 0) {
    int d = mzRun;
    if (!mzStep(d) || mzStopHere()) {
      mzRun = -1;
    } else if (!mzOpen(mzPos, d)) {  // follow the bend of the corridor
      for (int k = 0; k < 4; k++)
        if (k != ((d + 2) & 3) && mzOpen(mzPos, k)) mzRun = k;
    }
    if (!mzDone) mzNextAt = now + 55;
  } else if (mzHeld >= 0) {
    mzStep(mzHeld);
    if (!mzDone) mzNextAt = now + 150;
  }
}

static void mzPad(lv_event_t *e) {
  int d = (int)(intptr_t)lv_event_get_user_data(e);
  if (lv_event_get_code(e) == LV_EVENT_PRESSED) {
    if (mzDone) return;
    mzRun = -1;
    mzHeld = d;
    if (!mzStep(d)) SFX("tap", SND_TAP);
    if (!mzDone) mzNextAt = millis() + 320;  // the first repeat comes a little later
  } else if (mzHeld == d) {
    mzHeld = -1;
  }
}

static void mzSwipe(lv_event_t *) {
  lv_dir_t g = lv_indev_get_gesture_dir(lv_indev_get_act());
  int d = g == LV_DIR_TOP ? 0 : g == LV_DIR_RIGHT ? 1 : g == LV_DIR_BOTTOM ? 2 : g == LV_DIR_LEFT ? 3 : -1;
  if (d < 0 || mzDone) return;
  mzHeld = -1;
  mzRun = mzOpen(mzPos, d) ? d : -1;
  if (mzRun < 0) SFX("tap", SND_TAP);
  mzNextAt = 0;
}

static void mzPlay(int level) {
  mzLevel = level;
  mzCols = 5 + level * (MZ_MAXC - 5) / 29;
  mzRows = (mzCols * MZ_AH + MZ_AW / 2) / MZ_AW;
  mzRows = mzRows < 3 ? 3 : mzRows > MZ_MAXR ? MZ_MAXR : mzRows;
  mzCell = MZ_AW / mzCols < MZ_AH / mzRows ? MZ_AW / mzCols : MZ_AH / mzRows;
  mzOx = (MZ_AW - mzCols * mzCell) / 2;
  mzOy = (MZ_AH - mzRows * mzCell) / 2;
  mzT = mzCell >= 30 ? 4 : mzCell >= 20 ? 3 : 2;
  mzDark = level >= 20;
  mzGemN = level >= 20 ? 2 : level >= 10 ? 1 + (level - 10) / 4 : 0;
  if (mzGemN > 3) mzGemN = 3;
  mzRnd = 0x9E3779B9u ^ (uint32_t)(level + 1) * 2654435761u;
  mzGenerate();
  int n = mzCols * mzRows;
  mzPos = 0;
  mzGoal = mzBfs(0);
  for (int i = 0; i < mzGemN; i++) {  // gems: away from the start, not on the exit or on each other
    int c;
    for (int tries = 0;; tries++) {
      c = mzRand(n);
      bool ok = c != 0 && c != mzGoal && mzDist[c] >= 3;
      for (int k = 0; k < i; k++) ok = ok && mzGem[k] != c;
      if (ok || tries > 200) break;
    }
    mzGem[i] = c;
  }
  // par, the walk the stars are measured against: start -> nearest gem -> ... -> exit
  mzPar = 0;
  int at = 0;
  bool got[3] = {false, false, false};
  for (int k = 0; k < mzGemN; k++) {
    mzBfs(at);
    int best = -1;
    for (int i = 0; i < mzGemN; i++)
      if (!got[i] && (best < 0 || mzDist[mzGem[i]] < mzDist[mzGem[best]])) best = i;
    got[best] = true;
    mzPar += mzDist[mzGem[best]];
    at = mzGem[best];
  }
  mzBfs(at);
  mzPar += mzDist[mzGoal];
  mzGemLeft = mzGemN;
  mzSteps = 0;
  mzRun = mzHeld = -1;
  mzDone = false;
  mzNextAt = 0;
  memset(mzSeen, 0, sizeof(mzSeen));
  mzSeen[0] = 1;

  lv_obj_t *s = gmScreen();
  mzScr = s;
  lv_obj_add_event_cb(s, [](lv_event_t *) { mzScr = nullptr; }, LV_EVENT_DELETE, nullptr);
  char b[40];
  snprintf(b, sizeof(b), isVi() ? "Màn %d" : "Level %d", level + 1);
  gmBar(s, b, [](lv_event_t *) { mzListReplace(); });
  lv_obj_t *kind = makeLabel(s, "", F_CAPTION, C_MUTED);
  lv_label_set_text(kind, mzDark   ? DL("Mê cung tối: nhặt ngọc rồi ra cửa", "Dark maze: gems, then the exit")
                          : mzGemN ? DL("Nhặt hết ngọc rồi ra cửa", "Pick up the gems, then the exit")
                                   : DL("Tìm đường tới cửa vàng", "Find the way to the gold door"));
  lv_obj_set_pos(kind, 150, SB_H + 14);

  mzArea = lv_obj_create(s);
  lv_obj_remove_style_all(mzArea);
  lv_obj_set_size(mzArea, MZ_AW, MZ_AH);
  lv_obj_set_pos(mzArea, 6, SB_H + 44);
  lv_obj_set_style_radius(mzArea, 10, 0);
  lv_obj_set_style_bg_color(mzArea, lv_color_hex(mzDark ? 0x05070F : 0x0E1430), 0);
  lv_obj_set_style_bg_opa(mzArea, LV_OPA_COVER, 0);
  lv_obj_clear_flag(mzArea, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(mzArea, LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_add_flag(mzArea, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(mzArea, mzDraw, LV_EVENT_DRAW_MAIN, nullptr);
  lv_obj_add_event_cb(mzArea, mzSwipe, LV_EVENT_GESTURE, nullptr);

  int ex = mzCell * 7 / 10;
  mzExit = shape(mzArea, 0, 0, ex, ex, mzGemN ? 0x4A5378 : 0xFFC93C, 4);  // grey until the gems are picked up
  lv_obj_set_style_border_width(mzExit, 2, 0);
  lv_obj_set_style_border_color(mzExit, lv_color_hex(mzGemN ? 0x8A93B8 : 0xFFF1B0), 0);
  mzPlace(mzExit, mzGoal);
  int gs = mzCell / 2 < 6 ? 6 : mzCell / 2;
  for (int i = 0; i < 3; i++) {
    mzGemObj[i] = shape(mzArea, 0, 0, gs, gs, 0xFF5CC8);
    lv_obj_set_style_border_width(mzGemObj[i], 1, 0);
    lv_obj_set_style_border_color(mzGemObj[i], lv_color_hex(0xFFD6F2), 0);
    if (i < mzGemN) mzPlace(mzGemObj[i], mzGem[i]);
    else lv_obj_add_flag(mzGemObj[i], LV_OBJ_FLAG_HIDDEN);
  }
  int me = mzCell * 3 / 5 < 8 ? 8 : mzCell * 3 / 5;
  mzMe = shape(mzArea, 0, 0, me, me, 0x38E1FF);
  lv_obj_set_style_border_width(mzMe, 2, 0);
  lv_obj_set_style_border_color(mzMe, lv_color_hex(0xFFFFFF), 0);
  mzPlace(mzMe, 0);
  if (mzDark) {
    lv_obj_add_flag(mzExit, LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < mzGemN; i++) lv_obj_add_flag(mzGemObj[i], LV_OBJ_FLAG_HIDDEN);
    mzShowLit();
  }

  // arrow pad on the right (hold an arrow to keep walking)
  const int PX = 372, PW = 102, BS = 34, CX = PX + PW / 2, CY = SB_H + 44 + 150;
  mzInfo = makeLabel(s, "", F_BODY, C_ACCENT);
  lv_obj_set_width(mzInfo, PW);
  lv_obj_set_style_text_align(mzInfo, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(mzInfo, PX, SB_H + 50);
  mzStatus();
  static const char *const SYM[4] = {LV_SYMBOL_UP, LV_SYMBOL_RIGHT, LV_SYMBOL_DOWN, LV_SYMBOL_LEFT};
  for (int d = 0; d < 4; d++) {
    lv_obj_t *btn = lv_obj_create(s);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, BS, BS);
    lv_obj_set_pos(btn, CX - BS / 2 + MZ_DX[d] * BS, CY - BS / 2 + MZ_DY[d] * BS);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x6E55D6), 0);
    lv_obj_set_style_bg_color(btn, C_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_ext_click_area(btn, 4);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *l = makeLabel(btn, SYM[d], F_LABEL, C_TEXT);
    lv_obj_center(l);
    for (lv_event_code_t c : {LV_EVENT_PRESSED, LV_EVENT_RELEASED, LV_EVENT_PRESS_LOST})
      lv_obj_add_event_cb(btn, mzPad, c, (void *)(intptr_t)d);
  }
  lv_obj_t *h = makeLabel(s, DL("Vuốt trên mê cung để chạy nhanh", "Swipe on the maze to run"), F_CAPTION, C_MUTED);
  lv_obj_set_width(h, PW);
  lv_obj_set_style_text_align(h, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(h, PX, CY + BS * 3 / 2 + 8);

  pageTimer(s, mzTick, 20);
  pageShow(s);
}

// =====================================================================================
// The hub
// =====================================================================================
static void pageGames() {
  gamesLoad();
  gmFacesInit();
  lv_obj_t *c;
  lv_obj_t *s = pageBegin("Games", &c);
  lv_obj_del(c);
  gmHub = s;
  lv_obj_add_event_cb(s, [](lv_event_t *) {
    if (pageParent == gmHub) pageParent = nullptr;
    gmHub = nullptr;
  }, LV_EVENT_DELETE, nullptr);

  struct Card {
    const char *vi, *en;
    const lv_img_dsc_t *icon;
    void (*open)();
    uint32_t col;
  };
  char sub[3][48];
  snprintf(sub[0], sizeof(sub[0]), isVi() ? "%d/90 sao  -  30 màn" : "%d/90 stars  -  30 levels", gmStarTotal());
  snprintf(sub[1], sizeof(sub[1]), isVi() ? "Bậc %d  -  %d/10 sticker" : "Step %d  -  %d/10 stickers", gmCLevel, gmStickers());
  snprintf(sub[2], sizeof(sub[2]), isVi() ? "%d/90 sao  -  30 màn" : "%d/90 stars  -  30 levels", gmStarTotal(mzStars));
  const Card cards[3] = {{"Lật hình", "Memory match", DRAGON_SPRITES[2][1], []() { pageParent = gmHub; memList(); }, 0x6E55D6},
                         {"Đếm cùng rồng", "Count with the dragon", DRAGON_SPRITES[4][1], []() { pageParent = gmHub; cgPlay(); }, 0x1FA39A},
                         {"Mê cung", "Maze", DRAGON_SPRITES[6][1], []() { pageParent = gmHub; mzList(); }, 0xC9772E}};
  const int CW = 148;
  for (int i = 0; i < 3; i++) {
    lv_obj_t *card = lv_obj_create(s);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, CW, 188);
    lv_obj_set_pos(card, 12 + i * (CW + 8), SB_H + 52);
    lv_obj_set_style_radius(card, 20, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(cards[i].col), 0);
    lv_obj_set_style_bg_grad_color(card, lv_color_hex(0x151B33), 0);
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(cards[i].col), 0);
    lv_obj_set_style_opa(card, LV_OPA_70, LV_STATE_PRESSED);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *img = gmSprite(card, cards[i].icon, 80);
    lv_obj_set_pos(img, CW / 2 - 20, 22);
    lv_obj_t *t = makeLabel(card, "", F_LABEL, C_TEXT);
    lv_label_set_text(t, isVi() ? cards[i].vi : cards[i].en);
    lv_obj_set_width(t, CW - 12);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 112);
    lv_obj_t *d = makeLabel(card, sub[i], F_CAPTION, lv_color_hex(0xD9DEF5));
    lv_obj_set_width(d, CW - 12);
    lv_obj_set_style_text_align(d, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(d, LV_ALIGN_TOP_MID, 0, 140);
    lv_obj_add_event_cb(card, [](lv_event_t *e) {
      soundPlay(SND(SND_TAP), 0, false);
      ((void (*)())lv_event_get_user_data(e))();
    }, LV_EVENT_CLICKED, (void *)cards[i].open);
  }
  lv_obj_t *h = makeLabel(s, "", F_CAPTION, C_MUTED);
  lv_obj_set_width(h, 456);
  lv_obj_set_style_text_align(h, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(h, DL("Chơi mỗi ngày để mở thêm màn, lên bậc và sưu tầm đủ 10 sticker rồng. Hai game bắt ngọc và rồng bay nằm trong app Rồng.",
                          "Play every day to open levels, move up and collect all 10 dragon stickers. The gem and flying games are inside the Dragon app."));
  lv_obj_set_pos(h, 12, SB_H + 52 + 196);
  pageShow(s);
}
