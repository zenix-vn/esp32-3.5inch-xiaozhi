#pragma once
// Virtual dragon pet ("Nuôi rồng ảo"): a real-time game. Needs keep draining while the device runs and the
// time the device was off is caught up from the wall clock. Progress lives on the microSD card
// (/zin100/dragon.json). Design and story: see docs.md.
// Included by main.cpp after the weather/shape helpers (uses shape(), makeLabel(), pageBegin(), sdOk, ...).
#include <SD.h>
#include <ArduinoJson.h>
#include "dragon_img.h"
#include "decor_img.h"

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
    {"Chậu cây", "Potted plant", 30, &dc_plant, 6, 82},     {"Đèn ngủ", "Bedside lamp", 40, &dc_lamp, 252, 72},
    {"Thảm tròn", "Round rug", 50, &dc_rug, 71, 112},      {"Bức tranh", "Painting", 60, &dc_painting, 16, 10},
    {"Đèn lồng", "Lantern", 80, &dc_lantern, 112, -4},     {"Rương báu", "Treasure chest", 150, &dc_chest, 196, 96},
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
    soundPlay(SND(SND_COIN), 0, false);
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
// Drawing: pre-rendered sprites (tools/gen_dragons.py) with live eyes on top
// =====================================================================================
static const lv_img_dsc_t *const DRAGON_SPRITES[8] = {&dragon_baby,  &dragon_lv1,  &dragon_lv2,     &dragon_lv3,
                                                       &dragon_adult, &dragon_king, &dragon_emperor, &dragon_divine};
static const int DG_BOX_W = 292, DG_BOX_H = 140, DG_GROUND = 4;  // picture box and feet margin

// zoom > 1 scales the sprite from its top-left corner (used on the home screen, where there is more room)
static lv_obj_t *dgSprite(lv_obj_t *p, const lv_img_dsc_t *src, int x, int y, float zoom = 1.0f) {
  lv_obj_t *img = lv_img_create(p);
  lv_img_set_src(img, src);
  lv_obj_set_pos(img, x, y);
  if (zoom != 1.0f) {
    lv_img_set_pivot(img, 0, 0);
    lv_img_set_zoom(img, (uint16_t)(256 * zoom));
  }
  lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
  return img;
}
static void dgShadow(lv_obj_t *p, int cx, int footY, int w) { shape(p, cx - w / 2, footY - 7, w, 12, 0x000000, LV_RADIUS_CIRCLE, LV_OPA_30); }

static void dgDrawEgg(lv_obj_t *p, int boxW = DG_BOX_W, int boxH = DG_BOX_H, float zoom = 1.0f, bool shadow = true) {
  int prog = dgNeed() > 0 ? (int)(dg.growth * 100 / dgNeed()) : 0;
  const lv_img_dsc_t *src = dg.energy < 30 ? &egg_cold : prog > 80 ? &egg_crack2 : prog > 40 ? &egg_crack1 : &egg_warm;
  int w = src->header.w * zoom, h = src->header.h * zoom;
  if (shadow) dgShadow(p, boxW / 2, boxH - DG_GROUND, w * 3 / 4);
  dgSprite(p, src, (boxW - w) / 2, boxH - DG_GROUND - h, zoom);
}

