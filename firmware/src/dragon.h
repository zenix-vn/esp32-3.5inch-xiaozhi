#pragma once
// "Luyện rồng" (dragon training) virtual pet: a real-time game. Needs keep draining while the device runs and the
// time the device was off is caught up from the wall clock. Progress lives on the microSD card
// (/zin100/dragon.json). Design and story: see docs.md.
// Included by main.cpp after the weather/shape helpers (uses shape(), makeLabel(), pageBegin(), sdOk, ...).
#include <SD.h>
#include <ArduinoJson.h>
#include "pixel_img.h"
#include "decor_img.h"
#include "scene_img.h"


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
  // V2
  uint16_t best2 = 0;   // "Dragon flight" best score
  uint8_t decor = 0;    // owned room decorations (bitmask, see DECOR)
  uint16_t ach = 0;     // unlocked achievements (bitmask, see ACH)
  uint16_t streak = 0;  // consecutive days visited
  uint32_t pets = 0;
  struct Entry {
    uint32_t ts;
    uint8_t type, arg;  // type: 1 evolved (stage), 2 achievement (id), 3 reborn (generation), 4 bought (decor)
  } jn[20];             // journal, oldest first
  uint8_t jnCount = 0;
};
static DragonSave dg;
static bool dgOk = false;           // microSD present and the save is loaded
static bool dgNeedCatchup = false;  // apply the time the device was off once the clock is known
static int dgPendingEvolve = -1;    // stage reached and not yet shown to the player
static float dgSpeed = 1;           // growth multiplier (testing only, resets on reboot)
static uint32_t dgLastMs = 0, dgLastSave = 0;
static String dgToast;
static bool dgCoinFound = false;  // the room shows a coin popping up
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

// ---------- room decorations and achievements ----------
struct DecorInfo {
  const char *vi, *en;
  uint16_t price;
  const lv_img_dsc_t *img;
  int16_t x, y;  // top-left inside the room box
};
static const DecorInfo DECOR[6] = {
    {"Chậu cây", "Potted plant", 30, &dc_plant, 14, 112},    {"Đèn vườn", "Garden lamp", 40, &dc_lamp, 418, 104},
    {"Thảm picnic", "Picnic mat", 50, &dc_rug, 160, 152},    {"Biển gỗ", "Wooden sign", 60, &dc_painting, 96, 70},
    {"Đèn lồng", "Lantern", 80, &dc_lantern, 40, 34},        {"Rương báu", "Treasure chest", 150, &dc_chest, 340, 128},
};
struct AchInfo {
  const char *vi, *en, *dvi, *den;
};
static const AchInfo ACH[12] = {
    {"Phá vỏ", "Hatched", "Ấp nở quả trứng", "Hatch the egg"},
    {"Lớn khôn", "Growing up", "Đạt Rồng cấp 3", "Reach Dragon Lv.3"},
    {"Trưởng thành", "All grown up", "Đạt Rồng trưởng thành", "Reach Adult Dragon"},
    {"Thần long", "Divine", "Đạt Rồng thần", "Reach Divine Dragon"},
    {"Sành ăn", "Gourmet", "Cho ăn 100 lần", "Feed 100 times"},
    {"Sạch bong", "Squeaky clean", "Tắm 50 lần", "Bath 50 times"},
    {"Thợ săn ngọc", "Gem hunter", "Bắt ngọc 20 điểm", "Score 20 in Gem catch"},
    {"Phi công", "Sky pilot", "Rồng bay 10 điểm", "Score 10 in Dragon flight"},
    {"Đại gia", "Tycoon", "Có 1000 xu", "Hold 1000 coins"},
    {"Bạn thân", "Best friends", "Ghé thăm 7 ngày liền", "Visit 7 days in a row"},
    {"Nhà trang trí", "Decorator", "Mua đủ 6 món đồ", "Own all 6 decorations"},
    {"Luân hồi", "Reborn", "Rồng thế hệ thứ 2", "Raise a second generation"},
};

static void dgJournal(uint8_t type, uint8_t arg) {
  if (dg.jnCount == 20) {
    memmove(&dg.jn[0], &dg.jn[1], sizeof(dg.jn[0]) * 19);
    dg.jnCount = 19;
  }
  dg.jn[dg.jnCount++] = {dgTimeValid() ? (uint32_t)time(nullptr) : 0, type, arg};
}

static void soundPlay(const Note *seq, int len, int repeat, bool evenInDnd);
static void dgCheckAch() {
  bool cond[12] = {dg.stage >= ST_BABY, dg.stage >= ST_L3,  dg.stage >= ST_ADULT, dg.stage >= ST_DIVINE,
                   dg.fed >= 100,       dg.bathed >= 50,    dg.best >= 20,        dg.best2 >= 10,
                   dg.coins >= 1000,    dg.streak >= 7,     dg.decor == 0x3F,     dg.gen >= 2};
  for (int i = 0; i < 12; i++) {
    if (!cond[i] || (dg.ach & (1 << i))) continue;
    dg.ach |= 1 << i;
    dg.coins += 20;
    dgJournal(2, i);
    dgSay(String(DL("Huy hiệu mới: ", "New badge: ")) + (isVi() ? ACH[i].vi : ACH[i].en) + DL(" (+20 xu)", " (+20 coins)"));
    SFX("achievement", SND_COIN);
  }
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
  d["best2"] = dg.best2;
  d["decor"] = dg.decor;
  d["ach"] = dg.ach;
  d["streak"] = dg.streak;
  d["pets"] = dg.pets;
  JsonArray jn = d["journal"].to<JsonArray>();
  for (int i = 0; i < dg.jnCount; i++) {
    JsonArray e = jn.add<JsonArray>();
    e.add(dg.jn[i].ts);
    e.add(dg.jn[i].type);
    e.add(dg.jn[i].arg);
  }
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
  dg.best2 = d["best2"] | 0;
  dg.decor = (d["decor"] | 0) & 0x3F;
  dg.ach = d["ach"] | 0;
  dg.streak = d["streak"] | 0;
  dg.pets = d["pets"] | 0;
  dg.jnCount = 0;
  for (JsonArray e : d["journal"].as<JsonArray>()) {
    if (dg.jnCount >= 20) break;
    dg.jn[dg.jnCount++] = {e[0] | 0u, (uint8_t)(e[1] | 0), (uint8_t)(e[2] | 0)};
  }
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
    dgJournal(1, dg.stage);
  }
  if (dg.stage == ST_DIVINE) dg.growth = 0;
  dgCheckAch();
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
  time_t y = time(nullptr) - 86400;  // was the last visit yesterday?
  struct tm yt;
  localtime_r(&y, &yt);
  uint32_t yesterday = (yt.tm_year + 1900) * 10000 + (yt.tm_mon + 1) * 100 + yt.tm_mday;
  dg.streak = dg.dailyDay == yesterday ? dg.streak + 1 : 1;
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
    dgCoinFound = true;
  }
  if (ms - dgLastSave > 60000) dgSave();
}

