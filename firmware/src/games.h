#pragma once
// Games hub with two games for children made to be played for months:
//
//  * "Lật hình" (memory match): 30 levels from 3 to 12 pairs. The cards show the pixel-art dragons and treats of the
//    dragon game; they are shown for a moment, then turned down. 1 to 3 stars by the number of flips; a level opens
//    when the one before it is done. Cards are drawn at random every time, so a level is never the same twice.
//  * "Đếm cùng rồng" (counting and sums): ten steps, from counting 1-3 objects up to the times tables, that adapt to the
//    child (three misses in a row go one step back, ten right answers go one step up). Every 10 right answers give a
//    sticker of a dragon for the album (10 stickers) and 5 coins for the real dragon.
//
// Progress is kept in NVS namespace "games". Included by main.cpp after dragon.h and home.h (sprites, coins).
#include <algorithm>

// ---------- saved progress ----------
static uint8_t gmStars[30];      // stars per memory level (0 = not done)
static uint8_t gmCLevel = 1;     // counting step 1..10
static uint8_t gmCOk = 0;        // right answers on this step
static uint32_t gmCTotal = 0;    // right answers ever (10 of them = a sticker)

static void gamesLoad() {
  Preferences p;
  p.begin("games", true);
  memset(gmStars, 0, sizeof(gmStars));
  p.getBytes("ms", gmStars, sizeof(gmStars));
  gmCLevel = constrain(p.getUChar("cl", 1), 1, 10);
  gmCOk = p.getUChar("co", 0);
  gmCTotal = p.getUInt("ct", 0);
  p.end();
  for (auto &s : gmStars) s = s > 3 ? 3 : s;
}
static void gamesSave() {
  Preferences p;
  p.begin("games", false);
  p.putBytes("ms", gmStars, sizeof(gmStars));
  p.putUChar("cl", gmCLevel);
  p.putUChar("co", gmCOk);
  p.putUInt("ct", gmCTotal);
  p.end();
}
static int gmStarTotal() {
  int n = 0;
  for (auto s : gmStars) n += s;
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
  SFX("win", SND_OK);
  lv_obj_t *ov = gmOverlay(mcScr);
  lv_obj_t *card = pxPanel(ov, 360, 196);
  lv_obj_center(card);
  lv_obj_t *t = pxText(card, DL("Hoan hô!", "Well done!"), F_HEADING);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 10);
  gmStarDots(card, 360 / 2 - 40 - 10, 52, stars, 26);
  char b[64];
  snprintf(b, sizeof(b), isVi() ? "Màn %d  -  %d lượt lật" : "Level %d  -  %d flips", mcLevel + 1, mcMoves);
  lv_obj_t *l = pxText(card, b, F_BODY);
  lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 92);
  bool hasNext = mcLevel < 29;
  lv_obj_t *next = pxButton(card, 108, 40, nullptr, DL("Màn sau", "Next"), [](lv_event_t *) { memPlayReplace(mcLevel < 29 ? mcLevel + 1 : mcLevel); }, nullptr);
  lv_obj_align(next, LV_ALIGN_BOTTOM_LEFT, 10, -10);
  if (!hasNext) lv_obj_add_flag(next, LV_OBJ_FLAG_HIDDEN);
  lv_obj_t *again = pxButton(card, 108, 40, nullptr, DL("Chơi lại", "Again"), [](lv_event_t *) { memPlayReplace(mcLevel); }, nullptr);
  lv_obj_align(again, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_t *list = pxButton(card, 108, 40, nullptr, DL("Danh sách", "Levels"), [](lv_event_t *) { memListReplace(); }, nullptr);
  lv_obj_align(list, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
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

static void memList() {
  gamesLoad();
  lv_obj_t *s = gmScreen();
  gmBar(s, DL("Lật hình", "Memory match"), goBack);
  char b[32];
  snprintf(b, sizeof(b), isVi() ? "%d/90 sao" : "%d/90 stars", gmStarTotal());
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
  while (next < 29 && gmStars[next]) next++;
  lv_obj_t *focus = nullptr;
  for (int i = 0; i < 30; i++) {
    bool open = i == 0 || gmStars[i - 1] > 0;
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
    if (open) gmStarDots(btn, 68 / 2 - 20, 38, gmStars[i], 10);
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
      int i = (int)(intptr_t)lv_event_get_user_data(e);
      if (i == 0 || gmStars[i - 1] > 0) memPlayReplace(i);
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
  char sub[2][48];
  snprintf(sub[0], sizeof(sub[0]), isVi() ? "%d/90 sao  -  30 màn" : "%d/90 stars  -  30 levels", gmStarTotal());
  snprintf(sub[1], sizeof(sub[1]), isVi() ? "Bậc %d  -  %d/10 sticker" : "Step %d  -  %d/10 stickers", gmCLevel, gmStickers());
  const Card cards[2] = {{"Lật hình", "Memory match", DRAGON_SPRITES[2][1], []() { pageParent = gmHub; memList(); }, 0x6E55D6},
                         {"Đếm cùng rồng", "Count with the dragon", DRAGON_SPRITES[4][1], []() { pageParent = gmHub; cgPlay(); }, 0x1FA39A}};
  for (int i = 0; i < 2; i++) {
    lv_obj_t *card = lv_obj_create(s);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, 222, 188);
    lv_obj_set_pos(card, 12 + i * 234, SB_H + 52);
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
    lv_obj_set_pos(img, 222 / 2 - 20, 22);
    lv_obj_t *t = makeLabel(card, "", F_TITLE, C_TEXT);
    lv_label_set_text(t, isVi() ? cards[i].vi : cards[i].en);
    lv_obj_set_width(t, 206);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 112);
    lv_obj_t *d = makeLabel(card, sub[i], F_CAPTION, lv_color_hex(0xD9DEF5));
    lv_obj_set_width(d, 206);
    lv_obj_set_style_text_align(d, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(d, LV_ALIGN_TOP_MID, 0, 146);
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