static void dgDrawDragon(lv_obj_t *p, int st, bool sleeping, bool sick, int moodLv, int boxW = DG_BOX_W, int boxH = DG_BOX_H,
                         float zoom = 1.0f, bool shadow = true) {
  const lv_img_dsc_t *src = DRAGON_SPRITES[st - 1];
  int w = src->header.w * zoom, h = src->header.h * zoom;
  int x = (boxW - w) / 2 + 6, y = boxH - DG_GROUND - h;
  if (shadow) dgShadow(p, x + w / 2, boxH - DG_GROUND, w * 2 / 3);
  lv_obj_t *img = dgSprite(p, src, x, y, zoom);
  if (sick) {
    lv_obj_set_style_img_recolor(img, lv_color_hex(0x7FA060), 0);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_40, 0);
  }
  int ex = x + (int)(DRAGON_EYE[st - 1][0] * zoom), ey = y + (int)(DRAGON_EYE[st - 1][1] * zoom);
  int r = (int)(DRAGON_EYE[st - 1][2] * zoom + 0.5f);
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
// Game page (V2): the dragon lives in a room you decorate, walks around, can be petted, and reacts with
// little effects. Play opens two mini games; the header has the shop (room) and the badges.
// =====================================================================================
static lv_obj_t *dScr = nullptr, *dBox, *dRoom, *dBob, *dFx, *dCoins, *dStageLbl, *dMsg, *dXpBar, *dXpTxt, *dReinc;
static lv_obj_t *dBtns[5], *dBarLbl[4], *dBars[4], *dOv = nullptr;
static int dSig = -1, dRoomSig = -1;
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

// ---------- effects ----------
static void fxDel(lv_anim_t *a) { lv_obj_del((lv_obj_t *)a->var); }
static void fxOpa(void *o, int32_t v) { lv_obj_set_style_opa((lv_obj_t *)o, v, 0); }
static void fxY(void *o, int32_t v) { lv_obj_set_y((lv_obj_t *)o, v); }
static void fxX(void *o, int32_t v) { lv_obj_set_x((lv_obj_t *)o, v); }

// A sprite that drifts up by `rise` px and fades out, then deletes itself.
static lv_obj_t *dgFloat(const lv_img_dsc_t *src, int x, int y, int rise, uint32_t ms, uint32_t delay = 0) {
  if (!dScr) return nullptr;
  lv_obj_t *o = lv_img_create(dFx);
  lv_img_set_src(o, src);
  lv_obj_set_pos(o, x - src->header.w / 2, y - src->header.h / 2);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_opa(o, LV_OPA_TRANSP, 0);
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
  lv_anim_set_exec_cb(&a, fxOpa);
  lv_anim_set_values(&a, 255, 0);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
  lv_anim_set_ready_cb(&a, fxDel);
  lv_anim_start(&a);
  return o;
}

// Where the dragon's mouth is inside the room box (follows the walking offset).
static lv_point_t dgMouth() {
  if (dg.stage == ST_EGG) return {DG_BOX_W / 2, DG_BOX_H / 2};
  const lv_img_dsc_t *src = DRAGON_SPRITES[dg.stage - 1];
  int x = (DG_BOX_W - src->header.w) / 2 + 6 + lv_obj_get_x(dBob), y = DG_BOX_H - DG_GROUND - src->header.h;
  const float *e = DRAGON_EYE[dg.stage - 1];
  return {(lv_coord_t)(x + e[0] + e[2] * 4), (lv_coord_t)(y + e[1] + e[2] * 2)};
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
  for (int i = 0; i < 10; i++) dgFloat(&fx_bubble, m.x - 90 + random(0, 110), DG_BOX_H - 20 - random(0, 40), 50 + random(0, 40), 1200, i * 90);
}
static void dgFxFlash() {
  lv_obj_t *f = shape(dFx, 0, 0, DG_BOX_W, DG_BOX_H, 0xFFFFFF, 0, LV_OPA_COVER);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, f);
  lv_anim_set_exec_cb(&a, fxOpa);
  lv_anim_set_values(&a, 240, 0);
  lv_anim_set_time(&a, 800);
  lv_anim_set_ready_cb(&a, fxDel);
  lv_anim_start(&a);
  for (int i = 0; i < 8; i++) dgFloat(&fx_coin, random(30, DG_BOX_W - 30), DG_BOX_H - random(10, 50), 70, 1300, 200 + i * 80);
}

