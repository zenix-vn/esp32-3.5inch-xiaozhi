#pragma once
// Virtual dragon pet ("Nuôi rồng ảo"): a real-time game. Needs keep draining while the device runs and the
// time the device was off is caught up from the wall clock. Progress lives on the microSD card
// (/zin100/dragon.json). Design and story: see docs.md.
// Included by main.cpp after the weather/shape helpers (uses shape(), makeLabel(), pageBegin(), sdOk, ...).
#include <SD.h>
#include <ArduinoJson.h>
#include "dragon_img.h"

#define DL(vi, en) (isVi() ? (vi) : (en))

enum { ST_EGG, ST_BABY, ST_L1, ST_L2, ST_L3, ST_ADULT, ST_KING, ST_EMPEROR, ST_DIVINE, ST_COUNT };

struct StageInfo {
  const char *vi, *en;
  float need;  // healthy minutes spent in this stage before it evolves into the next one
  uint32_t body, belly, accent;
};
static const StageInfo STAGES[ST_COUNT] = {
    {"Trứng", "Egg", 10, 0xFFF1D6, 0xFFFFFF, 0xFFC93C},
    {"Rồng con", "Hatchling", 120, 0x7BE495, 0xE9FFD6, 0xFFC93C},
    {"Rồng cấp 1", "Dragon Lv.1", 480, 0x4FD1C5, 0xDFFBF7, 0xFFC93C},
    {"Rồng cấp 2", "Dragon Lv.2", 1440, 0x4C9AFF, 0xE3EEFF, 0xFF9F43},
    {"Rồng cấp 3", "Dragon Lv.3", 2880, 0x7C6BFF, 0xEAE6FF, 0xFF9F43},
    {"Rồng trưởng thành", "Adult Dragon", 5760, 0xB05CFF, 0xF3E3FF, 0xFFD23C},
    {"Rồng vương", "Dragon King", 10080, 0xE0457B, 0xFFE0EA, 0xFFD23C},
    {"Rồng hoàng", "Dragon Emperor", 14400, 0xFF8A3D, 0xFFEBD6, 0xFFF07A},
    {"Rồng thần", "Divine Dragon", 0, 0xFFD54A, 0xFFF8DA, 0x38E1FF},
};

static const char *STORY_VI[ST_COUNT] = {
    "Đêm sao băng, một quả trứng cổ phát sáng rơi xuống bàn của bạn. Hãy giữ ấm cho nó - Long Giới đang chờ một người bạn.",
    "Tách! Vỏ trứng nứt ra. Một chú rồng con dụi mắt nhìn bạn và kêu khẽ. Bạn là người đầu tiên nó thấy trong đời.",
    "Rồng con đã biết bay những vòng đầu tiên! Đôi cánh nhỏ bắt đầu mọc và ngọn lửa đầu tiên le lói trên mũi.",
    "Vảy rồng chuyển sang sắc xanh dương. Rồng biết phun lửa nhỏ và thích nghe bạn kể chuyện mỗi tối.",
    "Sừng đã dài ra, tiếng gầm vang hơn. Rồng không còn sợ sấm sét nữa - nó còn gọi được mưa về.",
    "Rồng trưởng thành! Đôi cánh sải rộng che cả căn phòng. Cả Long Giới đã nghe thấy tiếng gầm của nó.",
    "Rồng vương khoác vảy đỏ rực. Các rồng non kéo đến xin được che chở dưới đôi cánh của nó.",
    "Rồng hoàng đội vương miện vàng. Mỗi lần vẫy cánh đều rải xuống những đốm lửa vàng lấp lánh.",
    "Ngọc rồng thần thức tỉnh! Rồng hóa thành rồng thần và bay lên mây ngũ sắc. Nó mãi nhớ người đã ấp mình từ quả trứng nhỏ.",
};
static const char *STORY_EN[ST_COUNT] = {
    "On a night of shooting stars, a glowing ancient egg lands on your desk. Keep it warm - the Dragon Realm is waiting for a friend.",
    "Crack! The shell splits open. A baby dragon rubs its eyes, looks at you and chirps. You are the first face it has ever seen.",
    "The hatchling takes its first flights! Tiny wings sprout and the first flame flickers on its snout.",
    "Its scales turn blue. The dragon can breathe little flames and loves listening to your stories every evening.",
    "Its horns have grown and its roar is louder. It no longer fears thunder - it can even call the rain.",
    "A grown dragon! Its wings spread wide enough to cover the room. The whole Dragon Realm has heard its roar.",
    "The Dragon King wears blazing red scales. Young dragons gather to ask for shelter under its wings.",
    "The Dragon Emperor wears a golden crown. Every wingbeat scatters sparkling golden embers.",
    "The Divine Dragon Pearl awakens! The dragon rises into the five-colour clouds. It will always remember who hatched it from a tiny egg.",
};

struct DragonSave {
  String name;
  uint8_t stage = ST_EGG;
  float growth = 0;                                      // healthy minutes in the current stage
  float satiety = 80, mood = 80, clean = 80, energy = 60;  // 0..100 (energy doubles as warmth for the egg)
  float lowMin = 0;                                      // minutes with a need below 10
  bool sick = false, sleeping = false, intro = false;
  uint32_t coins = 30, lastTs = 0, bornTs = 0, dailyDay = 0;
  uint16_t gen = 1, best = 0;
  uint32_t fed = 0, played = 0, bathed = 0, meds = 0;
};
static DragonSave dg;
static bool dgOk = false;           // microSD present and the save is loaded
static bool dgNeedCatchup = false;  // apply the time the device was off once the clock is known
static int dgPendingEvolve = -1;    // stage reached and not yet shown to the player
static float dgSpeed = 1;           // growth multiplier (testing only, resets on reboot)
static uint32_t dgLastMs = 0, dgLastSave = 0;
static String dgToast;
static uint32_t dgToastUntil = 0;

