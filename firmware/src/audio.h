#pragma once
// Sound files from the memory card (/sounds/<name>.wav), played through the speaker on GPIO26 = DAC2 of the ESP32.
//
// Two channels are mixed by a small task: FX (interface sounds, alarm, game effects) and BG (a looping background:
// the sleep sounds). A tap sound therefore does not cut the rain. PCM WAV, 8 or 16 bit, mono or stereo, any rate
// (the generator makes 8-bit mono 16 kHz, the cheapest to stream). The I2S peripheral in its built-in-DAC mode does
// the timing through DMA; the driver exists only while something plays (plus 1.5 s), because the speaker pin is
// otherwise used by the LEDC beeps of sound.h, which stay as the fallback when a file is missing.
//
// Included by main.cpp after sdOk exists and before pages.h.
#include <driver/i2s.h>
#include <driver/dac.h>
#include <driver/rtc_io.h>

static const int A_RATE = 16000;
static const int A_BLOCK = 512;  // samples per I2S write = 32 ms

// ---------- which files exist ----------
// The first 40 names are kept in RAM (the interface and game sounds); past that (the kids' letter voices make ~150
// more) audioHas() asks the card itself.
static char audioNames[40][20];
static int audioNameCount = 0;
static bool audioMore = false;  // /sounds holds more files than the list
static volatile bool audioDirty = true;

static void audioRescan() {
  audioNameCount = 0;
  audioMore = false;
  audioDirty = false;
  if (!sdOk) return;
  File dir = SD.open("/sounds");
  if (!dir || !dir.isDirectory()) return;
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    if (audioNameCount >= 40) {
      audioMore = true;
      f.close();
      break;
    }
    String n = f.name();
    int sl = n.lastIndexOf('/');
    if (sl >= 0) n = n.substring(sl + 1);
    String low = n;
    low.toLowerCase();
    if (!f.isDirectory() && low.endsWith(".wav") && n.length() < 24) {
      n = n.substring(0, n.length() - 4);
      strlcpy(audioNames[audioNameCount++], n.c_str(), sizeof(audioNames[0]));
    }
    f.close();
  }
  dir.close();
}

static bool audioHas(const char *name) {
  if (audioDirty) audioRescan();
  for (int i = 0; i < audioNameCount; i++)
    if (strcasecmp(audioNames[i], name) == 0) return true;
  if (!audioMore || !sdOk) return false;
  char path[40];
  snprintf(path, sizeof(path), "/sounds/%s.wav", name);
  return SD.exists(path);
}

// ---------- one WAV stream ----------
struct AChan {
  File f;
  bool on = false, loop = false;
  uint32_t dataStart = 0, dataTotal = 0, dataLeft = 0;
  uint8_t bits = 8, chans = 1;
  uint32_t step = 65536, frac = 0;  // input samples per output sample, 16.16
  uint8_t buf[512];
  int bufLen = 0, bufPos = 0;
  int cur = 128;
};

static bool chRefill(AChan &c) {
  if (c.dataLeft == 0) {
    if (!c.loop) return false;
    c.f.seek(c.dataStart);
    c.dataLeft = c.dataTotal;
  }
  int want = c.dataLeft < sizeof(c.buf) ? (int)c.dataLeft : (int)sizeof(c.buf);
  want -= want % (c.chans * c.bits / 8);
  c.bufLen = want > 0 ? c.f.read(c.buf, want) : 0;
  c.bufPos = 0;
  if (c.bufLen <= 0) {  // the card stopped answering: end the sound (no retry loop)
    c.dataLeft = 0;
    c.bufLen = 0;
    return false;
  }
  c.dataLeft -= c.bufLen;
  return true;
}

// Next input frame -> 0..255 (mono, unsigned 8 bit); false = end of the data.
static bool chFrame(AChan &c) {
  int fb = c.chans * c.bits / 8;
  if (c.bufPos + fb > c.bufLen && !chRefill(c)) return false;
  int sum = 0;
  for (int k = 0; k < c.chans; k++) {
    if (c.bits == 8) sum += c.buf[c.bufPos++];
    else {
      int16_t v = (int16_t)(c.buf[c.bufPos] | (c.buf[c.bufPos + 1] << 8));
      c.bufPos += 2;
      sum += (v >> 8) + 128;
    }
  }
  c.cur = sum / c.chans;
  return true;
}