// ---------- room ----------
static void dgDrawRoom() {
  bool night = dg.sleeping || dgNight();
  int sig = night | (dg.decor << 1);
  if (sig == dRoomSig) return;
  dRoomSig = sig;
  lv_obj_set_style_bg_color(dBox, lv_color_hex(night ? 0x141A3A : 0x3E63B0), 0);
  lv_obj_set_style_bg_grad_color(dBox, lv_color_hex(night ? 0x241A4A : 0x7AA6E6), 0);
  lv_obj_clean(dRoom);
  // window
  lv_obj_t *win = shape(dRoom, 206, 10, 70, 50, 0x6B4F2A, 5);
  lv_obj_t *glass = shape(win, 4, 4, 62, 42, night ? 0x0B1230 : 0x9AD4FF, 3);
  if (night) {
    lv_obj_set_style_bg_grad_color(glass, lv_color_hex(0x1D2A5E), 0);
    shape(glass, 40, 6, 14, 14, 0xFFF3C4);
    shape(glass, 45, 4, 12, 12, 0x0B1230);
    shape(glass, 12, 10, 3, 3, 0xFFFFFF);
    shape(glass, 26, 26, 2, 2, 0xFFFFFF);
  } else {
    lv_obj_set_style_bg_grad_color(glass, lv_color_hex(0xE0F4FF), 0);
    shape(glass, 8, 18, 22, 10, 0xFFFFFF, 5);
    shape(glass, 16, 13, 16, 12, 0xFFFFFF);
  }
  lv_obj_set_style_bg_grad_dir(glass, LV_GRAD_DIR_VER, 0);
  shape(win, 33, 4, 4, 42, 0x6B4F2A, 0);
  shape(win, 4, 23, 62, 4, 0x6B4F2A, 0);
  // floor
  lv_obj_t *floor = shape(dRoom, 0, DG_BOX_H - 34, DG_BOX_W, 34, night ? 0x2E2340 : 0x9A6B45, 0);
  lv_obj_set_style_bg_grad_color(floor, lv_color_hex(night ? 0x1E1730 : 0x7A5236), 0);
  lv_obj_set_style_bg_grad_dir(floor, LV_GRAD_DIR_VER, 0);
  shape(dRoom, 0, DG_BOX_H - 34, DG_BOX_W, 2, night ? 0x4A3A60 : 0xC08A5C, 0);
  // decorations (back to front)
  static const int order[6] = {2, 3, 4, 0, 5, 1};
  for (int k = 0; k < 6; k++) {
    int i = order[k];
    if (!(dg.decor & (1 << i))) continue;
    if (i == 1 && night) shape(dRoom, DECOR[i].x - 16, DECOR[i].y - 12, 68, 52, 0xFFE08A, LV_RADIUS_CIRCLE, LV_OPA_20);  // lamp glow
    lv_obj_t *im = lv_img_create(dRoom);
    lv_img_set_src(im, DECOR[i].img);
    lv_obj_set_pos(im, DECOR[i].x, DECOR[i].y);
    lv_obj_clear_flag(im, LV_OBJ_FLAG_CLICKABLE);
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
  if (dPetCount == 1) soundPlay(SND(SND_CHIRP), 0, false);
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
    soundPlay(SND(SND_EVOLVE), 0, false);
  }
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

static lv_obj_t *dgResultCard(lv_obj_t *ov, const char *line1, const char *line2, lv_event_cb_t done) {
  lv_obj_t *card = makeCard(ov, 300, 170);
  lv_obj_center(card);
  lv_obj_t *t = makeLabel(card, DL("Kết thúc!", "Game over!"), F_HEADING, C_ACCENT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 14);
  lv_obj_t *r = makeLabel(card, line1, F_BODY, C_TEXT);
  lv_obj_align(r, LV_ALIGN_TOP_MID, 0, 62);
  lv_obj_t *b2 = makeLabel(card, line2, F_CAPTION, C_MUTED);
  lv_obj_align(b2, LV_ALIGN_TOP_MID, 0, 90);
  lv_obj_t *btn = smallBtn(card, DL("Xong", "Done"), C_ACCENT, done, nullptr, 120);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -12);
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
  lv_label_set_text_fmt(mg.lblScore, "%d", mg.score);
  soundPlay(SND(SND_TAP), 0, false);
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
  dgFinishPlay(score, 0);
  char l1[64], l2[40];
  snprintf(l1, sizeof(l1), isVi() ? "Bắt được %d ngọc  (+%d xu)" : "Caught %d gems  (+%d coins)", score, score);
  snprintf(l2, sizeof(l2), isVi() ? "Kỷ lục: %d" : "Best: %d", (int)dg.best);
  dgResultCard(mg.ov, l1, l2, mgClose);
  soundPlay(SND(SND_OK), 0, false);
}
static void dgStartGems() {
  mg = MiniGame();
  mg.ov = dgOpenOverlay();
  dOv = nullptr;  // the mini game owns its overlay
  lv_obj_t *hint = makeLabel(mg.ov, DL("Chạm vào ngọc thật nhanh!", "Tap the gems fast!"), F_LABEL, C_TEXT);
  lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, SB_H + 10);
  mg.lblScore = makeLabel(mg.ov, "0", F_HEADING, C_OK);
  lv_obj_align(mg.lblScore, LV_ALIGN_TOP_LEFT, 20, SB_H + 6);
  mg.lblTime = makeLabel(mg.ov, "12", F_HEADING, C_WARN);
  lv_obj_align(mg.lblTime, LV_ALIGN_TOP_RIGHT, -20, SB_H + 6);
  mg.startMs = millis();
  mg.nextSpawn = mg.startMs + 400;
  mg.t = lv_timer_create(mgTick, 60, nullptr);
}