static const char *DG_DIR = "/zin100";
static const char *DG_FILE = "/zin100/dragon.json";
static const char *DG_TMP = "/zin100/dragon.tmp";

// ---------- helpers ----------
static bool dgTimeValid() { return time(nullptr) > 1700000000; }
static uint32_t dgDayStamp() {
  struct tm t;
  if (!dgTimeValid() || !getLocalTime(&t, 0)) return 0;
  return (t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday;
}
static int dgHour() {
  struct tm t;
  if (!dgTimeValid() || !getLocalTime(&t, 0)) return -1;
  return t.tm_hour;
}
static bool dgNight() {
  int h = dgHour();
  return h >= 0 && (h >= 22 || h < 6);
}
static float dgClamp(float v) { return v < 0 ? 0 : v > 100 ? 100 : v; }
static const char *dgStageName(int st) { return isVi() ? STAGES[st].vi : STAGES[st].en; }
static String dgName() { return dg.name.length() ? dg.name : String(DL("Tiểu Long", "Draco")); }
static float dgNeed() {
  float bonus = 1.0f - 0.05f * (dg.gen > 1 ? (dg.gen - 1 > 4 ? 4 : dg.gen - 1) : 0);  // each past life hatches/grows faster
  return STAGES[dg.stage].need * bonus;
}
static void dgSay(const String &m) {
  dgToast = m;
  dgToastUntil = millis() + 4000;
}

// ---------- save / load (microSD) ----------
static void dgSave() {
  if (!dgOk) return;
  if (dgTimeValid()) dg.lastTs = time(nullptr);
  JsonDocument d;
  d["v"] = 1;
  d["name"] = dg.name;
  d["stage"] = dg.stage;
  d["growth"] = dg.growth;
  d["satiety"] = dg.satiety;
  d["mood"] = dg.mood;
  d["clean"] = dg.clean;
  d["energy"] = dg.energy;
  d["lowMin"] = dg.lowMin;
  d["sick"] = dg.sick;
  d["sleeping"] = dg.sleeping;
  d["intro"] = dg.intro;
  d["coins"] = dg.coins;
  d["lastTs"] = dg.lastTs;
  d["bornTs"] = dg.bornTs;
  d["dailyDay"] = dg.dailyDay;
  d["gen"] = dg.gen;
  d["best"] = dg.best;
  d["fed"] = dg.fed;
  d["played"] = dg.played;
  d["bathed"] = dg.bathed;
  d["meds"] = dg.meds;
  SD.mkdir(DG_DIR);
  File f = SD.open(DG_TMP, FILE_WRITE);
  if (!f) {
    Serial.println("DRAGON save failed (open)");
    return;
  }
  size_t n = serializeJson(d, f);
  f.close();
  if (n == 0) return;
  SD.remove(DG_FILE);  // write-then-rename so a power cut never leaves a half-written save
  SD.rename(DG_TMP, DG_FILE);
  dgLastSave = millis();
}

static bool dgLoadFile(const char *path) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  JsonDocument d;
  DeserializationError err = deserializeJson(d, f);
  f.close();
  if (err) return false;
  dg.name = d["name"] | "";
  dg.stage = constrain((int)(d["stage"] | 0), 0, ST_COUNT - 1);
  dg.growth = d["growth"] | 0.0f;
  dg.satiety = dgClamp(d["satiety"] | 80.0f);
  dg.mood = dgClamp(d["mood"] | 80.0f);
  dg.clean = dgClamp(d["clean"] | 80.0f);
  dg.energy = dgClamp(d["energy"] | 60.0f);
  dg.lowMin = d["lowMin"] | 0.0f;
  dg.sick = d["sick"] | false;
  dg.sleeping = d["sleeping"] | false;
  dg.intro = d["intro"] | false;
  dg.coins = d["coins"] | 30;
  dg.lastTs = d["lastTs"] | 0;
  dg.bornTs = d["bornTs"] | 0;
  dg.dailyDay = d["dailyDay"] | 0;
  dg.gen = d["gen"] | 1;
  dg.best = d["best"] | 0;
  dg.fed = d["fed"] | 0;
  dg.played = d["played"] | 0;
  dg.bathed = d["bathed"] | 0;
  dg.meds = d["meds"] | 0;
  return true;
}

// ---------- simulation ----------
static void dgEvolveCheck() {
  while (dg.stage < ST_DIVINE && dg.growth >= dgNeed()) {
    dg.growth -= dgNeed();
    dg.stage++;
    dg.coins += 50 * dg.stage;
    dg.mood = 100;
    if (dg.stage == ST_BABY) {  // hatching: the egg's warmth turns into a normal energy bar
      dg.satiety = 70;
      dg.clean = 90;
      dg.energy = 80;
    }
    dgPendingEvolve = dg.stage;
  }
  if (dg.stage == ST_DIVINE) dg.growth = 0;
}