static void dgNewEgg(bool keepLegacy) {
  uint16_t gen = dg.gen, best = dg.best;
  uint32_t coins = dg.coins, fed = dg.fed, played = dg.played, bathed = dg.bathed, meds = dg.meds;
  String name = dg.name;
  DragonSave old = dg;
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
    dg.best2 = old.best2;
    dg.decor = old.decor;
    dg.ach = old.ach;
    dg.streak = old.streak;
    dg.dailyDay = old.dailyDay;
    dg.pets = old.pets;
    dg.jnCount = old.jnCount;
    memcpy(dg.jn, old.jn, sizeof(dg.jn));
    dgJournal(3, dg.gen);
    dgCheckAch();
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
  dgCheckAch();
  dgSay(DL("Ngon quá!", "Yummy!"));
  dgSave();
}
static void dgBath() {
  if (dgBlockedAsleep()) return;
  dg.clean = dgClamp(dg.clean + 45);
  dg.mood = dgClamp(dg.mood + 3);
  dg.bathed++;
  dgCheckAch();
  dgSay(DL("Sạch bong, thơm phức!", "Squeaky clean!"));
  dgSave();
}
static void dgToggleSleep() {
  if (dg.sleeping) {
    dg.sleeping = false;
    SFX("chirp", SND_CHIRP);
    dgSay(DL("Rồng thức dậy.", "Your dragon woke up."));
  } else if (dg.energy > 90) {
    dgSay(DL("Rồng chưa buồn ngủ.", "Not sleepy yet."));
    return;
  } else {
    dg.sleeping = true;
    SFX("sleep", SND_TAP);
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
  SFX("heal", SND_OK);
  dgSave();
}
// game 0 = gem catch, 1 = dragon flight (worth double coins)
static void dgFinishPlay(int score, int game = 0) {
  dg.coins += game ? score * 2 : score;
  dg.mood = dgClamp(dg.mood + 8 + score * 3);
  dg.energy = dgClamp(dg.energy - (game ? 10 : 8));
  dg.growth += score / 2.0f > 5 ? 5 : score / 2.0f;
  if (game == 0 && score > dg.best) dg.best = score;
  if (game == 1 && score > dg.best2) dg.best2 = score;
  dg.played++;
  dgEvolveCheck();
  dgSave();
}
static bool dgBuyDecor(int i) {
  if (dg.decor & (1 << i)) return false;
  if (dg.coins < DECOR[i].price) {
    dgSay(DL("Không đủ xu.", "Not enough coins."));
    return false;
  }
  dg.coins -= DECOR[i].price;
  dg.decor |= 1 << i;
  dgJournal(4, i);
  dgCheckAch();
  dgSave();
  return true;
}

// =====================================================================================
// Drawing: hand-made pixel art (tools/gen_pixelart.py) - 40x40 dragons with four expressions each, eggs
// 24x30 - always shown with a whole-number zoom so every pixel stays a crisp square.
// =====================================================================================
enum { EX_NORMAL, EX_HAPPY, EX_SAD, EX_SLEEP };
static const lv_img_dsc_t *const DRAGON_SPRITES[8][4] = {
    {&dg_baby_normal, &dg_baby_happy, &dg_baby_sad, &dg_baby_sleep},
    {&dg_lv1_normal, &dg_lv1_happy, &dg_lv1_sad, &dg_lv1_sleep},
    {&dg_lv2_normal, &dg_lv2_happy, &dg_lv2_sad, &dg_lv2_sleep},
    {&dg_lv3_normal, &dg_lv3_happy, &dg_lv3_sad, &dg_lv3_sleep},
    {&dg_adult_normal, &dg_adult_happy, &dg_adult_sad, &dg_adult_sleep},
    {&dg_king_normal, &dg_king_happy, &dg_king_sad, &dg_king_sleep},
    {&dg_emperor_normal, &dg_emperor_happy, &dg_emperor_sad, &dg_emperor_sleep},
    {&dg_divine_normal, &dg_divine_happy, &dg_divine_sad, &dg_divine_sleep},
};
static const int DG_BOX_W = 472, DG_BOX_H = 188, DG_GROUND = 8;  // the game scene and the feet margin
static const uint8_t DG_STAGE_ZOOM[9] = {4, 3, 3, 3, 4, 4, 4, 4, 4};  // in the game: egg, then stages 1..8

static lv_obj_t *dgSprite(lv_obj_t *p, const lv_img_dsc_t *src, int x, int y, int zoom) {
  lv_obj_t *img = lv_img_create(p);
  lv_img_set_src(img, src);
  lv_obj_set_pos(img, x, y);
  lv_img_set_antialias(img, false);  // nearest-neighbour: whole pixels
  lv_img_set_pivot(img, 0, 0);
  lv_img_set_zoom(img, 256 * zoom);
  lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
  return img;
}
static void dgShadow(lv_obj_t *p, int cx, int footY, int w) { shape(p, cx - w / 2, footY - 5, w, 10, 0x000000, LV_RADIUS_CIRCLE, LV_OPA_20); }
static int dgExpr(bool sleeping, bool sick, int moodLv) {
  return sleeping ? EX_SLEEP : (sick || moodLv == 0) ? EX_SAD : moodLv == 2 ? EX_HAPPY : EX_NORMAL;
}

static lv_obj_t *dgDrawEgg(lv_obj_t *p, int boxW = DG_BOX_W, int boxH = DG_BOX_H, int zoom = 0, bool shadow = true) {
  if (!zoom) zoom = DG_STAGE_ZOOM[0];
  int prog = dgNeed() > 0 ? (int)(dg.growth * 100 / dgNeed()) : 0;
  const lv_img_dsc_t *src = dg.energy < 30 ? &egg_cold : prog > 80 ? &egg_crack2 : prog > 40 ? &egg_crack1 : &egg_warm;
  int w = src->header.w * zoom, h = src->header.h * zoom;
  if (shadow) dgShadow(p, boxW / 2, boxH - DG_GROUND, w * 3 / 4);
  return dgSprite(p, src, (boxW - w) / 2, boxH - DG_GROUND - h, zoom);
}

// Returns the sprite object (callers keep it to make the dragon blink).
static lv_obj_t *dgDrawDragon(lv_obj_t *p, int st, bool sleeping, bool sick, int moodLv, int boxW = DG_BOX_W, int boxH = DG_BOX_H,
                              int zoom = 0, bool shadow = true) {
  if (!zoom) zoom = DG_STAGE_ZOOM[st];
  const lv_img_dsc_t *src = DRAGON_SPRITES[st - 1][dgExpr(sleeping, sick, moodLv)];
  int w = src->header.w * zoom, h = src->header.h * zoom;
  int x = (boxW - w) / 2, y = boxH - DG_GROUND - h + zoom * 2;  // the art has a little empty space under the feet
  if (shadow) dgShadow(p, x + w / 2, boxH - DG_GROUND, w / 2);
  lv_obj_t *img = dgSprite(p, src, x, y, zoom);
  if (sick) {
    lv_obj_set_style_img_recolor(img, lv_color_hex(0x7FA060), 0);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_30, 0);
  }
  if (sleeping) {
    lv_obj_t *z = makeLabel(p, "Z z", F_TITLE, lv_color_hex(0x5A6FD8));
    lv_obj_set_pos(z, x + w * 3 / 4, y + zoom * 2);
  }
  return img;
}

// Blinking: every few seconds the open-eyed frame is swapped for the closed one for a moment.
struct Blinker {
  lv_obj_t *img = nullptr;
  const lv_img_dsc_t *open = nullptr, *closed = nullptr;
  uint32_t next = 0, until = 0;
};
static void blinkSet(Blinker &b, lv_obj_t *img, int st, int expr) {
  b.img = img;
  b.until = 0;
  bool canBlink = img && st >= 1 && (expr == EX_NORMAL || expr == EX_SAD);
  b.open = canBlink ? DRAGON_SPRITES[st - 1][expr] : nullptr;
  b.closed = canBlink ? DRAGON_SPRITES[st - 1][EX_SLEEP] : nullptr;
  b.next = millis() + 1500 + random(0, 2500);
}
static void blinkTick(Blinker &b) {
  if (!b.img || !b.open) return;
  uint32_t now = millis();
  if (b.until && now >= b.until) {
    lv_img_set_src(b.img, b.open);
    b.until = 0;
    b.next = now + 2200 + random(0, 3000);
  } else if (!b.until && now >= b.next) {
    lv_img_set_src(b.img, b.closed);
    b.until = now + 150;
  }
}

// =====================================================================================
// Game page "Luyện rồng": pixel-art garden (tools/gen_scene.py) with the dragon walking around, pastel
// striped panels with brown text (header: name / evolution + mood / coins, shop, badges; footer: actions).
// =====================================================================================
#define PX_INK 0x5A3A22     // brown text
#define PX_EDGE 0xE29B4B    // orange-brown panel border
#define PX_PANEL 0xD2F3F8   // pastel panel (under the stripes)

static lv_obj_t *dScr = nullptr, *dBox, *dRoom, *dBob, *dFx, *dCoins, *dName, *dStageLbl, *dXpBar, *dXpTxt, *dFace, *dReinc;
static lv_obj_t *dBubble, *dBubbleLbl, *dBtns[5], *dBars[4], *dOv = nullptr;
static Blinker dBlink;
static int dSig = -1, dRoomSig = -1, dFaceLv = -1;
static uint32_t dNextWalk = 0, dPetAt = 0, dPetHourStart = 0;
static int dPetDist = 0, dPetCount = 0;

struct MiniGame {
  lv_obj_t *ov = nullptr, *jewel = nullptr, *lblScore = nullptr, *lblTime = nullptr, *btn = nullptr;
  lv_timer_t *t = nullptr;
  int score = 0;
  uint32_t startMs = 0, nextSpawn = 0, jewelDie = 0;
  bool over = false;
};
static MiniGame mg;