// ---- mini game 2: dragon flight ----
static const int FB_TOP = SB_H, FB_PX = 80, FB_W = 46, FB_GAP = 116, FB_SPACING = 180;
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
  soundPlay(SND(SND_ERROR), 0, false);
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
      lv_label_set_text_fmt(fb.lbl, "%d", fb.score);
      soundPlay(SND(SND_TAP), 0, false);
    }
    // hit box a little smaller than the sprite
    int x0 = FB_PX + 10, x1 = FB_PX + pw - 8, y0 = (int)fb.y + 8, y1 = (int)fb.y + ph - 6;
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
  lv_obj_set_style_bg_color(fb.ov, lv_color_hex(0x2B4C99), 0);
  lv_obj_set_style_bg_grad_color(fb.ov, lv_color_hex(0x8EC0F0), 0);
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
        }
        fb.vy = -6.6f;
      },
      LV_EVENT_PRESSED, nullptr);
  for (int i = 0; i < 3; i++) {
    fb.cx[i] = 480 + i * FB_SPACING;
    fb.gapY[i] = random(FB_TOP + 70, 320 - 70);
    for (int k = 0; k < 2; k++) {
      lv_obj_t *c = shape(fb.ov, 0, 0, FB_W, 10, 0xF4F8FF, 14, LV_OPA_90);
      lv_obj_set_style_border_width(c, 3, 0);
      lv_obj_set_style_border_color(c, lv_color_hex(0xB8CCF0), 0);
      fb.col[i][k] = c;
    }
    fbPlaceCol(i);
  }
  fb.pl = lv_img_create(fb.ov);
  lv_img_set_src(fb.pl, &fx_flyer);
  lv_obj_set_pos(fb.pl, FB_PX, (int)fb.y);
  lv_obj_clear_flag(fb.pl, LV_OBJ_FLAG_CLICKABLE);
  fb.lbl = makeLabel(fb.ov, "0", F_HEADING, C_TEXT);
  lv_obj_align(fb.lbl, LV_ALIGN_TOP_MID, 0, SB_H + 6);
  fb.hint = makeLabel(fb.ov, DL("Chạm để vỗ cánh, bay qua khe giữa các cột mây!", "Tap to flap and fly through the gaps!"), F_LABEL, C_TEXT);
  lv_obj_set_width(fb.hint, 300);
  lv_label_set_long_mode(fb.hint, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(fb.hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(fb.hint, LV_ALIGN_CENTER, 40, 0);
  fb.t = lv_timer_create(fbTick, 30, nullptr);
}

// ---- play menu ----
static void dgOpenPlay() {
  if (!dgCanPlay()) return;
  lv_obj_t *ov = dgOpenOverlay();
  lv_obj_t *card = makeCard(ov, 360, 200);
  lv_obj_center(card);
  lv_obj_t *t = makeLabel(card, DL("Chơi trò gì?", "Which game?"), F_TITLE, C_TEXT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 12);
  char b1[64], b2[64];
  snprintf(b1, sizeof(b1), isVi() ? "Bắt ngọc  (kỷ lục %d)" : "Gem catch  (best %d)", (int)dg.best);
  snprintf(b2, sizeof(b2), isVi() ? "Rồng bay  (kỷ lục %d)" : "Dragon flight  (best %d)", (int)dg.best2);
  lv_obj_t *g1 = smallBtn(card, b1, C_ACCENT, [](lv_event_t *) {
    dgCloseOverlay();
    dgStartGems();
  }, nullptr, 320);
  lv_obj_set_height(g1, 42);
  lv_obj_align(g1, LV_ALIGN_TOP_MID, 0, 48);
  lv_obj_t *g2 = smallBtn(card, b2, C_ACCENT2, [](lv_event_t *) {
    dgCloseOverlay();
    dgStartFlight();
  }, nullptr, 320);
  lv_obj_set_height(g2, 42);
  lv_obj_align(g2, LV_ALIGN_TOP_MID, 0, 96);
  lv_obj_t *c = smallBtn(card, DL("Đóng", "Close"), C_MUTED, dgOverlayDone, nullptr, 120);
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
    soundPlay(SND(SND_CHIRP), 0, false);
  }
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

// ---- shop: room decorations ----
static void dgOpenShop();
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
  lv_obj_t *card = makeCard(ov, 452, 276);
  lv_obj_align(card, LV_ALIGN_TOP_MID, 0, SB_H + 8);
  lv_obj_t *t = makeLabel(card, DL("Trang trí phòng", "Decorate the room"), F_TITLE, C_TEXT);
  lv_obj_set_pos(t, 16, 10);
  lv_obj_t *cn = makeLabel(card, "", F_LABEL, C_WARN);
  lv_label_set_text_fmt(cn, "%u %s", (unsigned)dg.coins, DL("xu", "coins"));
  lv_obj_align(cn, LV_ALIGN_TOP_RIGHT, -16, 12);
  for (int i = 0; i < 6; i++) {
    bool owned = dg.decor & (1 << i);
    lv_obj_t *it = makeCard(card, 206, 62);
    lv_obj_set_pos(it, 14 + (i % 2) * 218, 44 + (i / 2) * 68);
    lv_obj_set_style_bg_color(it, C_BORDER, 0);
    lv_obj_set_style_bg_color(it, C_ACCENT2, LV_STATE_PRESSED);
    lv_obj_t *im = lv_img_create(it);
    lv_img_set_src(im, DECOR[i].img);
    const lv_img_dsc_t *d = DECOR[i].img;
    float z = 46.0f / d->header.h;
    if (56.0f / d->header.w < z) z = 56.0f / d->header.w;
    if (z > 1) z = 1;
    lv_img_set_zoom(im, (uint16_t)(256 * z));
    lv_obj_align(im, LV_ALIGN_LEFT_MID, 32 - d->header.w / 2, 0);
    lv_obj_t *n = makeLabel(it, isVi() ? DECOR[i].vi : DECOR[i].en, F_BODY, C_TEXT);
    lv_obj_set_pos(n, 70, 10);
    lv_obj_t *p = makeLabel(it, "", F_CAPTION, owned ? C_OK : C_WARN);
    if (owned) lv_label_set_text(p, DL("Đã có", "Owned"));
    else lv_label_set_text_fmt(p, "%u %s", (unsigned)DECOR[i].price, DL("xu", "coins"));
    lv_obj_set_pos(p, 70, 34);
    if (!owned) lv_obj_add_event_cb(it, shopBuy, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    else lv_obj_clear_flag(it, LV_OBJ_FLAG_CLICKABLE);
  }
  lv_obj_t *c = smallBtn(card, DL("Đóng", "Close"), C_MUTED, dgOverlayDone, nullptr, 120);
  lv_obj_set_height(c, 30);
  lv_obj_align(c, LV_ALIGN_BOTTOM_MID, 0, -6);
}

// ---- badges ----
static lv_obj_t *achDesc;
static void dgOpenBadges() {
  lv_obj_t *ov = dgOpenOverlay();
  lv_obj_t *card = makeCard(ov, 452, 276);
  lv_obj_align(card, LV_ALIGN_TOP_MID, 0, SB_H + 8);
  int got = 0;
  for (int i = 0; i < 12; i++) got += (dg.ach >> i) & 1;
  lv_obj_t *t = makeLabel(card, "", F_TITLE, C_TEXT);
  lv_label_set_text_fmt(t, isVi() ? "Huy hiệu  %d/12" : "Badges  %d/12", got);
  lv_obj_set_pos(t, 16, 10);
  lv_obj_t *st = makeLabel(card, "", F_CAPTION, C_MUTED);
  lv_label_set_text_fmt(st, isVi() ? "Chuỗi %u ngày  -  Thế hệ %u" : "Streak %u days  -  Generation %u", (unsigned)dg.streak, (unsigned)dg.gen);
  lv_obj_align(st, LV_ALIGN_TOP_RIGHT, -16, 16);
  for (int i = 0; i < 12; i++) {
    bool on = dg.ach & (1 << i);
    lv_obj_t *cell = lv_obj_create(card);
    lv_obj_remove_style_all(cell);
    lv_obj_set_size(cell, 104, 56);
    lv_obj_set_pos(cell, 12 + (i % 4) * 108, 42 + (i / 4) * 60);
    lv_obj_set_style_radius(cell, 12, 0);
    lv_obj_set_style_bg_color(cell, C_BORDER, 0);
    lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(cell, C_ACCENT2, LV_STATE_PRESSED);
    lv_obj_t *medal = shape(cell, 38, 4, 28, 28, on ? 0xFFC93C : 0x3A4470);
    lv_obj_set_style_border_width(medal, 3, 0);
    lv_obj_set_style_border_color(medal, lv_color_hex(on ? 0xFFF0B0 : 0x4E5880), 0);
    lv_obj_t *n = makeLabel(cell, isVi() ? ACH[i].vi : ACH[i].en, F_CAPTION, on ? C_TEXT : C_MUTED);
    lv_obj_align(n, LV_ALIGN_BOTTOM_MID, 0, -3);
    lv_obj_add_event_cb(
        cell,
        [](lv_event_t *e) {
          int k = (int)(intptr_t)lv_event_get_user_data(e);
          lv_label_set_text_fmt(achDesc, "%s: %s%s", isVi() ? ACH[k].vi : ACH[k].en, isVi() ? ACH[k].dvi : ACH[k].den,
                                (dg.ach & (1 << k)) ? (isVi() ? "  (đã đạt)" : "  (done)") : "");
        },
        LV_EVENT_CLICKED, (void *)(intptr_t)i);
  }
  achDesc = makeLabel(card, DL("Chạm vào huy hiệu để xem cách đạt được.", "Tap a badge to see how to earn it."), F_CAPTION, C_ACCENT);
  lv_obj_set_width(achDesc, 300);
  lv_label_set_long_mode(achDesc, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(achDesc, 16, 230);
  lv_obj_t *c = smallBtn(card, DL("Đóng", "Close"), C_MUTED, dgOverlayDone, nullptr, 100);
  lv_obj_set_height(c, 30);
  lv_obj_align(c, LV_ALIGN_BOTTOM_RIGHT, -12, -6);
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
      if (dg.bathed != before) dgFxBath();
      break;
    }
    case 3: dgToggleSleep(); break;
    case 4: dgMedicine(); break;
  }
}