// `min` minutes pass; factor 1 = device running, 0.5 = device off (the dragon rests, everything slows down).
static void dgAdvance(float min, float factor) {
  if (!dgOk || min <= 0) return;
  float h = min / 60.0f * factor;
  if (dg.stage == ST_EGG) {
    dg.energy = dgClamp(dg.energy - 6 * h);  // warmth
    if (dg.energy > 20) dg.growth += min * factor * dgSpeed;
  } else {
    if (dg.sleeping) {
      dg.energy = dgClamp(dg.energy + 18 * h);
      dg.satiety = dgClamp(dg.satiety - 2 * h);
      dg.mood = dgClamp(dg.mood - 1 * h);
      dg.clean = dgClamp(dg.clean - 1 * h);
      if (dg.energy >= 100) dg.sleeping = false;
    } else {
      dg.satiety = dgClamp(dg.satiety - 6 * h);
      dg.mood = dgClamp(dg.mood - (dg.sick ? 8 : 4) * h);
      dg.clean = dgClamp(dg.clean - 3 * h);
      dg.energy = dgClamp(dg.energy - (dgNight() ? 6 : 3) * h);
    }
    bool low = dg.satiety < 10 || dg.mood < 10 || dg.clean < 10 || dg.energy < 10;
    if (low) {
      dg.lowMin += min * factor;
      if (dg.lowMin > 30 && !dg.sick) {
        dg.sick = true;
        dgSay(DL("Rồng bị ốm rồi!", "Your dragon got sick!"));
      }
    } else {
      dg.lowMin = 0;
    }
    if (!dg.sick && dg.satiety > 25 && dg.mood > 25 && dg.clean > 25 && dg.energy > 25) dg.growth += min * factor * dgSpeed;
  }
  dgEvolveCheck();
}

static void dgCatchUp() {
  dgNeedCatchup = false;
  uint32_t now = time(nullptr);
  if (dg.lastTs && now > dg.lastTs) {
    float hours = (now - dg.lastTs) / 3600.0f;
    if (hours > 72) hours = 72;  // away longer than 3 days counts as 3 days
    int full = (int)hours;
    for (int i = 0; i < full; i++) dgAdvance(60, 0.5f);
    dgAdvance((hours - full) * 60, 0.5f);
    if (hours >= 1) dgSay(DL("Rồng nhớ bạn lắm!", "Your dragon missed you!"));
  }
  dg.lastTs = now;
}

static void dgDailyCheck() {
  uint32_t day = dgDayStamp();
  if (!day || day == dg.dailyDay || dg.stage == ST_EGG) return;
  dg.dailyDay = day;
  uint32_t bonus = 10 + 5 * dg.stage + (dg.stage == ST_DIVINE ? 100 : 0);
  dg.coins += bonus;
  dgSay(String(DL("Quà hằng ngày: +", "Daily gift: +")) + bonus + DL(" xu", " coins"));
  dgSave();
}

static void dgTick(lv_timer_t *) {
  if (!dgOk) return;
  uint32_t ms = millis();
  float dtMin = (ms - dgLastMs) / 60000.0f;
  dgLastMs = ms;
  if (dgNeedCatchup && dgTimeValid()) dgCatchUp();
  dgAdvance(dtMin, 1.0f);
  dgDailyCheck();
  if (dg.stage > ST_EGG && !dg.sleeping && random(0, 1800) == 0) {  // roughly one find every 30 minutes
    dg.coins += 3;
    dg.mood = dgClamp(dg.mood + 5);
    dgSay(DL("Rồng nhặt được 3 xu!", "Your dragon found 3 coins!"));
  }
  if (ms - dgLastSave > 60000) dgSave();
}

static void dgNewEgg(bool keepLegacy) {
  uint16_t gen = dg.gen, best = dg.best;
  uint32_t coins = dg.coins, fed = dg.fed, played = dg.played, bathed = dg.bathed, meds = dg.meds;
  String name = dg.name;
  dg = DragonSave();
  dg.intro = keepLegacy;  // a reborn egg skips the opening story
  if (keepLegacy) {
    dg.gen = gen + 1;
    dg.best = best;
    dg.coins = coins;
    dg.fed = fed;
    dg.played = played;
    dg.bathed = bathed;
    dg.meds = meds;
    dg.name = name;
  }
  dg.bornTs = dgTimeValid() ? time(nullptr) : 0;
  dgPendingEvolve = -1;
  dgSave();
}

static void dgSetName(const String &n) {
  String v = n;
  v.trim();
  if (v.length() > 20) v = v.substring(0, 20);
  dg.name = v;
  dgSave();
}

static void dragonBegin() {
  if (!sdOk) return;
  dgOk = true;
  if (!dgLoadFile(DG_FILE) && !dgLoadFile(DG_TMP)) {
    dgNewEgg(false);
  } else {
    dgNeedCatchup = true;
  }
  dgLastMs = millis();
  lv_timer_create(dgTick, 1000, nullptr);
  Serial.printf("DRAGON stage=%d growth=%.1f gen=%d\n", dg.stage, dg.growth, dg.gen);
}