struct Flight {  // "Rồng bay": tap to flap between cloud pillars
  lv_obj_t *ov = nullptr, *pl = nullptr, *lbl = nullptr, *hint = nullptr, *col[3][2] = {};
  lv_timer_t *t = nullptr;
  float y = 140, vy = 0;
  int cx[3] = {}, gapY[3] = {}, score = 0;
  bool passed[3] = {}, started = false, over = false;
};
static Flight fb;
static bool dgBusy() { return dOv || mg.ov || fb.ov; }

// ---------- pixel UI kit ----------
static lv_obj_t *pxPanel(lv_obj_t *parent, int w, int h) {
  lv_obj_t *p = lv_obj_create(parent);
  lv_obj_remove_style_all(p);
  lv_obj_set_size(p, w, h);
  lv_obj_set_style_radius(p, 6, 0);
  lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(p, lv_color_hex(PX_PANEL), 0);
  lv_obj_set_style_bg_img_src(p, &ui_stripe, 0);
  lv_obj_set_style_bg_img_tiled(p, true, 0);
  lv_obj_set_style_border_width(p, 3, 0);
  lv_obj_set_style_border_color(p, lv_color_hex(PX_EDGE), 0);
  lv_obj_set_style_clip_corner(p, false, 0);
  lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
  return p;
}
static lv_obj_t *pxText(lv_obj_t *parent, const char *t, const lv_font_t *font) { return makeLabel(parent, t, font, lv_color_hex(PX_INK)); }

// Pastel button: icon above a label (or a label alone).
static lv_obj_t *pxButton(lv_obj_t *parent, int w, int h, const lv_img_dsc_t *icon, const char *text, lv_event_cb_t cb, void *ud) {
  lv_obj_t *b = pxPanel(parent, w, h);
  lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(b, lv_color_hex(0xFFE9C4), LV_STATE_PRESSED);
  lv_obj_set_style_bg_img_opa(b, LV_OPA_TRANSP, LV_STATE_PRESSED);
  lv_obj_set_style_border_color(b, lv_color_hex(0xC9772E), LV_STATE_PRESSED);
  lv_obj_set_flex_flow(b, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(b, 0, 0);
  if (icon) {  // 16x16 pixel icons are shown doubled
    lv_obj_t *i = lv_img_create(b);
    lv_img_set_src(i, icon);
    lv_img_set_antialias(i, false);
    if (icon->header.w <= 20) {
      lv_img_set_zoom(i, 512);
      lv_img_set_size_mode(i, LV_IMG_SIZE_MODE_REAL);
    }
    lv_obj_clear_flag(i, LV_OBJ_FLAG_CLICKABLE);
  }
  if (text) pxText(b, text, F_BODY);
  if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
  return b;
}

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
  lv_obj_set_style_bg_color(dOv, lv_color_hex(0x1A1020), 0);
  lv_obj_set_style_bg_opa(dOv, LV_OPA_60, 0);
  lv_obj_add_flag(dOv, LV_OBJ_FLAG_CLICKABLE);  // swallow touches
  return dOv;
}

// ---------- effects ----------
static void fxDel(lv_anim_t *a) { lv_obj_del((lv_obj_t *)a->var); }
// img_opa / bg_opa, not style opa: that would composite each fading sprite through a layer buffer every frame
static void fxImgOpa(void *o, int32_t v) { lv_obj_set_style_img_opa((lv_obj_t *)o, v, 0); }
static void fxBgOpa(void *o, int32_t v) { lv_obj_set_style_bg_opa((lv_obj_t *)o, v, 0); }
static void fxY(void *o, int32_t v) { lv_obj_set_y((lv_obj_t *)o, v); }
static void fxX(void *o, int32_t v) { lv_obj_set_x((lv_obj_t *)o, v); }

// A sprite that drifts up by `rise` px and fades out, then deletes itself.
static lv_obj_t *dgFloat(const lv_img_dsc_t *src, int x, int y, int rise, uint32_t ms, uint32_t delay = 0) {
  if (!dScr) return nullptr;
  lv_obj_t *o = lv_img_create(dFx);
  lv_img_set_src(o, src);
  lv_obj_set_pos(o, x - src->header.w / 2, y - src->header.h / 2);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_img_opa(o, LV_OPA_TRANSP, 0);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, o);
  lv_anim_set_exec_cb(&a, fxY);
  lv_anim_set_values(&a, y - src->header.h / 2, y - src->header.h / 2 - rise);
  lv_anim_set_time(&a, ms);
  lv_anim_set_delay(&a, delay);
  lv_anim_set_early_apply(&a, false);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_start(&a);
  lv_anim_set_exec_cb(&a, fxImgOpa);
  lv_anim_set_values(&a, 255, 0);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
  lv_anim_set_ready_cb(&a, fxDel);
  lv_anim_start(&a);
  return o;
}

// Where the dragon's mouth is inside the scene (follows the walking offset).
static lv_point_t dgMouth() {
  if (dg.stage == ST_EGG) return {DG_BOX_W / 2, DG_BOX_H / 2};
  int z = DG_STAGE_ZOOM[dg.stage];
  int x = (DG_BOX_W - 40 * z) / 2 + lv_obj_get_x(dBob), y = DG_BOX_H - DG_GROUND - 40 * z + z * 2;
  return {(lv_coord_t)(x + 20 * z), (lv_coord_t)(y + 19 * z)};  // the mouth sits at (20, 19) in the 40x40 art
}

static void dgHearts(int n, int x, int y) {
  for (int i = 0; i < n; i++) dgFloat(&fx_heart, x + random(-18, 18), y - random(0, 10), 40 + random(0, 20), 900, i * 160);
}

static void dgFxFeed(int kind) {
  static const lv_img_dsc_t *foods[3] = {&fx_food0, &fx_food1, &fx_food2};
  lv_point_t m = dgMouth();
  lv_obj_t *o = lv_img_create(dFx);
  lv_img_set_src(o, foods[kind]);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, o);
  lv_anim_set_time(&a, 650);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_set_exec_cb(&a, fxX);
  lv_anim_set_values(&a, DG_BOX_W - 30, m.x - 13);
  lv_anim_start(&a);
  lv_anim_set_exec_cb(&a, fxY);
  lv_anim_set_values(&a, 10, m.y - 13);
  lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
  lv_anim_set_ready_cb(&a, fxDel);
  lv_anim_start(&a);
  dgHearts(3, m.x, m.y - 20);
}
static void dgFxBath() {
  lv_point_t m = dgMouth();
  for (int i = 0; i < 12; i++) dgFloat(&fx_bubble, m.x - 120 + random(0, 150), DG_BOX_H - 24 - random(0, 50), 60 + random(0, 40), 1200, i * 90);
}
static void dgFxFlash() {
  lv_obj_t *f = shape(dFx, 0, 0, DG_BOX_W, DG_BOX_H, 0xFFFFFF, 0, LV_OPA_COVER);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, f);
  lv_anim_set_exec_cb(&a, fxBgOpa);
  lv_anim_set_values(&a, 240, 0);
  lv_anim_set_time(&a, 800);
  lv_anim_set_ready_cb(&a, fxDel);
  lv_anim_start(&a);
  for (int i = 0; i < 10; i++) dgFloat(&fx_coin, random(40, DG_BOX_W - 40), DG_BOX_H - random(10, 60), 80, 1300, 200 + i * 80);
}