static void chClose(AChan &c) {
  if (c.f) c.f.close();
  c.on = false;
}

static bool chOpen(AChan &c, const char *name, bool loop) {
  chClose(c);
  char path[48];
  snprintf(path, sizeof(path), "/sounds/%s.wav", name);
  c.f = SD.open(path, FILE_READ);
  if (!c.f) return false;
  uint8_t h[12];
  if (c.f.read(h, 12) != 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4)) {
    c.f.close();
    return false;
  }
  uint32_t rate = 0;
  bool fmtOk = false;
  for (;;) {  // walk the chunks to "fmt " and "data"
    uint8_t ch[8];
    if (c.f.read(ch, 8) != 8) break;
    uint32_t len = ch[4] | (ch[5] << 8) | (ch[6] << 16) | ((uint32_t)ch[7] << 24);
    if (!memcmp(ch, "fmt ", 4) && len >= 16) {
      uint8_t fm[16];
      c.f.read(fm, 16);
      uint16_t tag = fm[0] | (fm[1] << 8);
      c.chans = fm[2] | (fm[3] << 8);
      rate = fm[4] | (fm[5] << 8) | (fm[6] << 16) | ((uint32_t)fm[7] << 24);
      c.bits = fm[14] | (fm[15] << 8);
      fmtOk = tag == 1 && (c.bits == 8 || c.bits == 16) && (c.chans == 1 || c.chans == 2) && rate >= 4000 && rate <= 48000;
      if (len > 16) c.f.seek(c.f.position() + len - 16);
    } else if (!memcmp(ch, "data", 4)) {
      c.dataStart = c.f.position();
      c.dataTotal = len;
      break;
    } else {
      c.f.seek(c.f.position() + len + (len & 1));
    }
  }
  if (!fmtOk || !c.dataTotal) {
    c.f.close();
    return false;
  }
  c.step = (uint32_t)(((uint64_t)rate << 16) / A_RATE);
  c.frac = 0;
  c.dataLeft = c.dataTotal;
  c.bufLen = c.bufPos = 0;
  c.loop = loop;
  c.on = chFrame(c);
  if (!c.on) c.f.close();
  return c.on;
}

// Next output sample (16 kHz, 0..255); closes the channel at the end of a one-shot sound.
static int chNext(AChan &c) {
  c.frac += c.step;
  while (c.frac >= 65536) {
    c.frac -= 65536;
    if (!chFrame(c)) {
      chClose(c);
      return 128;
    }
  }
  return c.cur;
}

// ---------- requests from the UI task, served by the audio task ----------
static volatile bool aFxReq = false, aFxStop = false, aFxLoop = false, aBgReq = false, aBgStop = false;
static char aFxName[24], aBgName[24], aBgPlaying[24];
static volatile uint32_t aBgMinutes = 0, aBgEndsAt = 0;  // sleep timer: millis() when the background fades out, 0 = never
static volatile bool aBgOn = false, aFxOn = false;
static TaskHandle_t aTask = nullptr;

static void audioEnsureTask();

// Radio (radio.h): a ring of 8-bit samples at A_RATE that the radio task fills and the mixer drains. The mixer
// waits until the ring is half full before it plays (and again after running dry), so short network stalls are
// silent pauses instead of crackle. The radio task frees the ring only after clearing aRadioOn and waiting a block.
// Samples are packed 4 to a 32-bit word (sample n = byte n & 3 of word n / 4) so the ring can live in IRAM, which
// takes 32-bit loads and stores only; the writer publishes aRadioHead a whole word at a time.
static uint32_t *volatile aRadioBuf = nullptr;
static uint32_t aRadioSize = 0;                          // in samples, power of two
static volatile uint32_t aRadioHead = 0, aRadioTail = 0;  // samples written / read so far
static volatile bool aRadioOn = false, aRadioPlaying = false;
static volatile bool aRadioNoDriver = false;  // the I2S driver could not start (no heap): the radio gives up

// Interface / game sound. false = no such file (the caller falls back to beeps).
static bool audioFx(const char *name, bool loop = false) {
  if (!audioHas(name)) return false;
  audioEnsureTask();
  strlcpy(aFxName, name, sizeof(aFxName));
  aFxLoop = loop;
  aFxReq = true;
  return true;
}
static void audioFxStop() {
  if (aTask) aFxStop = true;
}
static bool audioFxActive() { return aFxOn; }