// ---------- actions ----------
static bool dgBlockedAsleep() {
  if (!dg.sleeping) return false;
  dgSay(DL("Rồng đang ngủ, đừng đánh thức!", "Shh, your dragon is asleep!"));
  return true;
}
static void dgWarm() {
  dg.energy = dgClamp(dg.energy + 25);
  dg.growth += 0.5f;
  dgEvolveCheck();
  dgSay(DL("Quả trứng ấm lên.", "The egg feels warmer."));
  dgSave();
}
static void dgFeed(int kind) {
  if (dgBlockedAsleep()) return;
  if (dg.satiety > 90) {
    dg.mood = dgClamp(dg.mood - 8);
    dgSay(DL("No căng rồi, không ăn nổi nữa!", "Too full to eat any more!"));
    return;
  }
  static const int cost[3] = {0, 5, 15};
  if (dg.coins < (uint32_t)cost[kind]) {
    dgSay(DL("Không đủ xu.", "Not enough coins."));
    return;
  }
  dg.coins -= cost[kind];
  if (kind == 0) {
    dg.satiety = dgClamp(dg.satiety + 20);
  } else if (kind == 1) {
    dg.satiety = dgClamp(dg.satiety + 45);
    dg.mood = dgClamp(dg.mood + 5);
  } else {
    dg.satiety = dgClamp(dg.satiety + 30);
    dg.mood = dgClamp(dg.mood + 20);
    dg.growth += 2;
    dgEvolveCheck();
  }
  dg.fed++;
  dgSay(DL("Ngon quá!", "Yummy!"));
  dgSave();
}
static void dgBath() {
  if (dgBlockedAsleep()) return;
  dg.clean = dgClamp(dg.clean + 45);
  dg.mood = dgClamp(dg.mood + 3);
  dg.bathed++;
  dgSay(DL("Sạch bong, thơm phức!", "Squeaky clean!"));
  dgSave();
}
static void dgToggleSleep() {
  if (dg.sleeping) {
    dg.sleeping = false;
    dgSay(DL("Rồng thức dậy.", "Your dragon woke up."));
  } else if (dg.energy > 90) {
    dgSay(DL("Rồng chưa buồn ngủ.", "Not sleepy yet."));
    return;
  } else {
    dg.sleeping = true;
  }
  dgSave();
}
static void dgMedicine() {
  if (!dg.sick) {
    dgSay(DL("Rồng đang khỏe mạnh.", "Your dragon is healthy."));
    return;
  }
  if (dg.coins < 20) {
    dgSay(DL("Thuốc giá 20 xu.", "Medicine costs 20 coins."));
    return;
  }
  dg.coins -= 20;
  dg.sick = false;
  dg.lowMin = 0;
  dg.satiety = dg.satiety < 30 ? 30 : dg.satiety;
  dg.mood = dg.mood < 30 ? 30 : dg.mood;
  dg.clean = dg.clean < 30 ? 30 : dg.clean;
  dg.energy = dg.energy < 30 ? 30 : dg.energy;
  dg.meds++;
  dgSay(DL("Rồng khỏe lại rồi!", "Your dragon feels better!"));
  dgSave();
}
static void dgFinishPlay(int score) {
  dg.coins += score;
  dg.mood = dgClamp(dg.mood + 8 + score * 3);
  dg.energy = dgClamp(dg.energy - 8);
  dg.growth += score / 2.0f > 5 ? 5 : score / 2.0f;
  if (score > dg.best) dg.best = score;
  dg.played++;
  dgEvolveCheck();
  dgSave();
}

// =====================================================================================
// Drawing: pre-rendered sprites (tools/gen_dragons.py) with live eyes on top
// =====================================================================================
static const lv_img_dsc_t *const DRAGON_SPRITES[8] = {&dragon_baby,  &dragon_lv1,  &dragon_lv2,     &dragon_lv3,
                                                       &dragon_adult, &dragon_king, &dragon_emperor, &dragon_divine};
static const int DG_BOX_W = 292, DG_BOX_H = 148, DG_GROUND = 4;  // picture box and feet margin

static lv_obj_t *dgSprite(lv_obj_t *p, const lv_img_dsc_t *src, int x, int y) {
  lv_obj_t *img = lv_img_create(p);
  lv_img_set_src(img, src);
  lv_obj_set_pos(img, x, y);
  lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
  return img;
}

static void dgDrawEgg(lv_obj_t *p) {
  int prog = dgNeed() > 0 ? (int)(dg.growth * 100 / dgNeed()) : 0;
  const lv_img_dsc_t *src = dg.energy < 30 ? &egg_cold : prog > 80 ? &egg_crack2 : prog > 40 ? &egg_crack1 : &egg_warm;
  dgSprite(p, src, (DG_BOX_W - src->header.w) / 2, DG_BOX_H - DG_GROUND - src->header.h);
}