// ---------- garden scene ----------
static void dgDrawRoom() {
  bool night = dg.sleeping || dgNight();
  int sig = night | (dg.decor << 1);
  if (sig == dRoomSig) return;
  dRoomSig = sig;
  lv_obj_clean(dRoom);
  lv_obj_t *bg = lv_img_create(dRoom);
  lv_img_set_src(bg, &sc_garden);
  lv_obj_set_pos(bg, -4, -2);
  lv_obj_clear_flag(bg, LV_OBJ_FLAG_CLICKABLE);
  static const int order[6] = {3, 4, 2, 0, 5, 1};  // back to front
  for (int k = 0; k < 6; k++) {
    int i = order[k];
    if (!(dg.decor & (1 << i))) continue;
    lv_obj_t *im = lv_img_create(dRoom);
    lv_img_set_src(im, DECOR[i].img);
    lv_obj_set_pos(im, DECOR[i].x, DECOR[i].y);
    lv_obj_clear_flag(im, LV_OBJ_FLAG_CLICKABLE);
  }
  if (night) {  // dusk-blue veil, moon over the sun, a few stars; lamps glow through
    shape(dRoom, 0, 0, DG_BOX_W, DG_BOX_H, 0x0A1440, 0, LV_OPA_50);
    shape(dRoom, 386, 8, 28, 28, 0xFFF3C4, 4);
    shape(dRoom, 396, 4, 24, 24, 0x2A3360, 4);
    for (int i = 0; i < 9; i++) shape(dRoom, 30 + i * 41, 8 + (i * 17) % 40, 3, 3, 0xFFFFFF, 0);
    if (dg.decor & 2) shape(dRoom, DECOR[1].x - 14, DECOR[1].y - 8, 64, 48, 0xFFE08A, LV_RADIUS_CIRCLE, LV_OPA_30);
  }
}

// ---------- petting ----------
static void dgPet(lv_point_t p) {
  if (dg.sleeping) {
    dgSay(DL("Suỵt, rồng đang ngủ...", "Shh, the dragon is sleeping..."));
    return;
  }
  if (millis() - dPetHourStart > 3600000UL) {
    dPetHourStart = millis();
    dPetCount = 0;
  }
  if (dPetCount++ < 20) {  // petting lifts the mood a little, up to 20 times an hour
    dg.mood = dgClamp(dg.mood + 3);
    dg.pets++;
  }
  dgHearts(2, p.x, p.y);
  if (dPetCount == 1) SFX("pet", SND_CHIRP);
}
static void dgBoxEvent(lv_event_t *e) {
  if (dg.stage == ST_EGG || dgBusy()) return;
  lv_event_code_t code = lv_event_get_code(e);
  lv_point_t p;
  lv_area_t a;
  lv_indev_get_point(lv_indev_get_act(), &p);
  lv_obj_get_coords(dBox, &a);
  p.x -= a.x1;
  p.y -= a.y1;
  if (code == LV_EVENT_PRESSING) {
    lv_point_t v;
    lv_indev_get_vect(lv_indev_get_act(), &v);
    dPetDist += abs(v.x) + abs(v.y);
    if (dPetDist > 70 && millis() - dPetAt > 350) {
      dPetDist = 0;
      dPetAt = millis();
      dgPet(p);
    }
  } else if (code == LV_EVENT_SHORT_CLICKED) {
    if (!dg.sleeping) {
      soundPlay(SND(SND_CHIRP), 0, false);
      dgHearts(1, p.x, p.y);
    }
  } else if (code == LV_EVENT_RELEASED) {
    dPetDist = 0;
  }
}

