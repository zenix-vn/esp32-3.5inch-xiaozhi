#pragma once
// Speaker on the SPEAK connector (amplifier input on GPIO26), driven with LEDC square-wave tones.
// Non-blocking: sound*() queue a short melody that soundTick() plays from loop().
static const int PIN_SPK = 26;
static const int SPK_CH = 2;

struct Note {
  uint16_t hz, ms;  // hz 0 = rest
};
static const Note *sndSeq = nullptr;
static int sndLen = 0, sndPos = 0, sndRepeat = 0;
static uint32_t sndNext = 0;

static void soundInit() {
  ledcSetup(SPK_CH, 2000, 10);
  ledcAttachPin(PIN_SPK, SPK_CH);
  ledcWrite(SPK_CH, 0);
}

static void soundTone(uint16_t hz) {
  if (!hz || cfg.volume == 0) {
    ledcWrite(SPK_CH, 0);
    return;
  }
  ledcWriteTone(SPK_CH, hz);
  ledcWrite(SPK_CH, 2 + (uint32_t)cfg.volume * 300 / 100);  // duty sets loudness (511 = 50% = loudest)
}

static void soundStop() {
  sndSeq = nullptr;
  sndRepeat = 0;
  ledcWrite(SPK_CH, 0);
}

// repeat: number of extra plays, -1 = until soundStop()
static void soundPlay(const Note *seq, int len, int repeat = 0, bool evenInDnd = false) {
  if (cfg.volume == 0 || (cfg.dnd && !evenInDnd)) return;
  sndSeq = seq;
  sndLen = len;
  sndPos = 0;
  sndRepeat = repeat;
  sndNext = 0;
}

static void soundTick() {
  if (!sndSeq || millis() < sndNext) return;
  if (sndPos >= sndLen) {
    if (sndRepeat == 0) {
      soundStop();
      return;
    }
    if (sndRepeat > 0) sndRepeat--;
    sndPos = 0;
  }
  const Note &n = sndSeq[sndPos++];
  soundTone(n.hz);
  sndNext = millis() + n.ms;
}

static bool soundPlaying() { return sndSeq != nullptr; }

#define SND(arr) arr, (int)(sizeof(arr) / sizeof(arr[0]))
static const Note SND_TAP[] = {{2400, 12}, {0, 1}};
static const Note SND_NOTIFY[] = {{1319, 90}, {0, 30}, {1760, 140}, {0, 1}};
static const Note SND_OK[] = {{1047, 70}, {1568, 110}, {0, 1}};
static const Note SND_ERROR[] = {{392, 140}, {0, 40}, {330, 200}, {0, 1}};
static const Note SND_ALARM[] = {{1760, 120}, {0, 60}, {1760, 120}, {0, 60}, {1760, 120}, {0, 500}};
static const Note SND_CHIRP[] = {{1400, 40}, {1700, 40}, {2100, 50}, {0, 30}, {1900, 60}, {0, 1}};
static const Note SND_EVOLVE[] = {{523, 110}, {659, 110}, {784, 110}, {1047, 220}, {0, 60}, {784, 90}, {1047, 300}, {0, 1}};
static const Note SND_COIN[] = {{1976, 50}, {2637, 120}, {0, 1}};