static void dgDrawDragon(lv_obj_t *p, int st, bool sleeping, bool sick, int moodLv) {
  const lv_img_dsc_t *src = DRAGON_SPRITES[st - 1];
  int x = (DG_BOX_W - src->header.w) / 2 + 6, y = DG_BOX_H - DG_GROUND - src->header.h;
  lv_obj_t *img = dgSprite(p, src, x, y);
  if (sick) {
    lv_obj_set_style_img_recolor(img, lv_color_hex(0x7FA060), 0);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_40, 0);
  }
  int ex = x + (int)DRAGON_EYE[st - 1][0], ey = y + (int)DRAGON_EYE[st - 1][1];
  int r = (int)(DRAGON_EYE[st - 1][2] + 0.5f);
  const uint32_t ink = 0x241A3A;
  if (sleeping) {
    shape(p, ex - r, ey, 2 * r + 1, 2, ink, 1);
    lv_obj_t *z = makeLabel(p, "Z z", F_LABEL, C_ACCENT);
    lv_obj_set_pos(z, ex + 2 * r, ey - 6 * r);
  } else if (moodLv == 2) {  // happy: closed "^" eye
    lv_obj_t *a = lv_arc_create(p);
    lv_obj_remove_style(a, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(a, LV_OBJ_FLAG_CLICKABLE);
    lv_arc_set_bg_angles(a, 200, 340);
    lv_obj_set_style_arc_width(a, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(a, lv_color_hex(ink), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_pad_all(a, 0, 0);
    lv_obj_set_size(a, 2 * r + 2, 2 * r + 2);
    lv_obj_set_pos(a, ex - r - 1, ey - r + r / 2);
  } else {
    shape(p, ex - r, ey - r - 1, 2 * r + 1, 2 * r + 2, 0xFFFFFF);
    int py = moodLv == 0 ? ey - r / 2 + 2 : ey - r / 2 - 1;
    shape(p, ex - r / 2 + 1, py, r + 1, r + 3, ink);
    shape(p, ex, py + 1, 2, 2, 0xFFFFFF);
    if (moodLv == 0) shape(p, ex - r / 2, ey + r + 2, r, r + 3, 0x6BB5FF);  // tear
  }
  if (sick) shape(p, ex + 3 * r, ey - 5 * r, r + 2, r + 5, 0x6BB5FF);  // sweat drop
}

// =====================================================================================
// Game page
// =====================================================================================
static lv_obj_t *dScr = nullptr, *dBox, *dBob, *dCoins, *dStageLbl, *dMsg, *dXpBar, *dXpTxt, *dReinc;
static lv_obj_t *dBtns[5], *dBarLbl[4], *dBars[4], *dOv = nullptr;
static int dSig = -1;

struct MiniGame {
  lv_obj_t *ov = nullptr, *jewel = nullptr, *lblScore = nullptr, *lblTime = nullptr, *btn = nullptr;
  lv_timer_t *t = nullptr;
  int score = 0;
  uint32_t startMs = 0, nextSpawn = 0, jewelDie = 0;
  bool over = false;
};
static MiniGame mg;

static void dgCloseOverlay() {
  if (dOv) lv_obj_del(dOv);
  dOv = nullptr;
}
static void dgOverlayDone(lv_event_t *) { dgCloseOverlay(); }

static lv_obj_t *dgOpenOverlay() {
  dgCloseOverlay();
  dOv = lv_obj_create(dScr);
  lv_obj_remove_style_all(dOv);
  lv_obj_set_size(dOv, 480, 320);
  lv_obj_set_style_bg_color(dOv, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(dOv, LV_OPA_70, 0);
  lv_obj_add_flag(dOv, LV_OBJ_FLAG_CLICKABLE);  // swallow touches
  return dOv;
}

static void dgShowStory(int idx, bool evolved) {
  if (!dScr || idx < 0 || idx >= ST_COUNT) return;
  lv_obj_t *ov = dgOpenOverlay();
  lv_obj_t *card = makeCard(ov, 400, 236);
  lv_obj_center(card);
  lv_obj_t *t = makeLabel(card, evolved ? DL("Tiến hóa!", "Evolution!") : DL("Quả trứng bí ẩn", "A mysterious egg"), F_HEADING, C_ACCENT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 14);
  lv_obj_t *n = makeLabel(card, dgStageName(idx), F_TITLE, C_TEXT);
  lv_obj_align(n, LV_ALIGN_TOP_MID, 0, 54);
  lv_obj_t *s = makeLabel(card, isVi() ? STORY_VI[idx] : STORY_EN[idx], F_BODY, C_MUTED);
  lv_obj_set_width(s, 360);
  lv_label_set_long_mode(s, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(s, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(s, LV_ALIGN_TOP_MID, 0, 88);
  lv_obj_t *b = smallBtn(card, DL("Tuyệt vời!", "Awesome!"), C_ACCENT, dgOverlayDone, nullptr, 150);
  lv_obj_align(b, LV_ALIGN_BOTTOM_MID, 0, -12);
}

// ---- mini game: catch the jewels ----
static void mgEnd();
static void mgJewelTap(lv_event_t *e) {
  if (mg.over) return;
  mg.score++;
  lv_label_set_text_fmt(mg.lblScore, "%d", mg.score);
  lv_obj_del_async((lv_obj_t *)lv_event_get_target(e));
  mg.jewel = nullptr;
}
static void mgSpawn() {
  mg.jewel = lv_obj_create(mg.ov);
  lv_obj_remove_style_all(mg.jewel);
  lv_obj_set_size(mg.jewel, 58, 58);
  lv_obj_set_pos(mg.jewel, 20 + random(0, 380), 70 + random(0, 190));
  lv_obj_set_style_radius(mg.jewel, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(mg.jewel, LV_OPA_COVER, 0);
  static const uint32_t cols[] = {0x38E1FF, 0xFF6B9A, 0xFFC93C, 0x3DEBA0, 0xA16BFF};
  lv_obj_set_style_bg_color(mg.jewel, lv_color_hex(cols[random(0, 5)]), 0);
  lv_obj_set_style_border_width(mg.jewel, 4, 0);
  lv_obj_set_style_border_color(mg.jewel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_opa(mg.jewel, LV_OPA_60, 0);
  lv_obj_add_flag(mg.jewel, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(mg.jewel, mgJewelTap, LV_EVENT_PRESSED, nullptr);
}
static void mgTick(lv_timer_t *) {
  if (!mg.ov || mg.over) return;
  uint32_t now = millis(), el = now - mg.startMs;
  if (el >= 12000) {
    mgEnd();
    return;
  }
  lv_label_set_text_fmt(mg.lblTime, "%d", (int)((12000 - el + 999) / 1000));
  if (mg.jewel && now > mg.jewelDie) {
    lv_obj_del(mg.jewel);
    mg.jewel = nullptr;
  }
  if (!mg.jewel && now >= mg.nextSpawn) {
    mgSpawn();
    mg.jewelDie = now + 1100;
    mg.nextSpawn = now + 650 + random(0, 350);
  }
}
static void mgClose(lv_event_t *) {
  if (mg.t) lv_timer_del(mg.t);
  mg.t = nullptr;
  if (mg.ov) lv_obj_del(mg.ov);
  mg = MiniGame();
}
static void mgEnd() {
  mg.over = true;
  if (mg.jewel) lv_obj_del(mg.jewel);
  mg.jewel = nullptr;
  if (mg.t) lv_timer_del(mg.t);
  mg.t = nullptr;
  int score = mg.score;
  dgFinishPlay(score);
  lv_obj_t *card = makeCard(mg.ov, 300, 170);
  lv_obj_center(card);
  lv_obj_t *t = makeLabel(card, DL("Hết giờ!", "Time's up!"), F_HEADING, C_ACCENT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 14);
  char buf[80];
  snprintf(buf, sizeof(buf), isVi() ? "Bắt được %d ngọc  (+%d xu)" : "Caught %d gems  (+%d coins)", score, score);
  lv_obj_t *r = makeLabel(card, buf, F_BODY, C_TEXT);
  lv_obj_align(r, LV_ALIGN_TOP_MID, 0, 62);
  snprintf(buf, sizeof(buf), isVi() ? "Kỷ lục: %d" : "Best: %d", (int)dg.best);
  lv_obj_t *b2 = makeLabel(card, buf, F_CAPTION, C_MUTED);
  lv_obj_align(b2, LV_ALIGN_TOP_MID, 0, 90);
  mg.btn = smallBtn(card, DL("Xong", "Done"), C_ACCENT, mgClose, nullptr, 120);
  lv_obj_align(mg.btn, LV_ALIGN_BOTTOM_MID, 0, -12);
}
static void dgStartMini() {
  if (dgBlockedAsleep()) return;
  if (dg.energy < 15) {
    dgSay(DL("Rồng mệt quá, cho nghỉ đã.", "Too tired to play."));
    return;
  }
  if (dg.sick) {
    dgSay(DL("Rồng đang ốm, chưa chơi được.", "Too sick to play."));
    return;
  }
  mg = MiniGame();
  mg.ov = dgOpenOverlay();
  dOv = nullptr;  // the mini game owns its overlay
  lv_obj_t *hint = makeLabel(mg.ov, DL("Chạm vào ngọc thật nhanh!", "Tap the gems fast!"), F_LABEL, C_TEXT);
  lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 14);
  mg.lblScore = makeLabel(mg.ov, "0", F_HEADING, C_OK);
  lv_obj_align(mg.lblScore, LV_ALIGN_TOP_LEFT, 20, 10);
  mg.lblTime = makeLabel(mg.ov, "12", F_HEADING, C_WARN);
  lv_obj_align(mg.lblTime, LV_ALIGN_TOP_RIGHT, -20, 10);
  mg.startMs = millis();
  mg.nextSpawn = mg.startMs + 400;
  mg.t = lv_timer_create(mgTick, 60, nullptr);
}

// ---- feeding menu ----
static void feedPick(lv_event_t *e) {
  int kind = (int)(intptr_t)lv_event_get_user_data(e);
  dgCloseOverlay();
  dgFeed(kind);
}
static void dgOpenFeed() {
  if (dgBlockedAsleep()) return;
  lv_obj_t *ov = dgOpenOverlay();
  lv_obj_t *card = makeCard(ov, 360, 230);
  lv_obj_center(card);
  lv_obj_t *t = makeLabel(card, DL("Cho rồng ăn gì?", "What to feed?"), F_TITLE, C_TEXT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 12);
  const char *names[3] = {DL("Trái cây  (miễn phí)", "Fruit  (free)"), DL("Thịt nướng  (5 xu)", "Grilled meat  (5 coins)"),
                          DL("Bánh ngọc  (15 xu)", "Jewel cake  (15 coins)")};
  for (int i = 0; i < 3; i++) {
    lv_obj_t *b = smallBtn(card, names[i], C_ACCENT, feedPick, (void *)(intptr_t)i, 320);
    lv_obj_set_size(b, 320, 38);
    lv_obj_align(b, LV_ALIGN_TOP_MID, 0, 48 + i * 44);
  }
  lv_obj_t *c = smallBtn(card, DL("Đóng", "Close"), C_MUTED, dgOverlayDone, nullptr, 120);
  lv_obj_align(c, LV_ALIGN_BOTTOM_MID, 0, -8);
}

// ---- reincarnation (divine dragon only) ----
static void doReincarnate() {
  dgNewEgg(true);
  dSig = -1;
  dgSay(DL("Một quả trứng mới xuất hiện!", "A new egg appears!"));
}
static void askReincarnate(lv_event_t *) {
  confirmDialog("Reincarnate?", "Your divine dragon leaves a new egg. You keep your coins and the next dragon grows faster.", doReincarnate);
}

static void dgBtnCb(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (dOv || mg.ov) return;
  if (dg.stage == ST_EGG) {
    if (i == 0) dgWarm();
    return;
  }
  switch (i) {
    case 0: dgOpenFeed(); break;
    case 1: dgStartMini(); break;
    case 2: dgBath(); break;
    case 3: dgToggleSleep(); break;
    case 4: dgMedicine(); break;
  }
}

static lv_color_t dgBarColor(int i, float v) {
  static const uint32_t cols[4] = {0xFFB84A, 0xFF6B9A, 0x38E1FF, 0x3DEBA0};
  return v < 25 ? C_DANGER : lv_color_hex(cols[i]);
}

static void dgRefreshUi() {
  if (!dScr || !dgOk) return;
  bool egg = dg.stage == ST_EGG;
  float vals[4] = {egg ? dg.energy : dg.satiety, dg.mood, dg.clean, egg ? 0 : dg.energy};
  if (!egg) {
    vals[0] = dg.satiety;
    vals[3] = dg.energy;
  }
  const char *names[4] = {egg ? DL("Độ ấm", "Warmth") : DL("No bụng", "Fed"), DL("Vui vẻ", "Mood"), DL("Sạch sẽ", "Clean"), DL("Năng lượng", "Energy")};
  for (int i = 0; i < 4; i++) {
    bool show = !egg || i == 0;
    if (show) {
      lv_obj_clear_flag(dBarLbl[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(dBars[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(dBarLbl[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(dBars[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text_fmt(dBarLbl[i], "%s  %d", names[i], (int)vals[i]);
    lv_bar_set_value(dBars[i], (int)vals[i], LV_ANIM_OFF);
    lv_obj_set_style_bg_color(dBars[i], dgBarColor(i, vals[i]), LV_PART_INDICATOR);
  }
  lv_label_set_text_fmt(dCoins, "%u %s", (unsigned)dg.coins, DL("xu", "coins"));
  lv_label_set_text_fmt(dStageLbl, "%s  -  %s%s", dgName().c_str(), dgStageName(dg.stage), dg.gen > 1 ? (String("  (G") + dg.gen + ")").c_str() : "");
  if (dg.stage == ST_DIVINE) {
    lv_bar_set_value(dXpBar, 100, LV_ANIM_OFF);
    lv_label_set_text(dXpTxt, DL("Đã đạt cấp cao nhất", "Maximum level reached"));
    lv_obj_clear_flag(dReinc, LV_OBJ_FLAG_HIDDEN);
  } else {
    int pct = (int)(dg.growth * 100 / dgNeed());
    lv_bar_set_value(dXpBar, pct > 100 ? 100 : pct, LV_ANIM_OFF);
    lv_label_set_text_fmt(dXpTxt, "%s: %d%%", egg ? DL("Ấp trứng", "Hatching") : DL("Tiến hóa", "Evolution"), pct);
    lv_obj_add_flag(dReinc, LV_OBJ_FLAG_HIDDEN);
  }

  // message line
  String msg;
  uint32_t ms = millis();
  if (ms < dgToastUntil) msg = dgToast;
  else if (egg) msg = dg.energy < 30 ? DL("Trứng lạnh quá, hãy ấp ấm!", "The egg is cold - warm it up!") : DL("Trứng đang ấm, sắp nở rồi...", "The egg is warm, hatching soon...");
  else if (dg.sleeping) msg = DL("Zzz... Rồng đang ngủ", "Zzz... sleeping");
  else if (dg.sick) msg = DL("Rồng bị ốm, cần uống thuốc!", "Sick - give it medicine!");
  else if (dg.satiety < 25) msg = DL("Rồng đói bụng quá!", "Your dragon is hungry!");
  else if (dg.clean < 25) msg = DL("Rồng bẩn quá, cho đi tắm đi!", "Time for a bath!");
  else if (dg.energy < 25) msg = DL("Rồng buồn ngủ rồi.", "Your dragon is sleepy.");
  else if (dg.mood < 25) msg = DL("Rồng buồn, chơi với rồng đi!", "Feeling sad - play with it!");
  else if (dgNight() && dg.energy < 60) msg = DL("Khuya rồi, cho rồng đi ngủ nhé.", "It's late, time for bed.");
  else msg = DL("Rồng đang rất vui!", "Your dragon is happy!");
  lv_label_set_text(dMsg, msg.c_str());

  // buttons
  const char *bl[5] = {egg ? DL("Ấp", "Warm") : DL("Ăn", "Feed"), DL("Chơi", "Play"), DL("Tắm", "Bath"), dg.sleeping ? DL("Dậy", "Wake") : DL("Ngủ", "Sleep"), DL("Thuốc", "Medicine")};
  for (int i = 0; i < 5; i++) {
    lv_label_set_text(lv_obj_get_child(dBtns[i], 0), bl[i]);
    if (egg && i > 0) lv_obj_add_flag(dBtns[i], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(dBtns[i], LV_OBJ_FLAG_HIDDEN);
  }
  if (egg) {
    lv_obj_set_width(dBtns[0], 464);
    lv_obj_set_x(dBtns[0], 8);
  } else {
    lv_obj_set_width(dBtns[0], 88);
  }

  // dragon picture, redrawn only when something visible changed
  int moodLv = dg.mood < 30 ? 0 : dg.mood < 65 ? 1 : 2;
  bool night = dg.sleeping || dgNight();
  int eggStep = egg ? (int)(dg.growth * 100 / dgNeed()) / 20 : 0;
  int sig = dg.stage | (dg.sleeping << 4) | (dg.sick << 5) | (moodLv << 6) | (night << 8) | (eggStep << 9) | ((egg && dg.energy < 30) << 12);
  if (sig != dSig) {
    dSig = sig;
    lv_obj_set_style_bg_color(dBox, lv_color_hex(night ? 0x070B1A : 0x2B4C99), 0);
    lv_obj_set_style_bg_grad_color(dBox, lv_color_hex(night ? 0x1D1146 : 0x6FA0E8), 0);
    lv_obj_clean(dBob);
    if (egg) dgDrawEgg(dBob);
    else dgDrawDragon(dBob, dg.stage, dg.sleeping, dg.sick, moodLv);
  }
  if (dgPendingEvolve > 0 && !dOv && !mg.ov) {
    int st = dgPendingEvolve;
    dgPendingEvolve = -1;
    dgShowStory(st, true);
  }
}

static void dgUiTick(lv_timer_t *) { dgRefreshUi(); }
static void animBob(void *o, int32_t v) { lv_obj_set_y((lv_obj_t *)o, v); }

static void pageDragon() {
  lv_obj_t *c;
  lv_obj_t *s = pageBegin("Dragon", &c);
  lv_obj_del(c);
  if (!dgOk) {
    lv_obj_t *card = makeCard(s, 456, 120);
    lv_obj_set_pos(card, 12, 80);
    lv_obj_t *t = makeLabel(card, DL("Cần thẻ nhớ microSD", "A microSD card is required"), F_TITLE, C_WARN);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 18);
    lv_obj_t *h = makeLabel(card, DL("Dữ liệu rồng được lưu trên thẻ nhớ. Hãy cắm thẻ rồi khởi động lại thiết bị.", "The dragon is saved on the card. Insert one and restart the device."), F_BODY, C_MUTED);
    lv_obj_set_width(h, 410);
    lv_label_set_long_mode(h, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(h, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(h, LV_ALIGN_TOP_MID, 0, 58);
    pageShow(s);
    return;
  }
  dScr = s;
  dSig = -1;
  dOv = nullptr;
  mg = MiniGame();

  dCoins = makeLabel(s, "", F_LABEL, C_WARN);
  lv_obj_align(dCoins, LV_ALIGN_TOP_RIGHT, -14, 18);
  dReinc = smallBtn(s, DL("Luân hồi", "Rebirth"), C_ACCENT2, askReincarnate, nullptr, 96);
  lv_obj_align(dReinc, LV_ALIGN_TOP_RIGHT, -120, 10);
  lv_obj_add_flag(dReinc, LV_OBJ_FLAG_HIDDEN);

  dStageLbl = makeLabel(s, "", F_LABEL, C_ACCENT);
  lv_obj_align(dStageLbl, LV_ALIGN_TOP_LEFT, 14, 54);

  dBox = makeCard(s, 292, 148);
  lv_obj_set_pos(dBox, 10, 78);
  lv_obj_set_style_bg_grad_dir(dBox, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_radius(dBox, 18, 0);
  lv_obj_set_style_clip_corner(dBox, true, 0);
  dBob = lv_obj_create(dBox);
  lv_obj_remove_style_all(dBob);
  lv_obj_set_size(dBob, 292, 148);
  lv_obj_set_pos(dBob, 0, 0);
  lv_obj_clear_flag(dBob, LV_OBJ_FLAG_CLICKABLE);
  lv_anim_t a;  // gentle breathing
  lv_anim_init(&a);
  lv_anim_set_var(&a, dBob);
  lv_anim_set_exec_cb(&a, animBob);
  lv_anim_set_values(&a, 0, -5);
  lv_anim_set_time(&a, 1400);
  lv_anim_set_playback_time(&a, 1400);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_start(&a);

  for (int i = 0; i < 4; i++) {
    int y = 80 + i * 36;
    dBarLbl[i] = makeLabel(s, "", F_CAPTION, C_MUTED);
    lv_obj_set_pos(dBarLbl[i], 314, y);
    dBars[i] = lv_bar_create(s);
    lv_obj_set_size(dBars[i], 154, 9);
    lv_obj_set_pos(dBars[i], 314, y + 18);
    lv_bar_set_range(dBars[i], 0, 100);
    lv_obj_set_style_bg_color(dBars[i], C_BORDER, LV_PART_MAIN);
  }

  dXpBar = lv_bar_create(s);
  lv_obj_set_size(dXpBar, 292, 7);
  lv_obj_set_pos(dXpBar, 10, 232);
  lv_bar_set_range(dXpBar, 0, 100);
  lv_obj_set_style_bg_color(dXpBar, C_BORDER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(dXpBar, C_ACCENT2, LV_PART_INDICATOR);
  dXpTxt = makeLabel(s, "", F_CAPTION, C_MUTED);
  lv_obj_set_pos(dXpTxt, 314, 226);

  dMsg = makeLabel(s, "", F_BODY, C_TEXT);
  lv_obj_set_width(dMsg, 456);
  lv_label_set_long_mode(dMsg, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(dMsg, 14, 245);

  for (int i = 0; i < 5; i++) {
    lv_obj_t *b = lv_btn_create(s);
    lv_obj_set_size(b, 88, 40);
    lv_obj_set_pos(b, 8 + i * 94, 272);
    lv_obj_set_style_radius(b, 14, 0);
    lv_obj_set_style_bg_color(b, C_CARD, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_border_color(b, i == 4 ? C_DANGER : C_ACCENT, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_bg_color(b, C_BORDER, LV_STATE_PRESSED);
    lv_obj_t *l = makeLabel(b, "", F_BODY, i == 4 ? C_DANGER : C_TEXT);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, dgBtnCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    dBtns[i] = b;
  }

  pageTimer(s, dgUiTick, 500);
  lv_obj_add_event_cb(
      s,
      [](lv_event_t *) {
        if (mg.t) lv_timer_del(mg.t);
        mg = MiniGame();
        dScr = nullptr;
        dOv = nullptr;
        dgSave();
      },
      LV_EVENT_DELETE, nullptr);

  dgDailyCheck();
  dgRefreshUi();
  if (!dg.intro) {
    dg.intro = true;
    dgShowStory(0, false);
    dgSave();
  }
  pageShow(s);
}