// ---------- story ----------
static void dgShowStory(int idx, bool evolved) {
  if (!dScr || idx < 0 || idx >= ST_COUNT) return;
  if (evolved) {
    dgFxFlash();
    if (idx == ST_BABY) SFX("hatch", SND_EVOLVE);  // the egg hatches
    else soundPlay(SND(SND_EVOLVE), 0, false);
  }
  lv_obj_t *ov = dgOpenOverlay();
  lv_obj_t *card = pxPanel(ov, 400, 236);
  lv_obj_center(card);
  lv_obj_t *t = pxText(card, evolved ? DL("Tiến hóa!", "Evolution!") : DL("Quả trứng bí ẩn", "A mysterious egg"), F_HEADING);
  lv_obj_set_style_text_color(t, lv_color_hex(0xD0582A), 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 12);
  lv_obj_t *n = pxText(card, dgStageName(idx), F_TITLE);
  lv_obj_align(n, LV_ALIGN_TOP_MID, 0, 52);
  lv_obj_t *s = pxText(card, isVi() ? STORY_VI[idx] : STORY_EN[idx], F_BODY);
  lv_obj_set_width(s, 360);
  lv_label_set_long_mode(s, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(s, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(s, LV_ALIGN_TOP_MID, 0, 86);
  lv_obj_t *b = pxButton(card, 150, 40, nullptr, DL("Tuyệt vời!", "Awesome!"), dgOverlayDone, nullptr);
  lv_obj_align(b, LV_ALIGN_BOTTOM_MID, 0, -10);
}

static lv_obj_t *dgResultCard(lv_obj_t *ov, const char *line1, const char *line2, lv_event_cb_t done) {
  lv_obj_t *card = pxPanel(ov, 300, 170);
  lv_obj_center(card);
  lv_obj_t *t = pxText(card, DL("Kết thúc!", "Game over!"), F_HEADING);
  lv_obj_set_style_text_color(t, lv_color_hex(0xD0582A), 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 12);
  lv_obj_t *r = pxText(card, line1, F_BODY);
  lv_obj_align(r, LV_ALIGN_TOP_MID, 0, 58);
  lv_obj_t *b2 = pxText(card, line2, F_CAPTION);
  lv_obj_align(b2, LV_ALIGN_TOP_MID, 0, 84);
  lv_obj_t *btn = pxButton(card, 120, 40, nullptr, DL("Xong", "Done"), done, nullptr);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
  return card;
}

static bool dgCanPlay() {
  if (dgBlockedAsleep()) return false;
  if (dg.energy < 15) {
    dgSay(DL("Rồng mệt quá, cho nghỉ đã.", "Too tired to play."));
    return false;
  }
  if (dg.sick) {
    dgSay(DL("Rồng đang ốm, chưa chơi được.", "Too sick to play."));
    return false;
  }
  return true;
}

// ---- mini game 1: catch the gems ----
static void mgEnd();
static void mgJewelTap(lv_event_t *e) {
  if (mg.over) return;
  mg.score++;
  setTextFmt(mg.lblScore, "%d", mg.score);
  SFX("gem", SND_TAP);
  lv_obj_del_async((lv_obj_t *)lv_event_get_target(e));
  mg.jewel = nullptr;
}
static void mgSpawn() {
  mg.jewel = lv_img_create(mg.ov);
  lv_img_set_src(mg.jewel, &fx_gem);
  lv_img_set_antialias(mg.jewel, false);
  lv_img_set_zoom(mg.jewel, 256 * 4);
  lv_img_set_size_mode(mg.jewel, LV_IMG_SIZE_MODE_REAL);
  lv_obj_set_pos(mg.jewel, 20 + random(0, 380), 70 + random(0, 180));
  static const uint32_t tint[] = {0xFF4D7A, 0x38E1FF, 0x3DEBA0, 0xFFC93C, 0xA16BFF};
  lv_obj_set_style_img_recolor(mg.jewel, lv_color_hex(tint[random(0, 5)]), 0);
  lv_obj_set_style_img_recolor_opa(mg.jewel, LV_OPA_50, 0);
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
  setTextFmt(mg.lblTime, "%d", (int)((12000 - el + 999) / 1000));
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
  dgFinishPlay(score, 0);
  char l1[64], l2[40];
  snprintf(l1, sizeof(l1), isVi() ? "Bắt được %d ngọc  (+%d xu)" : "Caught %d gems  (+%d coins)", score, score);
  snprintf(l2, sizeof(l2), isVi() ? "Kỷ lục: %d" : "Best: %d", (int)dg.best);
  dgResultCard(mg.ov, l1, l2, mgClose);
  if (score > 0) SFX("win", SND_OK);
  else SFX("gameover", SND_ERROR);
}
static void dgStartGems() {
  mg = MiniGame();
  mg.ov = lv_obj_create(dScr);
  lv_obj_remove_style_all(mg.ov);
  lv_obj_set_size(mg.ov, 480, 320);
  lv_obj_add_flag(mg.ov, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *bg = lv_img_create(mg.ov);  // play on the garden
  lv_img_set_src(bg, &sc_garden);
  lv_obj_set_pos(bg, 0, 128);
  lv_obj_set_style_bg_color(mg.ov, lv_color_hex(0x86D3F5), 0);
  lv_obj_set_style_bg_opa(mg.ov, LV_OPA_COVER, 0);
  lv_obj_t *top = pxPanel(mg.ov, 472, 40);
  lv_obj_set_pos(top, 4, SB_H + 2);
  lv_obj_t *hint = pxText(top, DL("Chạm vào ngọc thật nhanh!", "Tap the gems fast!"), F_LABEL);
  lv_obj_center(hint);
  mg.lblScore = pxText(top, "0", F_TITLE);
  lv_obj_align(mg.lblScore, LV_ALIGN_LEFT_MID, 12, 0);
  mg.lblTime = pxText(top, "12", F_TITLE);
  lv_obj_set_style_text_color(mg.lblTime, lv_color_hex(0xD0582A), 0);
  lv_obj_align(mg.lblTime, LV_ALIGN_RIGHT_MID, -12, 0);
  mg.startMs = millis();
  mg.nextSpawn = mg.startMs + 400;
  mg.t = lv_timer_create(mgTick, 60, nullptr);
}

// ---- mini game 2: dragon flight ----
static const int FB_TOP = SB_H, FB_PX = 70, FB_W = 46, FB_GAP = 138, FB_SPACING = 200;
static void fbPlaceCol(int i) {
  int top = fb.gapY[i] - FB_GAP / 2, bot = fb.gapY[i] + FB_GAP / 2;
  lv_obj_set_pos(fb.col[i][0], fb.cx[i], FB_TOP - 12);
  lv_obj_set_height(fb.col[i][0], top - FB_TOP + 12);
  lv_obj_set_pos(fb.col[i][1], fb.cx[i], bot);
  lv_obj_set_height(fb.col[i][1], 320 - bot + 12);
}
static void fbClose(lv_event_t *) {
  if (fb.t) lv_timer_del(fb.t);
  if (fb.ov) lv_obj_del(fb.ov);
  fb = Flight();
}
static void fbEnd() {
  fb.over = true;
  if (fb.t) lv_timer_del(fb.t);
  fb.t = nullptr;
  dgFinishPlay(fb.score, 1);
  char l1[64], l2[40];
  snprintf(l1, sizeof(l1), isVi() ? "Bay qua %d cột mây  (+%d xu)" : "Passed %d pillars  (+%d coins)", fb.score, fb.score * 2);
  snprintf(l2, sizeof(l2), isVi() ? "Kỷ lục: %d" : "Best: %d", (int)dg.best2);
  dgResultCard(fb.ov, l1, l2, fbClose);
  SFX("gameover", SND_ERROR);
}
static void fbTick(lv_timer_t *) {
  if (!fb.ov || fb.over || !fb.started) return;
  const int ph = fx_flyer.header.h, pw = fx_flyer.header.w;
  fb.vy += 0.45f;
  fb.y += fb.vy;
  if (fb.y < FB_TOP) {
    fb.y = FB_TOP;
    fb.vy = 0;
  }
  lv_obj_set_y(fb.pl, (int)fb.y);
  if (fb.y + ph >= 320) {
    fbEnd();
    return;
  }
  for (int i = 0; i < 3; i++) {
    fb.cx[i] -= 3;
    if (fb.cx[i] < -FB_W) {
      fb.cx[i] += 3 * FB_SPACING;
      fb.gapY[i] = random(FB_TOP + 70, 320 - 70);
      fb.passed[i] = false;
      fbPlaceCol(i);
    } else {
      lv_obj_set_x(fb.col[i][0], fb.cx[i]);
      lv_obj_set_x(fb.col[i][1], fb.cx[i]);
    }
    if (!fb.passed[i] && fb.cx[i] + FB_W < FB_PX) {
      fb.passed[i] = true;
      fb.score++;
      setTextFmt(fb.lbl, "%d", fb.score);
      SFX("gem", SND_TAP);
    }
    int x0 = FB_PX + 14, x1 = FB_PX + pw - 14, y0 = (int)fb.y + 14, y1 = (int)fb.y + ph - 10;
    if (x1 > fb.cx[i] && x0 < fb.cx[i] + FB_W && (y0 < fb.gapY[i] - FB_GAP / 2 || y1 > fb.gapY[i] + FB_GAP / 2)) {
      fbEnd();
      return;
    }
  }
}
static void dgStartFlight() {
  fb = Flight();
  fb.ov = lv_obj_create(dScr);
  lv_obj_remove_style_all(fb.ov);
  lv_obj_set_size(fb.ov, 480, 320);
  lv_obj_set_style_bg_color(fb.ov, lv_color_hex(0x86D3F5), 0);
  lv_obj_set_style_bg_grad_color(fb.ov, lv_color_hex(0xD4F0FA), 0);
  lv_obj_set_style_bg_grad_dir(fb.ov, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(fb.ov, LV_OPA_COVER, 0);
  lv_obj_add_flag(fb.ov, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(fb.ov, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(
      fb.ov,
      [](lv_event_t *) {
        if (fb.over) return;
        if (!fb.started) {
          fb.started = true;
          lv_obj_del(fb.hint);
          fb.hint = nullptr;
          SFX("fire", SND_CHIRP);  // take-off roar
        }
        SFX("flap", SND_TAP);
        fb.vy = -6.6f;
      },
      LV_EVENT_PRESSED, nullptr);
  for (int i = 0; i < 3; i++) {
    fb.cx[i] = 480 + i * FB_SPACING;
    fb.gapY[i] = random(FB_TOP + 70, 320 - 70);
    for (int k = 0; k < 2; k++) {  // white pixel-cloud pillars with a brown outline
      lv_obj_t *c = shape(fb.ov, 0, 0, FB_W, 10, 0xFFFFFF, 6, LV_OPA_COVER);
      lv_obj_set_style_border_width(c, 3, 0);
      lv_obj_set_style_border_color(c, lv_color_hex(PX_INK), 0);
      fb.col[i][k] = c;
    }
    fbPlaceCol(i);
  }
  fb.pl = lv_img_create(fb.ov);
  lv_img_set_src(fb.pl, &fx_flyer);
  lv_img_set_antialias(fb.pl, false);
  lv_obj_set_pos(fb.pl, FB_PX, (int)fb.y);
  lv_obj_clear_flag(fb.pl, LV_OBJ_FLAG_CLICKABLE);
  fb.lbl = pxText(fb.ov, "0", F_HEADING);
  lv_obj_align(fb.lbl, LV_ALIGN_TOP_MID, 0, SB_H + 6);
  fb.hint = pxPanel(fb.ov, 300, 70);
  lv_obj_align(fb.hint, LV_ALIGN_CENTER, 40, 0);
  lv_obj_t *ht = pxText(fb.hint, DL("Chạm để vỗ cánh, bay qua khe giữa các cột mây!", "Tap to flap and fly through the gaps!"), F_LABEL);
  lv_obj_set_width(ht, 270);
  lv_label_set_long_mode(ht, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(ht, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(ht);
  fb.t = lv_timer_create(fbTick, 30, nullptr);
}

// ---- play menu ----
static void dgOpenPlay() {
  if (!dgCanPlay()) return;
  lv_obj_t *ov = dgOpenOverlay();
  lv_obj_t *card = pxPanel(ov, 360, 196);
  lv_obj_center(card);
  lv_obj_t *t = pxText(card, DL("Chơi trò gì?", "Which game?"), F_TITLE);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 10);
  static char b1[64], b2[64];
  snprintf(b1, sizeof(b1), isVi() ? "Bắt ngọc  (kỷ lục %d)" : "Gem catch  (best %d)", (int)dg.best);
  snprintf(b2, sizeof(b2), isVi() ? "Rồng bay  (kỷ lục %d)" : "Dragon flight  (best %d)", (int)dg.best2);
  lv_obj_t *g1 = pxButton(card, 320, 42, nullptr, b1, [](lv_event_t *) {
    dgCloseOverlay();
    dgStartGems();
  }, nullptr);
  lv_obj_align(g1, LV_ALIGN_TOP_MID, 0, 44);
  lv_obj_t *g2 = pxButton(card, 320, 42, nullptr, b2, [](lv_event_t *) {
    dgCloseOverlay();
    dgStartFlight();
  }, nullptr);
  lv_obj_align(g2, LV_ALIGN_TOP_MID, 0, 92);
  lv_obj_t *c = pxButton(card, 120, 34, nullptr, DL("Đóng", "Close"), dgOverlayDone, nullptr);
  lv_obj_align(c, LV_ALIGN_BOTTOM_MID, 0, -8);
}

// ---- feeding menu ----
static void feedPick(lv_event_t *e) {
  int kind = (int)(intptr_t)lv_event_get_user_data(e);
  dgCloseOverlay();
  uint32_t fedBefore = dg.fed;
  dgFeed(kind);
  if (dg.fed != fedBefore) {
    dgFxFeed(kind);
    SFX("eat", SND_CHIRP);
  }
}
static void dgOpenFeed() {
  if (dgBlockedAsleep()) return;
  lv_obj_t *ov = dgOpenOverlay();
  lv_obj_t *card = pxPanel(ov, 420, 200);
  lv_obj_center(card);
  lv_obj_t *t = pxText(card, DL("Cho rồng ăn gì?", "What to feed?"), F_TITLE);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 10);
  static const lv_img_dsc_t *foods[3] = {&fx_food0, &fx_food1, &fx_food2};
  const char *names[3] = {DL("Trái cây\nmiễn phí", "Fruit\nfree"), DL("Thịt nướng\n5 xu", "Grilled meat\n5 coins"),
                          DL("Bánh ngọc\n15 xu", "Jewel cake\n15 coins")};
  for (int i = 0; i < 3; i++) {
    lv_obj_t *b = pxButton(card, 124, 104, foods[i], names[i], feedPick, (void *)(intptr_t)i);
    lv_obj_set_style_text_align(lv_obj_get_child(b, 1), LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(b, 12 + i * 132, 42);
  }
  lv_obj_t *c = pxButton(card, 100, 30, nullptr, DL("Đóng", "Close"), dgOverlayDone, nullptr);
  lv_obj_align(c, LV_ALIGN_BOTTOM_MID, 0, -6);
}

// ---- shop: garden decorations ----
static void shopBuy(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (dgBuyDecor(i)) {
    soundPlay(SND(SND_COIN), 0, false);
    dgSay(String(DL("Đã mua ", "Bought ")) + (isVi() ? DECOR[i].vi : DECOR[i].en) + "!");
    dgCloseOverlay();
    dgDrawRoom();
    dgHearts(3, DECOR[i].x + DECOR[i].img->header.w / 2, DECOR[i].y + 10);
  }
}
static void dgOpenShop() {
  lv_obj_t *ov = dgOpenOverlay();
  lv_obj_t *card = pxPanel(ov, 452, 276);
  lv_obj_align(card, LV_ALIGN_TOP_MID, 0, SB_H + 8);
  lv_obj_t *t = pxText(card, DL("Trang trí khu vườn", "Decorate the garden"), F_TITLE);
  lv_obj_set_pos(t, 14, 8);
  lv_obj_t *cn = pxText(card, "", F_LABEL);
  setTextFmt(cn, "%u %s", (unsigned)dg.coins, DL("xu", "coins"));
  lv_obj_align(cn, LV_ALIGN_TOP_RIGHT, -14, 10);
  for (int i = 0; i < 6; i++) {
    bool owned = dg.decor & (1 << i);
    lv_obj_t *it = pxPanel(card, 206, 62);
    lv_obj_set_pos(it, 12 + (i % 2) * 216, 40 + (i / 2) * 68);
    lv_obj_set_style_bg_color(it, lv_color_hex(0xFFE9C4), LV_STATE_PRESSED);
    lv_obj_t *im = lv_img_create(it);
    lv_img_set_src(im, DECOR[i].img);
    lv_img_set_antialias(im, false);
    const lv_img_dsc_t *d = DECOR[i].img;
    float z = 46.0f / d->header.h;
    if (56.0f / d->header.w < z) z = 56.0f / d->header.w;
    if (z > 1) z = 1;
    lv_img_set_zoom(im, (uint16_t)(256 * z));
    lv_obj_align(im, LV_ALIGN_LEFT_MID, 30 - d->header.w / 2, 0);
    lv_obj_t *n = pxText(it, isVi() ? DECOR[i].vi : DECOR[i].en, F_BODY);
    lv_obj_set_pos(n, 66, 8);
    lv_obj_t *p = pxText(it, "", F_CAPTION);
    if (owned) setText(p, DL("Đã có", "Owned"));
    else setTextFmt(p, "%u %s", (unsigned)DECOR[i].price, DL("xu", "coins"));
    lv_obj_set_style_text_color(p, lv_color_hex(owned ? 0x2E8B3E : 0xD0582A), 0);
    lv_obj_set_pos(p, 66, 32);
    if (!owned) {
      lv_obj_add_flag(it, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(it, shopBuy, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
  }
  lv_obj_t *c = pxButton(card, 110, 30, nullptr, DL("Đóng", "Close"), dgOverlayDone, nullptr);
  lv_obj_align(c, LV_ALIGN_BOTTOM_MID, 0, -6);
}

// ---- badges ----
static lv_obj_t *achDesc;
static void dgOpenBadges() {
  lv_obj_t *ov = dgOpenOverlay();
  lv_obj_t *card = pxPanel(ov, 452, 276);
  lv_obj_align(card, LV_ALIGN_TOP_MID, 0, SB_H + 8);
  int got = 0;
  for (int i = 0; i < 12; i++) got += (dg.ach >> i) & 1;
  lv_obj_t *t = pxText(card, "", F_TITLE);
  setTextFmt(t, isVi() ? "Huy hiệu  %d/12" : "Badges  %d/12", got);
  lv_obj_set_pos(t, 14, 8);
  lv_obj_t *st = pxText(card, "", F_CAPTION);
  setTextFmt(st, isVi() ? "Chuỗi %u ngày  -  Thế hệ %u" : "Streak %u days  -  Generation %u", (unsigned)dg.streak, (unsigned)dg.gen);
  lv_obj_align(st, LV_ALIGN_TOP_RIGHT, -14, 14);
  for (int i = 0; i < 12; i++) {
    bool on = dg.ach & (1 << i);
    lv_obj_t *cell = pxPanel(card, 104, 56);
    lv_obj_set_pos(cell, 10 + (i % 4) * 108, 40 + (i / 4) * 60);
    lv_obj_set_style_border_width(cell, 2, 0);
    lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(cell, lv_color_hex(0xFFE9C4), LV_STATE_PRESSED);
    lv_obj_t *medal = lv_img_create(cell);
    lv_img_set_src(medal, &ui_badge);
    lv_img_set_antialias(medal, false);
    lv_img_set_zoom(medal, 512);
    lv_img_set_size_mode(medal, LV_IMG_SIZE_MODE_REAL);
    lv_obj_align(medal, LV_ALIGN_TOP_MID, 0, 0);
    if (!on) lv_obj_set_style_img_opa(medal, LV_OPA_30, 0);
    lv_obj_t *n = pxText(cell, isVi() ? ACH[i].vi : ACH[i].en, F_CAPTION);
    if (!on) lv_obj_set_style_text_opa(n, LV_OPA_50, 0);
    lv_obj_align(n, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_add_event_cb(
        cell,
        [](lv_event_t *e) {
          int k = (int)(intptr_t)lv_event_get_user_data(e);
          setTextFmt(achDesc, "%s: %s%s", isVi() ? ACH[k].vi : ACH[k].en, isVi() ? ACH[k].dvi : ACH[k].den,
                                (dg.ach & (1 << k)) ? (isVi() ? "  (đã đạt)" : "  (done)") : "");
        },
        LV_EVENT_CLICKED, (void *)(intptr_t)i);
  }
  achDesc = pxText(card, DL("Chạm vào huy hiệu để xem cách đạt được.", "Tap a badge to see how to earn it."), F_CAPTION);
  lv_obj_set_width(achDesc, 300);
  lv_label_set_long_mode(achDesc, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(achDesc, 14, 226);
  lv_obj_t *c = pxButton(card, 96, 30, nullptr, DL("Đóng", "Close"), dgOverlayDone, nullptr);
  lv_obj_align(c, LV_ALIGN_BOTTOM_RIGHT, -10, -6);
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
  if (dgBusy()) return;
  if (dg.stage == ST_EGG) {
    if (i == 0) {
      dgWarm();
      dgHearts(2, DG_BOX_W / 2, DG_BOX_H / 2);
    }
    return;
  }
  switch (i) {
    case 0: dgOpenFeed(); break;
    case 1: dgOpenPlay(); break;
    case 2: {
      uint32_t before = dg.bathed;
      dgBath();
      if (dg.bathed != before) {
        dgFxBath();
        SFX("bath", SND_OK);
      }
      break;
    }
    case 3: dgToggleSleep(); break;
    case 4: dgMedicine(); break;
  }
}

static void dgWalk() {
  if (dg.stage == ST_EGG || dg.sleeping || dgBusy() || millis() < dNextWalk) return;
  dNextWalk = millis() + 5000 + random(0, 5000);
  int from = lv_obj_get_x(dBob), to = random(-140, 120);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, dBob);
  lv_anim_set_exec_cb(&a, fxX);
  lv_anim_set_values(&a, from, to);
  lv_anim_set_time(&a, 300 + abs(to - from) * 18);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_start(&a);
}

static void dgRefreshUi() {
  if (!dScr || !dgOk) return;
  bool egg = dg.stage == ST_EGG;
  float vals[4] = {egg ? dg.energy : dg.satiety, dg.mood, dg.clean, dg.energy};
  static const uint32_t cols[4] = {0xFFA94D, 0xFF6B9A, 0x4FB8FF, 0x7ED957};
  for (int i = 0; i < 4; i++) {
    lv_obj_t *row = lv_obj_get_parent(dBars[i]);
    if (!egg || i == 0) lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
    lv_bar_set_value(dBars[i], (int)vals[i], LV_ANIM_OFF);
    lv_obj_set_style_bg_color(dBars[i], lv_color_hex(vals[i] < 25 ? 0xE5484D : cols[i]), LV_PART_INDICATOR);
  }
  setTextFmt(dCoins, "%u", (unsigned)dg.coins);
  setText(dName, dgName().c_str());
  setTextFmt(dStageLbl, "%s%s", dgStageName(dg.stage), dg.gen > 1 ? (String(" - G") + dg.gen).c_str() : "");
  int moodLv = dg.mood < 30 ? 0 : dg.mood < 65 ? 1 : 2;
  if (moodLv != dFaceLv) {
    dFaceLv = moodLv;
    lv_img_set_src(dFace, moodLv == 2 ? &fc_happy : moodLv == 1 ? &fc_ok : &fc_sad);
  }
  if (dg.stage == ST_DIVINE) {
    lv_bar_set_value(dXpBar, 100, LV_ANIM_OFF);
    setText(dXpTxt, DL("Cấp cao nhất", "Max level"));
    lv_obj_clear_flag(dReinc, LV_OBJ_FLAG_HIDDEN);
  } else {
    int pct = (int)(dg.growth * 100 / dgNeed());
    lv_bar_set_value(dXpBar, pct > 100 ? 100 : pct, LV_ANIM_OFF);
    setTextFmt(dXpTxt, "%s %d%%", egg ? DL("Ấp trứng", "Hatching") : DL("Tiến hóa", "Evolution"), pct);
    lv_obj_add_flag(dReinc, LV_OBJ_FLAG_HIDDEN);
  }

  // speech bubble
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
  else msg = DL("Xoa vào rồng để vuốt ve nhé!", "Rub the dragon to pet it!");
  if (strcmp(msg.c_str(), lv_label_get_text(dBubbleLbl)) != 0) {
    lv_point_t sz;
    lv_txt_get_size(&sz, msg.c_str(), F_BODY, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_obj_set_width(dBubbleLbl, sz.x > 250 ? 250 : LV_SIZE_CONTENT);
    setText(dBubbleLbl, msg.c_str());
  }

  // buttons (icons only): feed / play / bath / sleep (a bolt to wake up) / medicine; the egg only has "warm"
  static const lv_img_dsc_t *icons[5] = {&ui_food, &ui_play, &ui_bath, &ui_sleep, &ui_med};
  for (int i = 0; i < 5; i++) {
    const lv_img_dsc_t *ic = egg && i == 0 ? &st_heart : (i == 3 && dg.sleeping) ? &st_energy : icons[i];
    lv_obj_t *im = lv_obj_get_child(dBtns[i], 0);
    if (lv_img_get_src(im) != ic) lv_img_set_src(im, ic);
    if (egg && i > 0) lv_obj_add_flag(dBtns[i], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(dBtns[i], LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_set_width(dBtns[0], egg ? 466 : 90);

  // garden and dragon, redrawn only when something visible changed
  dgDrawRoom();
  int eggStep = egg ? (int)(dg.growth * 100 / dgNeed()) / 20 : 0;
  int sig = dg.stage | (dg.sleeping << 4) | (dg.sick << 5) | (moodLv << 6) | (eggStep << 9) | ((egg && dg.energy < 30) << 12);
  if (sig != dSig) {
    dSig = sig;
    lv_obj_clean(dBob);
    if (egg) {
      dgDrawEgg(dBob);
      blinkSet(dBlink, nullptr, 0, 0);
    } else {
      blinkSet(dBlink, dgDrawDragon(dBob, dg.stage, dg.sleeping, dg.sick, moodLv), dg.stage, dgExpr(dg.sleeping, dg.sick, moodLv));
    }
  }
  if (dgCoinFound) {
    dgCoinFound = false;
    lv_point_t m = dgMouth();
    dgFloat(&fx_coin, m.x, m.y + 30, 50, 1200);
    soundPlay(SND(SND_COIN), 0, false);
  }
  dgWalk();
  if (dgPendingEvolve > 0 && !dgBusy()) {
    int st = dgPendingEvolve;
    dgPendingEvolve = -1;
    dgShowStory(st, true);
  }
}

static void dgUiTick(lv_timer_t *) { dgRefreshUi(); }
static void animBob(void *o, int32_t v) { lv_obj_set_y((lv_obj_t *)o, v); }

static void pageDragon() {
  lv_obj_t *s = lv_obj_create(nullptr);
  lv_obj_remove_style_all(s);
  lv_obj_set_style_bg_color(s, lv_color_hex(0xF6E7C8), 0);  // warm parchment behind the panels
  lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
  lv_obj_clear_flag(s, LV_OBJ_FLAG_SCROLLABLE);
  if (!dgOk) {
    lv_obj_t *back = pxButton(s, 52, 36, nullptr, LV_SYMBOL_LEFT, goBack, nullptr);
    lv_obj_set_pos(back, 8, SB_H + 6);
    lv_obj_t *card = pxPanel(s, 456, 130);
    lv_obj_set_pos(card, 12, SB_H + 74);
    lv_obj_t *t = pxText(card, DL("Cần thẻ nhớ microSD", "A microSD card is required"), F_TITLE);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_t *h = pxText(card, DL("Dữ liệu rồng được lưu trên thẻ nhớ. Hãy cắm thẻ rồi khởi động lại thiết bị.", "The dragon is saved on the card. Insert one and restart the device."), F_BODY);
    lv_obj_set_width(h, 410);
    lv_label_set_long_mode(h, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(h, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(h, LV_ALIGN_TOP_MID, 0, 56);
    pageShow(s);
    return;
  }
  dScr = s;
  dSig = dRoomSig = dFaceLv = -1;
  dOv = nullptr;
  mg = MiniGame();
  fb = Flight();
  dNextWalk = millis() + 3000;
  const int TOP = SB_H + 3;

  // header: [back | name, stage]  [mood face | evolution bar]  [coins | shop | badges]
  lv_obj_t *pA = pxPanel(s, 156, 40);  // the whole panel goes back home, not just the arrow
  lv_obj_add_flag(pA, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(pA, lv_color_hex(0xFFE9C4), LV_STATE_PRESSED);
  lv_obj_set_style_bg_img_opa(pA, LV_OPA_TRANSP, LV_STATE_PRESSED);
  lv_obj_add_event_cb(pA, goBack, LV_EVENT_CLICKED, nullptr);
  lv_obj_set_pos(pA, 4, TOP);
  lv_obj_t *back = pxButton(pA, 44, 34, nullptr, LV_SYMBOL_LEFT, goBack, nullptr);  // big, easy to hit
  lv_obj_set_style_border_width(back, 2, 0);
  lv_obj_set_style_text_font(lv_obj_get_child(back, 0), F_TITLE, 0);
  lv_obj_set_ext_click_area(back, 14);
  lv_obj_align(back, LV_ALIGN_LEFT_MID, -2, 0);
  dName = pxText(pA, "", F_LABEL);
  lv_obj_set_width(dName, 100);
  lv_label_set_long_mode(dName, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(dName, 46, 0);
  dStageLbl = pxText(pA, "", F_CAPTION);
  lv_obj_set_width(dStageLbl, 102);
  lv_label_set_long_mode(dStageLbl, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(dStageLbl, 46, 18);

  lv_obj_t *pB = pxPanel(s, 164, 40);
  lv_obj_set_pos(pB, 164, TOP);
  dFace = lv_img_create(pB);
  lv_img_set_antialias(dFace, false);
  lv_img_set_src(dFace, &fc_happy);
  lv_img_set_zoom(dFace, 512);
  lv_img_set_size_mode(dFace, LV_IMG_SIZE_MODE_REAL);
  lv_obj_align(dFace, LV_ALIGN_LEFT_MID, 0, 0);
  dXpTxt = pxText(pB, "", F_CAPTION);
  lv_obj_set_pos(dXpTxt, 38, 0);
  dXpBar = lv_bar_create(pB);
  lv_obj_set_size(dXpBar, 114, 10);
  lv_obj_set_pos(dXpBar, 38, 19);
  lv_bar_set_range(dXpBar, 0, 100);
  lv_obj_set_style_bg_color(dXpBar, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(dXpBar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(dXpBar, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(dXpBar, lv_color_hex(PX_INK), LV_PART_MAIN);
  lv_obj_set_style_radius(dXpBar, 3, LV_PART_MAIN);
  lv_obj_set_style_radius(dXpBar, 2, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(dXpBar, lv_color_hex(0xFF8FB8), LV_PART_INDICATOR);

  lv_obj_t *pC = pxPanel(s, 148, 40);
  lv_obj_set_pos(pC, 332, TOP);
  lv_obj_t *coin = lv_img_create(pC);
  lv_img_set_src(coin, &ui_coin);
  lv_img_set_antialias(coin, false);
  lv_img_set_zoom(coin, 384);
  lv_img_set_size_mode(coin, LV_IMG_SIZE_MODE_REAL);
  lv_obj_align(coin, LV_ALIGN_LEFT_MID, 0, 0);
  dCoins = pxText(pC, "", F_LABEL);
  lv_obj_align(dCoins, LV_ALIGN_LEFT_MID, 28, 0);
  lv_obj_t *bShop = pxButton(pC, 32, 30, &ui_shop, nullptr, [](lv_event_t *) {
    if (!dgBusy()) dgOpenShop();
  }, nullptr);
  lv_obj_set_style_border_width(bShop, 2, 0);
  lv_obj_align(bShop, LV_ALIGN_RIGHT_MID, -38, 0);
  lv_obj_t *bBadge = pxButton(pC, 32, 30, &ui_badge, nullptr, [](lv_event_t *) {
    if (!dgBusy()) dgOpenBadges();
  }, nullptr);
  lv_obj_set_style_border_width(bBadge, 2, 0);
  lv_obj_align(bBadge, LV_ALIGN_RIGHT_MID, -2, 0);

  // scene
  dBox = lv_obj_create(s);
  lv_obj_remove_style_all(dBox);
  lv_obj_set_size(dBox, DG_BOX_W, DG_BOX_H);
  lv_obj_set_pos(dBox, 4, TOP + 43);
  lv_obj_set_style_radius(dBox, 6, 0);
  lv_obj_set_style_clip_corner(dBox, true, 0);
  lv_obj_clear_flag(dBox, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(dBox, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(dBox, dgBoxEvent, LV_EVENT_ALL, nullptr);
  dRoom = lv_obj_create(dBox);
  lv_obj_remove_style_all(dRoom);
  lv_obj_set_size(dRoom, DG_BOX_W, DG_BOX_H);
  lv_obj_clear_flag(dRoom, LV_OBJ_FLAG_CLICKABLE);
  dBob = lv_obj_create(dBox);
  lv_obj_remove_style_all(dBob);
  lv_obj_set_size(dBob, DG_BOX_W, DG_BOX_H);
  lv_obj_clear_flag(dBob, LV_OBJ_FLAG_CLICKABLE);
  dFx = lv_obj_create(dBox);
  lv_obj_remove_style_all(dFx);
  lv_obj_set_size(dFx, DG_BOX_W, DG_BOX_H);
  lv_obj_clear_flag(dFx, LV_OBJ_FLAG_CLICKABLE);
  lv_anim_t a;  // breathing: a 2-pixel hop, in pixel steps
  lv_anim_init(&a);
  lv_anim_set_var(&a, dBob);
  lv_anim_set_exec_cb(&a, animBob);
  lv_anim_set_values(&a, 0, -4);
  lv_anim_set_time(&a, 1200);
  lv_anim_set_playback_time(&a, 1200);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&a, lv_anim_path_step);
  lv_anim_start(&a);
  lv_obj_t *frame = lv_obj_create(s);  // pixel frame drawn over the scene edges
  lv_obj_remove_style_all(frame);
  lv_obj_set_size(frame, DG_BOX_W, DG_BOX_H);
  lv_obj_set_pos(frame, 4, TOP + 43);
  lv_obj_set_style_radius(frame, 6, 0);
  lv_obj_set_style_border_width(frame, 3, 0);
  lv_obj_set_style_border_color(frame, lv_color_hex(PX_EDGE), 0);
  lv_obj_clear_flag(frame, LV_OBJ_FLAG_CLICKABLE);

  // needs, top-left inside the scene
  lv_obj_t *stats = pxPanel(dBox, 116, 80);
  lv_obj_set_pos(stats, 8, 8);
  lv_obj_set_style_bg_opa(stats, LV_OPA_80, 0);
  lv_obj_set_style_border_width(stats, 2, 0);
  lv_obj_set_flex_flow(stats, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(stats, 5, 0);
  lv_obj_set_style_pad_row(stats, 1, 0);
  static const lv_img_dsc_t *sicons[4] = {&st_food, &st_heart, &st_clean, &st_energy};
  for (int i = 0; i < 4; i++) {
    lv_obj_t *row = lv_obj_create(stats);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, 104, 16);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *ic = lv_img_create(row);
    lv_img_set_src(ic, sicons[i]);
    lv_img_set_antialias(ic, false);
    lv_obj_align(ic, LV_ALIGN_LEFT_MID, 0, 0);
    dBars[i] = lv_bar_create(row);
    lv_obj_set_size(dBars[i], 80, 8);
    lv_obj_align(dBars[i], LV_ALIGN_LEFT_MID, 20, 0);
    lv_bar_set_range(dBars[i], 0, 100);
    lv_obj_set_style_bg_color(dBars[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dBars[i], LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(dBars[i], 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(dBars[i], lv_color_hex(PX_INK), LV_PART_MAIN);
    lv_obj_set_style_radius(dBars[i], 2, LV_PART_MAIN);
    lv_obj_set_style_radius(dBars[i], 1, LV_PART_INDICATOR);
  }

  // speech bubble, top centre of the scene
  dBubble = lv_obj_create(dBox);
  lv_obj_remove_style_all(dBubble);
  lv_obj_set_size(dBubble, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(dBubble, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_opa(dBubble, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(dBubble, 6, 0);
  lv_obj_set_style_border_width(dBubble, 2, 0);
  lv_obj_set_style_border_color(dBubble, lv_color_hex(PX_INK), 0);
  lv_obj_set_style_pad_hor(dBubble, 8, 0);
  lv_obj_set_style_pad_ver(dBubble, 4, 0);
  lv_obj_clear_flag(dBubble, LV_OBJ_FLAG_CLICKABLE);
  dBubbleLbl = pxText(dBubble, "", F_BODY);
  lv_label_set_long_mode(dBubbleLbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_align(dBubble, LV_ALIGN_TOP_MID, 40, 10);

  dReinc = pxButton(dBox, 104, 32, nullptr, DL("Luân hồi", "Rebirth"), askReincarnate, nullptr);
  lv_obj_align(dReinc, LV_ALIGN_TOP_RIGHT, -8, 8);
  lv_obj_add_flag(dReinc, LV_OBJ_FLAG_HIDDEN);

  // actions
  for (int i = 0; i < 5; i++) {
    lv_obj_t *b = pxButton(s, 90, 52, &ui_food, nullptr, dgBtnCb, (void *)(intptr_t)i);
    lv_img_set_zoom(lv_obj_get_child(b, 0), 256 * 3);  // icon only: 48 px
    lv_obj_set_pos(b, 7 + i * 94, 320 - 54);
    dBtns[i] = b;
  }

  pageTimer(s, dgUiTick, 500);
  dBlink = Blinker();
  pageTimer(s, [](lv_timer_t *) { blinkTick(dBlink); }, 60);
  lv_obj_add_event_cb(
      s,
      [](lv_event_t *) {
        if (mg.t) lv_timer_del(mg.t);
        if (fb.t) lv_timer_del(fb.t);
        mg = MiniGame();
        fb = Flight();
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
