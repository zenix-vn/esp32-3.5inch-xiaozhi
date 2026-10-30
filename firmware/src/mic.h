#pragma once
// Microphone: an I2S digital MEMS mic (INMP441 / SPH0645 / MSM261 type) wired to IO21, IO22 and IO35.
// IO35 is input-only, so it is the data line (SD); IO21 and IO22 are the bit clock (SCK) and the word select (WS).
// Which of the two is which depends on the wiring, so it is a saved setting (Settings > Microphone > Swap SCK/WS).
//
// The speaker uses I2S0 in its built-in-DAC mode (audio.h); the microphone takes I2S1, so both can run at once.
// The driver is only installed while something listens (the test page now, voice chat later).
#include <driver/i2s.h>

static const int MIC_PIN_A = 21;
static const int MIC_PIN_B = 22;
static const int MIC_PIN_SD = 35;
static const int MIC_RATE = 16000;

static bool micSwap = false;  // false: SCK = IO22, WS = IO21; true: SCK = IO21, WS = IO22
static bool micUp = false;

static void micPrefLoad() {
  Preferences p;
  p.begin("ui", true);
  micSwap = p.getBool("micSw", false);
  p.end();
}

static void micPrefSave() {
  Preferences p;
  p.begin("ui", false);
  p.putBool("micSw", micSwap);
  p.end();
}

static bool micStart() {
  if (micUp) return true;
  i2s_config_t c = {};
  c.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  c.sample_rate = MIC_RATE;
  c.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;  // these mics send 24 valid bits inside a 32-bit slot
  c.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;   // L/R pin of the mic on GND = left slot
  c.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  c.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  c.dma_buf_count = 4;
  c.dma_buf_len = 256;
  c.use_apll = false;
  if (i2s_driver_install(I2S_NUM_1, &c, 0, nullptr) != ESP_OK) {
    Serial.println("MIC i2s_driver_install failed");
    return false;
  }
  i2s_pin_config_t pin = {};
  pin.mck_io_num = I2S_PIN_NO_CHANGE;
  pin.bck_io_num = micSwap ? MIC_PIN_A : MIC_PIN_B;
  pin.ws_io_num = micSwap ? MIC_PIN_B : MIC_PIN_A;
  pin.data_out_num = I2S_PIN_NO_CHANGE;
  pin.data_in_num = MIC_PIN_SD;
  if (i2s_set_pin(I2S_NUM_1, &pin) != ESP_OK) {
    i2s_driver_uninstall(I2S_NUM_1);
    Serial.println("MIC i2s_set_pin failed");
    return false;
  }
  i2s_zero_dma_buffer(I2S_NUM_1);
  micUp = true;
  Serial.printf("MIC up: SCK=IO%d WS=IO%d SD=IO%d\n", pin.bck_io_num, pin.ws_io_num, pin.data_in_num);
  return true;
}

static void micStop() {
  if (!micUp) return;
  i2s_driver_uninstall(I2S_NUM_1);
  micUp = false;
}

// Reads up to n 16-bit samples; waits at most waitMs (0 = only what the DMA already holds). Returns the count.
static int micRead(int16_t *out, int n, uint32_t waitMs = 0) {
  if (!micUp) return 0;
  int32_t raw[128];
  int done = 0;
  while (done < n) {
    int want = min(n - done, 128);
    size_t got = 0;
    if (i2s_read(I2S_NUM_1, raw, want * sizeof(int32_t), &got, pdMS_TO_TICKS(waitMs)) != ESP_OK || got == 0) break;
    int cnt = got / sizeof(int32_t);
    for (int i = 0; i < cnt; i++) {
      int32_t v = raw[i] >> 14;  // 24-bit sample -> 16-bit with 4x gain (the mics are quiet)
      out[done + i] = v > 32767 ? 32767 : v < -32768 ? -32768 : (int16_t)v;
    }
    done += cnt;
    if ((size_t)(cnt * sizeof(int32_t)) < (size_t)want * sizeof(int32_t)) break;
  }
  return done;
}

// Loudness of the last block, 0..32767 (RMS with the DC offset removed); -1 when no samples arrived.
static int micLevel(bool *flat = nullptr) {
  int16_t s[256];
  int n = micRead(s, 256, 0);
  if (n <= 0) return -1;
  int64_t sum = 0;
  int mn = 32767, mx = -32768;
  for (int i = 0; i < n; i++) {
    sum += s[i];
    mn = min(mn, (int)s[i]);
    mx = max(mx, (int)s[i]);
  }
  int mean = sum / n;
  int64_t sq = 0;
  for (int i = 0; i < n; i++) {
    int d = s[i] - mean;
    sq += (int64_t)d * d;
  }
  if (flat) *flat = (mn == mx);  // every sample identical: the data line is stuck, not a quiet room
  return (int)sqrt((double)(sq / n));
}