// Background loop (sleep sounds); minutes = 0 plays until stopped.
static bool audioBg(const char *name, uint32_t minutes) {
  if (!audioHas(name)) return false;
  audioEnsureTask();
  strlcpy(aBgName, name, sizeof(aBgName));
  aBgMinutes = minutes;
  aBgReq = true;
  return true;
}
static void audioBgStopNow() {
  if (aTask) aBgStop = true;
}
static const char *audioBgPlaying() { return aBgOn ? aBgPlaying : nullptr; }
static uint32_t audioBgLeftSec() {
  if (!aBgOn || !aBgEndsAt) return 0;
  int32_t ms = (int32_t)(aBgEndsAt - millis());
  return ms > 0 ? (uint32_t)ms / 1000 : 0;
}

// ---------- the audio task ----------
static bool audioDriverUp(bool up) {
  if (up) {
    i2s_config_t c = {};
    c.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN);
    c.sample_rate = A_RATE;
    c.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    c.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    c.communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_MSB;
    c.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    c.dma_buf_count = 4;  // 4 x 256 frames, 4 KB of DMA RAM (was 12 KB: too much next to the radio)
    c.dma_buf_len = A_BLOCK / 2;
    // APLL: in built-in DAC mode the ordinary clock divider ran ~5.5x too fast (88.6 kHz for 16 kHz: sounds
    // played chipmunk-fast, the radio ring drained in no time); the audio PLL gives the asked rate exactly
    c.use_apll = true;
    c.tx_desc_auto_clear = true;
    ledcWrite(SPK_CH, 0);
    ledcDetachPin(PIN_SPK);  // the beeper lets go of the pin; the DAC takes it
    if (i2s_driver_install(I2S_NUM_0, &c, 0, nullptr) != ESP_OK) {
      ledcAttachPin(PIN_SPK, SPK_CH);
      return false;
    }
    Serial.printf("AUDIO up (free heap %u)\n", (unsigned)ESP.getFreeHeap());
    i2s_set_pin(I2S_NUM_0, nullptr);                    // built-in DAC pins
    i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN);          // left = DAC2 = GPIO26, the speaker amplifier
    i2s_zero_dma_buffer(I2S_NUM_0);
  } else {
    i2s_driver_uninstall(I2S_NUM_0);
    Serial.println("AUDIO down");
    // Hand the pin back to the digital side: left in DAC / RTC mode it floats, and the amplifier hums and whines.
    i2s_set_dac_mode(I2S_DAC_CHANNEL_DISABLE);
    dac_output_disable(DAC_CHANNEL_2);
    rtc_gpio_deinit((gpio_num_t)PIN_SPK);
    pinMode(PIN_SPK, OUTPUT);
    digitalWrite(PIN_SPK, LOW);
    ledcAttachPin(PIN_SPK, SPK_CH);
    ledcWrite(SPK_CH, 0);
  }
  return true;
}

// Around the DC level: linear up to +-96, then compressed 4:1 up to the 8-bit DAC's limit, so a loud sound
// flattens a little instead of cracking.
static inline int softClip(int s) {
  if (s > 96) return 96 + (s - 96) / 4;
  if (s < -96) return -96 + (s + 96) / 4;
  return s;
}