static lv_color_t dgBarColor(int i, float v) {
  static const uint32_t cols[4] = {0xFFB84A, 0xFF6B9A, 0x38E1FF, 0x3DEBA0};
  return v < 25 ? C_DANGER : lv_color_hex(cols[i]);
}

static void dgWalk() {
  if (dg.stage == ST_EGG || dg.sleeping || dgBusy() || millis() < dNextWalk) return;
  dNextWalk = millis() + 5000 + random(0, 5000);
  int from = lv_obj_get_x(dBob), to = random(-55, 45);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, dBob);
  lv_anim_set_exec_cb(&a, fxX);
  lv_anim_set_values(&a, from, to);
  lv_anim_set_time(&a, 300 + abs(to - from) * 22);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_start(&a);
}

static void dgRefreshUi() {
  if (!dScr || !dgOk) return;
  bool egg = dg.stage == ST_EGG;
  float vals[4] = {egg ? dg.energy : dg.satiety, dg.mood, dg.clean, dg.energy};
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
  else msg = DL("Xoa vào rồng để vuốt ve nhé!", "Rub the dragon to pet it!");
  lv_label_set_text(dMsg, msg.c_str());

  // buttons
  const char *bl[5] = {egg ? DL("Ấp", "Warm") : DL("Ăn", "Feed"), DL("Chơi", "Play"), DL("Tắm", "Bath"), dg.sleeping ? DL("Dậy", "Wake") : DL("Ngủ", "Sleep"), DL("Thuốc", "Medicine")};
  for (int i = 0; i < 5; i++) {
    lv_label_set_text(lv_obj_get_child(dBtns[i], 0), bl[i]);
    if (egg && i > 0) lv_obj_add_flag(dBtns[i], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(dBtns[i], LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_set_width(dBtns[0], egg ? 464 : 88);

  // room and dragon, redrawn only when something visible changed
  dgDrawRoom();
  int moodLv = dg.mood < 30 ? 0 : dg.mood < 65 ? 1 : 2;
  int eggStep = egg ? (int)(dg.growth * 100 / dgNeed()) / 20 : 0;
  int sig = dg.stage | (dg.sleeping << 4) | (dg.sick << 5) | (moodLv << 6) | (eggStep << 9) | ((egg && dg.energy < 30) << 12);
  if (sig != dSig) {
    dSig = sig;
    lv_obj_clean(dBob);
    if (egg) dgDrawEgg(dBob);
    else dgDrawDragon(dBob, dg.stage, dg.sleeping, dg.sick, moodLv);
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
  lv_obj_t *c;
  lv_obj_t *s = pageBegin("Dragon", &c);
  lv_obj_del(c);
  if (!dgOk) {
    lv_obj_t *card = makeCard(s, 456, 120);
    lv_obj_set_pos(card, 12, SB_H + 74);
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
  dSig = dRoomSig = -1;
  dOv = nullptr;
  mg = MiniGame();
  fb = Flight();
  dNextWalk = millis() + 3000;

  dCoins = makeLabel(s, "", F_LABEL, C_WARN);
  lv_obj_align(dCoins, LV_ALIGN_TOP_RIGHT, -14, SB_H + 14);
  lv_obj_t *bBadge = smallBtn(s, DL("Huy hiệu", "Badges"), C_ACCENT2, [](lv_event_t *) {
    if (!dgBusy()) dgOpenBadges();
  }, nullptr, 88);
  lv_obj_align(bBadge, LV_ALIGN_TOP_RIGHT, -96, SB_H + 8);
  lv_obj_t *bShop = smallBtn(s, DL("Phòng", "Room"), C_ACCENT, [](lv_event_t *) {
    if (!dgBusy()) dgOpenShop();
  }, nullptr, 76);
  lv_obj_align(bShop, LV_ALIGN_TOP_RIGHT, -190, SB_H + 8);

  dStageLbl = makeLabel(s, "", F_LABEL, C_ACCENT);
  lv_obj_align(dStageLbl, LV_ALIGN_TOP_LEFT, 14, SB_H + 48);
  dReinc = smallBtn(s, DL("Luân hồi", "Rebirth"), C_ACCENT2, askReincarnate, nullptr, 96);
  lv_obj_set_height(dReinc, 26);
  lv_obj_align(dReinc, LV_ALIGN_TOP_RIGHT, -170, SB_H + 44);
  lv_obj_add_flag(dReinc, LV_OBJ_FLAG_HIDDEN);

  dBox = makeCard(s, DG_BOX_W, DG_BOX_H);
  lv_obj_set_pos(dBox, 10, 92);
  lv_obj_set_style_bg_grad_dir(dBox, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_radius(dBox, 18, 0);
  lv_obj_set_style_clip_corner(dBox, true, 0);
  lv_obj_set_style_border_width(dBox, 0, 0);
  lv_obj_add_flag(dBox, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(dBox, dgBoxEvent, LV_EVENT_ALL, nullptr);
  dRoom = lv_obj_create(dBox);
  lv_obj_remove_style_all(dRoom);
  lv_obj_set_size(dRoom, DG_BOX_W, DG_BOX_H);
  lv_obj_clear_flag(dRoom, LV_OBJ_FLAG_CLICKABLE);
  dBob = lv_obj_create(dBox);
  lv_obj_remove_style_all(dBob);
  lv_obj_set_size(dBob, DG_BOX_W, DG_BOX_H);
  lv_obj_set_pos(dBob, 0, 0);
  lv_obj_clear_flag(dBob, LV_OBJ_FLAG_CLICKABLE);
  dFx = lv_obj_create(dBox);
  lv_obj_remove_style_all(dFx);
  lv_obj_set_size(dFx, DG_BOX_W, DG_BOX_H);
  lv_obj_clear_flag(dFx, LV_OBJ_FLAG_CLICKABLE);
  lv_anim_t a;  // gentle breathing
  lv_anim_init(&a);
  lv_anim_set_var(&a, dBob);
  lv_anim_set_exec_cb(&a, animBob);
  lv_anim_set_values(&a, 0, -4);
  lv_anim_set_time(&a, 1400);
  lv_anim_set_playback_time(&a, 1400);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_start(&a);

  for (int i = 0; i < 4; i++) {
    int y = 92 + i * 34;
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
  lv_obj_set_pos(dXpBar, 10, 238);
  lv_bar_set_range(dXpBar, 0, 100);
  lv_obj_set_style_bg_color(dXpBar, C_BORDER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(dXpBar, C_ACCENT2, LV_PART_INDICATOR);
  dXpTxt = makeLabel(s, "", F_CAPTION, C_MUTED);
  lv_obj_set_pos(dXpTxt, 314, 232);

  dMsg = makeLabel(s, "", F_BODY, C_TEXT);
  lv_obj_set_width(dMsg, 456);
  lv_label_set_long_mode(dMsg, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(dMsg, 14, 250);

  for (int i = 0; i < 5; i++) {
    lv_obj_t *b = lv_btn_create(s);
    lv_obj_set_size(b, 88, 40);
    lv_obj_set_pos(b, 8 + i * 94, 275);
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