static void audioTask(void *) {
  static AChan fx, bg;
  static int16_t blk[A_BLOCK * 2];
  bool up = false;
  uint32_t idleSince = millis();
  int dc = 0;       // DC level of the output: ramps 0 -> 128 after start-up and back before shut-down (no thump)
  int bgFade = 256;  // 256 = full volume, falls to 0 at the end of the sleep timer
  for (;;) {
    if (aFxStop) {
      aFxStop = false;
      chClose(fx);
    }
    if (aBgStop) {
      aBgStop = false;
      chClose(bg);
    }
    if (aFxReq) {
      aFxReq = false;
      if (!chOpen(fx, aFxName, aFxLoop)) Serial.printf("AUDIO cannot play /sounds/%s.wav\n", aFxName);
    }
    if (aBgReq) {
      aBgReq = false;
      if (!chOpen(bg, aBgName, true)) Serial.printf("AUDIO cannot play /sounds/%s.wav\n", aBgName);
      else {
        Serial.printf("AUDIO background %s, %u min (rate step %u, %u-bit x%u)\n", aBgName, (unsigned)aBgMinutes, (unsigned)bg.step, bg.bits, bg.chans);
        strlcpy(aBgPlaying, aBgName, sizeof(aBgPlaying));
        aBgEndsAt = aBgMinutes ? millis() + aBgMinutes * 60000UL : 0;
        bgFade = 256;
      }
    }
    aFxOn = fx.on;
    aBgOn = bg.on;
    bool radio = aRadioOn;
    if (!radio) aRadioPlaying = false;
    if (!fx.on && !bg.on && !radio) {
      if (up && millis() - idleSince > 1500) {
        for (int i = 0; i < A_BLOCK; i++) {  // ramp the DC level down, then release the pin
          dc = dc > 0 ? dc - 1 : 0;
          blk[i * 2] = blk[i * 2 + 1] = (int16_t)(dc << 8);
        }
        size_t w;
        i2s_write(I2S_NUM_0, blk, sizeof(blk), &w, portMAX_DELAY);
        audioDriverUp(false);
        up = false;
        dc = 0;
      }
      if (!up) vTaskDelay(pdMS_TO_TICKS(10));
      else {  // idle but still up: keep feeding the mid level
        for (int i = 0; i < A_BLOCK; i++) {
          if (dc < 128 && (i & 3) == 0) dc++;
          blk[i * 2] = blk[i * 2 + 1] = (int16_t)(dc << 8);
        }
        size_t w;
        i2s_write(I2S_NUM_0, blk, sizeof(blk), &w, portMAX_DELAY);
      }
      continue;
    }
    if (!up) {
      if (!audioDriverUp(true)) {
        chClose(fx);
        chClose(bg);
        if (aRadioOn) {  // tell the radio (it shows "out of memory") instead of retrying for ever
          aRadioOn = false;
          aRadioNoDriver = true;
        }
        continue;
      }
      up = true;
      dc = 0;
    }
    idleSince = millis();
    if (bg.on && aBgEndsAt) {  // sleep timer: fade the background out over 6 s, then stop it
      int32_t left = (int32_t)(aBgEndsAt - millis());
      if (left <= 0) {
        bgFade -= 4;
        if (bgFade <= 0) {
          chClose(bg);
          aBgEndsAt = 0;
          bgFade = 256;
        }
      }
    }
    int vol = cfg.volume * 410 / 100;  // 100 % = 1.6x: the files leave headroom, and softClip() rounds off the peaks
    int gb = vol * bgFade / 256 * 3 / 4, gf = vol;  // background a little under the effects
    if (radio) {
      uint32_t avail = aRadioHead - aRadioTail;
      if (!aRadioPlaying && avail >= aRadioSize / 2) aRadioPlaying = true;
      else if (aRadioPlaying && avail < A_BLOCK) aRadioPlaying = false;  // ran dry: wait for it to fill again
    }
    const uint32_t *rb = aRadioPlaying ? aRadioBuf : nullptr;
    uint32_t rt = aRadioTail, rm = aRadioSize - 1;
    for (int i = 0; i < A_BLOCK; i++) {
      int s = 0;
      if (rb) {
        int v = (rb[(rt & rm) >> 2] >> ((rt & 3) * 8)) & 255;
        rt++;
        s += (v - 128) * vol >> 8;
      }
      if (bg.on) s += (chNext(bg) - 128) * gb >> 8;
      if (fx.on) s += (chNext(fx) - 128) * gf >> 8;
      if (dc < 128 && (i & 3) == 0) dc++;
      s = softClip(s) + dc;
      s = s < 0 ? 0 : s > 255 ? 255 : s;
      blk[i * 2] = blk[i * 2 + 1] = (int16_t)(s << 8);
    }
    if (rb) aRadioTail = rt;
    size_t w;
    i2s_write(I2S_NUM_0, blk, sizeof(blk), &w, portMAX_DELAY);  // blocks until the DMA has room: that is the pacing
  }
}

static void audioEnsureTask() {
  if (!aTask) xTaskCreatePinnedToCore(audioTask, "audio", 6144, nullptr, 2, &aTask, 0);
}
