#pragma once
// Radio: the VOV channels from vovlive.vn, streamed over the internet.
//
// The streams are HLS over HTTPS: a master playlist points at a chunk list, which names ~10 s segments of AAC-LC
// (ADTS, 256 kbit/s, with an ID3 tag in front). Every request must carry "Referer: https://vovlive.vn/" or the
// server answers 403, which is why this is a small HLS client of its own instead of a stream library.
// One task does it all: keeps one TLS connection open (keep-alive, so a new segment costs no handshake), decodes
// with the Helix AAC decoder (lib/helix_aac, without SBR to save ~50 KB), mixes the stereo 44.1/48 kHz output down
// to 16 kHz mono 8 bit and feeds the ring of audio.h, whose mixer plays it under the interface sounds.
// While the radio plays it holds netMutex(): the board has heap for one TLS session only (weather waits).
//
// That direct mode leaves the board with ~8 KB of heap while it plays, too little for Wi-Fi to buffer 256 kbit/s:
// the ring runs dry and the sound stutters. So there is a relay mode: tools/radio_relay.py on a PC re-encodes a
// channel to AAC-LC 16 kHz mono 40 kbit/s and serves it as one endless plain-HTTP ADTS stream (GET /vov1 ...).
// No TLS (~21 KB more heap) and a sixth of the data. The relay address ("ip:port", NVS "radio"/"relay") is set
// with POST /api/radio?relay=...; empty = direct mode.
//
// Included by main.cpp after audio.h, games.h and statusbar.h.
#include <ESP_SSLClient.h>
#include "net_lock.h"
extern "C" {
#include "aacdec.h"
}

struct RadioCh {
  const char *name, *vi, *en, *path, *slug;  // path on stream.vovlive.vn; slug = path on the relay
  uint32_t col;
};
static const RadioCh RADIO_CH[] = {
    {"VOV1", "Thời sự - Chính trị - Tổng hợp", "News and current affairs", "/vovlive/vov1vov5Vietnamese.sdp_aac/playlist.m3u8", "/vov1", 0xE0453A},
    {"VOV2", "Văn hóa - Đời sống - Khoa giáo", "Culture, life and education", "/vovlive/vov2.sdp_aac/playlist.m3u8", "/vov2", 0x2F8FD0},
    {"VOV3", "Âm nhạc - Thông tin - Giải trí", "Music and entertainment", "/vovlive/vov3.sdp_aac/playlist.m3u8", "/vov3", 0xC759D6},
    {"VOV5", "VOV24/7 - Đối ngoại (tiếng Anh)", "VOV24/7 - in English", "/vovlive/vov247vov5English.sdp_aac/playlist.m3u8", "/vov5", 0x1FA39A},
    {"VOV GT HN", "Giao thông Hà Nội", "Traffic, Hanoi", "/vovlive/vovGTHN.sdp_aac/playlist.m3u8", "/vovgthn", 0xE89A2A},
    {"VOV GT HCM", "Giao thông TP. Hồ Chí Minh", "Traffic, Ho Chi Minh City", "/vovlive/vovGTHCM.sdp_aac/playlist.m3u8", "/vovgthcm", 0xD06A3A},
};
static const int RADIO_N = sizeof(RADIO_CH) / sizeof(RADIO_CH[0]);
static const char *const RADIO_HOST = "stream.vovlive.vn";
static const char *const RADIO_REFERER = "https://vovlive.vn/";

enum { RS_OFF, RS_CONNECT, RS_BUFFER, RS_PLAY, RS_ERROR };
static volatile int radioState = RS_OFF;
static volatile int radioCh = -1;        // channel playing (or being started), -1 = none
static volatile int radioWant = -1;      // channel asked for by the UI, -1 = stop
static char radioErr[64] = "";
static TaskHandle_t radioTaskH = nullptr;
static char radioRelay[48] = "";  // "host:port" of tools/radio_relay.py, "" = direct HTTPS

static void radioRelayLoad() {
  static bool loaded = false;
  if (loaded) return;
  loaded = true;
  Preferences p;
  p.begin("radio", true);
  p.getString("relay", radioRelay, sizeof(radioRelay));
  p.end();
}
static void radioRelaySet(const String &v) {
  strlcpy(radioRelay, v.c_str(), sizeof(radioRelay));
  Preferences p;
  p.begin("radio", false);
  p.putString("relay", radioRelay);
  p.end();
}

// ---------- decoder and buffers ----------
// Allocated after the TLS handshake and freed before a new one: the handshake peaks at ~50 KB and the decoder, the
// ring and the frame buffers take ~32 KB more; both at once do not fit next to the rest of the firmware.
static HAACDecoder rAac = nullptr;
static int16_t *rPcm = nullptr;  // one decoded frame: 1024 samples x 2 channels
static uint8_t *rIn = nullptr;   // compressed input
static const int R_IN = 2048;  // one ADTS frame is at most 1536 bytes (stereo)
static uint32_t rPhase = 0;      // downsampler: output-rate phase accumulator
static int32_t rAcc = 0, rAccN = 0;
static uint32_t rWord = 0, rWordN = 0;  // samples packed so far into the next ring word

static void radioFree() {
  if (!aRadioBuf && !rIn && !rPcm && !rAac) return;
  aRadioOn = false;
  vTaskDelay(pdMS_TO_TICKS(120));  // the mixer may be in the middle of a block that reads the ring
  free(aRadioBuf);
  aRadioBuf = nullptr;
  free(rIn);
  rIn = nullptr;
  free(rPcm);
  rPcm = nullptr;
  if (rAac) AACFreeDecoder(rAac);
  rAac = nullptr;
}

static bool radioAlloc() {
  rAac = AACInitDecoder();  // biggest first (one 20.5 KB block), before the small ones split the free space
  rPcm = (int16_t *)malloc(AAC_MAX_NSAMPS * AAC_MAX_NCHANS * sizeof(int16_t));
  aRadioSize = 8192;  // 0.5 s at 16 kHz; IRAM when there is some (32-bit access only: see audio.h)
  aRadioBuf = (uint32_t *)heap_caps_malloc(aRadioSize, MALLOC_CAP_EXEC | MALLOC_CAP_32BIT);
  if (!aRadioBuf) aRadioBuf = (uint32_t *)malloc(aRadioSize);
  rIn = (uint8_t *)malloc(R_IN);
  bool ok = aRadioBuf && rIn && rPcm && rAac;
  Serial.printf("RADIO buffers %s %d%d%d%d (8-bit heap %u, largest %u; IRAM left %u)\n", ok ? "ok" : "FAILED", rAac != nullptr,
                rPcm != nullptr, aRadioBuf != nullptr, rIn != nullptr, (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
                (unsigned)(heap_caps_get_free_size(MALLOC_CAP_32BIT) - heap_caps_get_free_size(MALLOC_CAP_8BIT)));
  if (!ok) {
    radioFree();
    return false;
  }
  aRadioHead = aRadioTail = 0;
  rPhase = rAcc = rAccN = 0;
  rWord = rWordN = 0;
  aRadioOn = true;
  audioEnsureTask();
  return true;
}

// ---------- HTTP/1.1 over one kept-alive connection (TLS on port 443) ----------
struct RHttp {
  WiFiClient *tcp = nullptr;
  ESP_SSLClient *ssl = nullptr;  // BearSSL: a 16 KB receive buffer and a 512 B send buffer (mbedTLS: 2 x 16 KB)
  Client *c = nullptr;           // ssl, or tcp alone for plain HTTP (the relay)
  char host[64] = "";
  uint16_t port = 0;
  int32_t left = 0;       // body bytes left; -1 = until the server closes
  bool chunked = false, keep = true;
  int32_t chunkLeft = 0;  // chunked: bytes left in this chunk, -1 = body done
};
static RHttp rh;

static bool radioStopping() { return radioWant != radioCh || aRadioNoDriver; }
static volatile bool gNetHeavy = false;  // TLS handshake running: loop() leaves LVGL and the web server alone

static void rClose() {
  if (rh.c) rh.c->stop();
  delete rh.ssl;
  rh.ssl = nullptr;
  delete rh.tcp;
  rh.tcp = nullptr;
  rh.c = nullptr;
  rh.host[0] = 0;
}

// Read one header line (without CR LF); false on timeout.
static bool rLine(char *buf, int sz) {
  int n = 0;
  uint32_t t0 = millis();
  while (millis() - t0 < 8000) {
    int ch = rh.c->read();
    if (ch < 0) {
      if (!rh.c->connected()) return false;
      vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }
    if (ch == '\n') {
      if (n && buf[n - 1] == '\r') n--;
      buf[n] = 0;
      return true;
    }
    if (n < sz - 1) buf[n++] = ch;
  }
  return false;
}

// GET http(s)://<host>:<port><path> (TLS on 443). Returns the HTTP status (0 = connection failed); the body is then
// read with rRead().
static int rGet(const char *host, uint16_t port, const char *path) {
  for (int attempt = 0; attempt < 2; attempt++) {  // a kept-alive connection may have been closed by the server
    if (!rh.c || strcmp(rh.host, host) || rh.port != port || !rh.c->connected()) {
      rClose();
      bool tls = port == 443;
      if (tls) radioFree();  // room for the handshake (a short silence when it reconnects mid-stream)
      rh.tcp = new WiFiClient();
      rh.c = rh.tcp;
      if (tls) {
        rh.ssl = new ESP_SSLClient();
        rh.ssl->setClient(rh.tcp);
        rh.ssl->setInsecure();  // TODO: pin the server CA instead of skipping verification
        rh.ssl->setBufferSizes(16384, 512);  // a whole TLS record must fit when receiving; requests are short
        rh.ssl->setTimeout(8000);
        rh.c = rh.ssl;
      }
      gNetHeavy = tls;
      if (tls) vTaskDelay(pdMS_TO_TICKS(40));  // let loop() finish the pass it is in
      uint32_t t0 = millis();
      bool ok = rh.c->connect(host, port);
      gNetHeavy = false;
      if (!ok) {
        Serial.printf("RADIO connect %s failed (free heap %u, largest %u)\n", host, (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
        rClose();
        return 0;
      }
      strlcpy(rh.host, host, sizeof(rh.host));
      rh.port = port;
      Serial.printf("RADIO connected to %s in %u ms (8-bit heap %u, largest %u, stack never used %u)\n", host, (unsigned)(millis() - t0),
                    (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT), (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
                    (unsigned)uxTaskGetStackHighWaterMark(nullptr));
    }
    if (!aRadioBuf && !radioAlloc()) {
      snprintf(radioErr, sizeof(radioErr), "%s", DL("Không đủ bộ nhớ", "Out of memory"));
      return 0;
    }
    rh.c->printf("GET %s HTTP/1.1\r\nHost: %s\r\nReferer: %s\r\nUser-Agent: Zin100\r\nConnection: keep-alive\r\n\r\n", path, host, RADIO_REFERER);
    char line[160];
    if (!rLine(line, sizeof(line))) {
      rClose();
      continue;
    }
    int code = 0;
    sscanf(line, "HTTP/%*s %d", &code);
    rh.left = -1;
    rh.chunked = false;
    rh.keep = true;
    rh.chunkLeft = 0;
    while (rLine(line, sizeof(line)) && line[0]) {
      if (!strncasecmp(line, "Content-Length:", 15)) rh.left = atol(line + 15);
      else if (!strncasecmp(line, "Transfer-Encoding:", 18) && strcasestr(line, "chunked")) rh.chunked = true;
      else if (!strncasecmp(line, "Connection:", 11) && strcasestr(line, "close")) rh.keep = false;
    }
    return code;
  }
  return 0;
}

// Up to n body bytes (waits for at least one); 0 = end of the body, -1 = error.
static int rRead(uint8_t *buf, int n) {
  if (rh.chunked) {
    if (rh.chunkLeft < 0) return 0;
    if (rh.chunkLeft == 0) {
      char line[24];
      if (!rLine(line, sizeof(line))) return -1;
      if (!line[0] && !rLine(line, sizeof(line))) return -1;  // CR LF that ended the previous chunk
      rh.chunkLeft = strtol(line, nullptr, 16);
      if (rh.chunkLeft == 0) {
        rLine(line, sizeof(line));  // empty trailer
        rh.chunkLeft = -1;
        return 0;
      }
    }
    if (n > rh.chunkLeft) n = rh.chunkLeft;
  } else {
    if (rh.left == 0) return 0;
    if (rh.left > 0 && n > rh.left) n = rh.left;
  }
  uint32_t t0 = millis();
  for (;;) {
    int got = rh.c->read(buf, n);
    if (got > 0) {
      if (rh.chunked) rh.chunkLeft -= got;
      else if (rh.left > 0) rh.left -= got;
      return got;
    }
    if (!rh.c->connected()) return rh.left < 0 && !rh.chunked ? 0 : -1;
    if (millis() - t0 > 8000 || radioStopping()) return -1;
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// Rest of a response that is not needed; the connection stays usable when that succeeds.
static void rDrain() {
  uint8_t b[256];
  int n;
  while ((n = rRead(b, sizeof(b))) > 0) {
  }
  if (n < 0 || !rh.keep) rClose();
}

// Whole (small) body as text: playlists.
static bool rGetText(const char *host, const char *path, char *out, int sz) {
  int code = rGet(host, 443, path);
  if (code != 200) {
    if (code) {
      snprintf(radioErr, sizeof(radioErr), "HTTP %d", code);
      rDrain();
    } else if (!radioErr[0]) {  // rGet may already have said why (out of memory)
      snprintf(radioErr, sizeof(radioErr), "%s", DL("Không kết nối được máy chủ", "Cannot reach the server"));
    }
    return false;
  }
  int len = 0, n;
  while (len < sz - 1 && (n = rRead((uint8_t *)out + len, sz - 1 - len)) > 0) len += n;
  out[len] = 0;
  if (n != 0) rDrain();
  else if (!rh.keep) rClose();
  return len > 0;
}

// A playlist entry relative to the playlist's own path -> absolute path on the same host.
static void rResolve(const char *base, const char *ref, char *out, int sz) {
  if (ref[0] == '/') {
    strlcpy(out, ref, sz);
    return;
  }
  const char *slash = strrchr(base, '/');
  int dir = slash ? slash - base + 1 : 0;
  snprintf(out, sz, "%.*s%s", dir, base, ref);
}

// ---------- decoding ----------
static void rPush(const int16_t *pcm, int frames, int chans, int rate) {
  // box filter: average the input samples that fall into each 1/16000 s output slot (also the anti-alias filter)
  while (aRadioSize - (aRadioHead - aRadioTail) < (uint32_t)(frames * A_RATE / rate + 4)) {  // ring full: wait
    if (radioStopping()) return;
    vTaskDelay(pdMS_TO_TICKS(15));
  }
  uint32_t head = aRadioHead, m = aRadioSize - 1;
  for (int i = 0; i < frames; i++) {
    rAcc += chans == 2 ? (pcm[i * 2] + pcm[i * 2 + 1]) >> 1 : pcm[i];
    rAccN++;
    rPhase += A_RATE;
    if (rPhase >= (uint32_t)rate) {
      rPhase -= rate;
      // broadcast audio is mastered loud: at 60 % with a soft knee above +-96 the small speaker amplifier on the
      // board stops clipping (full scale distorted badly)
      int x = rAcc / rAccN * 154 >> 16;  // 16 bit -> 8 bit at 60 %
      if (x > 96) x = 96 + (x - 96) / 3;
      else if (x < -96) x = -96 + (x + 96) / 3;
      int v = x + 128;
      rWord |= (uint32_t)(v < 0 ? 0 : v > 255 ? 255 : v) << (rWordN * 8);
      rAcc = rAccN = 0;
      if (++rWordN == 4) {  // a whole word: store it and let the mixer have it
        aRadioBuf[(head & m) >> 2] = rWord;
        head += 4;
        rWord = rWordN = 0;
      }
    }
  }
  aRadioHead = head;
}

// Fetch one segment and decode it into the ring. false = network error.
static bool rPlaySegment(const char *host, uint16_t port, const char *path) {
  int code = rGet(host, port, path);
  if (code != 200) {
    Serial.printf("RADIO segment HTTP %d\n", code);
    if (code) rDrain();
    return false;
  }
  int len = 0, n;
  bool eof = false;
  // the ID3 tag in front of the audio: 10-byte header + syncsafe size
  while (len < 10 && (n = rRead(rIn + len, 10 - len)) > 0) len += n;
  if (len == 10 && !memcmp(rIn, "ID3", 3)) {
    int32_t skip = ((rIn[6] & 127) << 21 | (rIn[7] & 127) << 14 | (rIn[8] & 127) << 7 | (rIn[9] & 127)) + ((rIn[5] & 16) ? 10 : 0);
    len = 0;
    while (skip > 0 && (n = rRead(rIn, skip < R_IN ? skip : R_IN)) > 0) skip -= n;
  }
  int errors = 0;
  while (!radioStopping()) {
    while (!eof && len < R_IN) {  // keep the input full: the decoder needs a whole frame
      n = rRead(rIn + len, R_IN - len);
      if (n < 0) {
        rClose();
        return false;
      }
      if (n == 0) eof = true;
      len += n;
    }
    if (len < 8) break;
    int off = AACFindSyncWord(rIn, len);
    if (off < 0) {  // no frame start in here: keep the last byte (it may be half of a sync word)
      rIn[0] = rIn[len - 1];
      len = 1;
      if (eof) break;
      continue;
    }
    if (off) {
      memmove(rIn, rIn + off, len - off);
      len -= off;
      continue;
    }
    uint8_t *p = rIn;
    int left = len;
    int err = AACDecode(rAac, &p, &left, rPcm);
    if (err == ERR_AAC_INDATA_UNDERFLOW) {
      if (eof) break;
      continue;
    }
    if (err) {  // a broken frame: step over its sync word and look for the next one
      if (++errors > 50) {
        snprintf(radioErr, sizeof(radioErr), "AAC error %d", err);
        rDrain();
        return false;
      }
      memmove(rIn, rIn + 1, len - 1);
      len--;
      continue;
    }
    AACFrameInfo fi;
    AACGetLastFrameInfo(rAac, &fi);
    if (fi.nChans >= 1 && fi.sampRateOut >= 8000) rPush(rPcm, fi.outputSamps / fi.nChans, fi.nChans, fi.sampRateOut);
    memmove(rIn, p, left);
    len = left;
  }
  if (!eof) rDrain();
  else if (!rh.keep) rClose();
  return true;
}

// Relay mode: one endless stream; reconnects when it ends.
static bool rPlayRelay(int ch) {
  char host[48];
  strlcpy(host, radioRelay, sizeof(host));
  char *colon = strchr(host, ':');
  uint16_t port = colon ? atoi(colon + 1) : 8000;
  if (colon) *colon = 0;
  radioState = RS_BUFFER;
  int fails = 0;
  while (!radioStopping()) {
    if (rPlaySegment(host, port, RADIO_CH[ch].slug)) {
      fails = 0;  // the relay closed the stream (ffmpeg restarted): open it again
    } else if (++fails > 3) {
      if (!radioErr[0]) snprintf(radioErr, sizeof(radioErr), isVi() ? "Không kết nối được máy trung gian %s" : "Cannot reach the relay %s", radioRelay);
      return false;
    } else {
      for (int i = 0; i < 20 && !radioStopping(); i++) vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
  return true;
}

// Plays one channel until it is changed or stopped. false = it failed (radioErr says why).
static bool rPlayChannel(int ch) {
  if (radioRelay[0]) return rPlayRelay(ch);
  static char text[1024];
  static char media[160], seg[200];
  static char segs[8][96];
  const char *master = RADIO_CH[ch].path;
  radioState = RS_CONNECT;
  if (!rGetText(RADIO_HOST, master, text, sizeof(text))) return false;
  // master playlist -> the chunk list (the first variant; VOV has just one)
  media[0] = 0;
  bool variant = false;
  for (char *ln = strtok(text, "\r\n"); ln; ln = strtok(nullptr, "\r\n")) {
    if (!strncmp(ln, "#EXT-X-STREAM-INF", 17)) variant = true;
    else if (variant && ln[0] != '#') {
      rResolve(master, ln, media, sizeof(media));
      break;
    }
  }
  if (!media[0]) strlcpy(media, master, sizeof(media));
  radioState = RS_BUFFER;
  uint32_t next = 0;  // media sequence number of the next segment to play
  int fails = 0;
  while (!radioStopping()) {
    if (!rGetText(RADIO_HOST, media, text, sizeof(text))) {
      if (++fails > 3) return false;
      vTaskDelay(pdMS_TO_TICKS(1500));
      continue;
    }
    uint32_t seq0 = 0;
    int nseg = 0;
    for (char *ln = strtok(text, "\r\n"); ln; ln = strtok(nullptr, "\r\n")) {
      if (!strncmp(ln, "#EXT-X-MEDIA-SEQUENCE:", 22)) seq0 = strtoul(ln + 22, nullptr, 10);
      else if (ln[0] != '#' && ln[0] && nseg < 8) strlcpy(segs[nseg++], ln, sizeof(segs[0]));
    }
    if (!nseg) {
      snprintf(radioErr, sizeof(radioErr), "%s", DL("Danh sách phát trống", "Empty playlist"));
      return false;
    }
    if (!next) next = seq0 + (nseg > 2 ? nseg - 2 : 0);  // start two segments behind live: room to catch up
    if (next < seq0) next = seq0;                         // fell behind the window
    bool played = false;
    for (int i = 0; i < nseg && !radioStopping(); i++) {
      if (seq0 + i < next) continue;
      rResolve(media, segs[i], seg, sizeof(seg));
      if (!rPlaySegment(RADIO_HOST, 443, seg)) {
        if (++fails > 3) {
          if (!radioErr[0]) snprintf(radioErr, sizeof(radioErr), "%s", DL("Mất kết nối", "Connection lost"));
          return false;
        }
        break;  // reload the chunk list and go on from the same segment
      }
      fails = 0;
      next = seq0 + i + 1;
      played = true;
    }
    if (!played) {  // nothing new yet: ask again in a moment
      for (int i = 0; i < 20 && !radioStopping(); i++) vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
  return true;
}

static void radioTask(void *) {
  bool direct = !radioRelay[0];
  if (direct) xSemaphoreTake(netMutex(), portMAX_DELAY);  // TLS: after any other TLS request in progress
  for (int i = 0; i < 100 && buf2; i++) vTaskDelay(pdMS_TO_TICKS(10));  // loop() hands over LVGL's second draw buffer
  while (radioWant >= 0) {
    int ch = radioCh = radioWant;
    radioErr[0] = 0;
    aRadioNoDriver = false;
    radioState = RS_CONNECT;
    Serial.printf("RADIO %s (free heap %u, largest %u)\n", RADIO_CH[ch].name, (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
    bool ok = wifiConnected();
    if (!ok) snprintf(radioErr, sizeof(radioErr), "%s", DL("Chưa kết nối Wi-Fi", "No Wi-Fi"));
    if (ok) {
      if (aRadioBuf) {  // switching channels: drop what the old one left in the ring
        aRadioOn = false;
        vTaskDelay(pdMS_TO_TICKS(60));
        aRadioHead = aRadioTail = 0;
        rWord = rWordN = 0;
        AACFlushCodec(rAac);
        aRadioOn = true;
      }
      ok = rPlayChannel(ch);
      if (aRadioNoDriver) {  // the speaker driver found no heap (audio.h)
        ok = false;
        snprintf(radioErr, sizeof(radioErr), "%s", DL("Không đủ bộ nhớ", "Out of memory"));
      }
    }
    if (!ok && radioWant == ch) {  // failed: show it and wait for the user
      Serial.printf("RADIO error: %s\n", radioErr);
      radioState = RS_ERROR;
      rClose();
      radioFree();
      while (radioWant == ch) vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
  rClose();
  radioFree();
  radioCh = -1;
  radioState = RS_OFF;
  if (direct) xSemaphoreGive(netMutex());
  Serial.printf("RADIO off (free heap %u, stack never used %u)\n", (unsigned)ESP.getFreeHeap(), (unsigned)uxTaskGetStackHighWaterMark(nullptr));
  radioTaskH = nullptr;
  vTaskDelete(nullptr);
}

static void radioPlay(int ch) {
  if (ch < 0 || ch >= RADIO_N) return;
  radioRelayLoad();
  if (radioWant == ch && radioState != RS_ERROR) return;
  if (radioState == RS_ERROR && radioWant == ch) {  // retry the same channel: go through "stop"
    radioWant = -1;
    while (radioTaskH) vTaskDelay(pdMS_TO_TICKS(20));
  }
  radioWant = ch;
  // Core 1 next to loop() (same priority, time-sliced): the BearSSL handshake is seconds of big-number maths that
  // starves IDLE0 into the task watchdog on core 0, even at priority 0. Core 1's idle task is not watched.
  // 7 KB: the BearSSL handshake needs ~5 KB. Priority 2, above loop(): at the same priority the screen took the CPU
  // the decoder needs and the ring kept running dry (choppy, distorted sound); the decoder sleeps whenever the
  // ring is full or the network is slow, so loop() still gets most of core 1.
  if (!radioTaskH) xTaskCreatePinnedToCore(radioTask, "radio", 7168, nullptr, 2, &radioTaskH, 1);
}
static void radioStop() { radioWant = -1; }
static bool radioActive() { return radioWant >= 0; }

static const char *radioStateText() {
  switch (radioState) {
    case RS_CONNECT: return DL("Đang kết nối...", "Connecting...");
    case RS_BUFFER: return DL("Đang tải...", "Buffering...");
    case RS_PLAY: return DL("Đang phát", "Playing");
    case RS_ERROR: return radioErr[0] ? radioErr : DL("Lỗi", "Error");
    default: return "";
  }
}

// ---------- the page ----------
static lv_obj_t *rdRow[RADIO_N], *rdNowName, *rdNowState, *rdNowBtn, *rdBars[5];

static void radioRefresh(lv_timer_t * = nullptr) {
  if (radioState == RS_BUFFER && aRadioPlaying) radioState = RS_PLAY;
  else if (radioState == RS_PLAY && !aRadioPlaying) radioState = RS_BUFFER;
  int cur = radioWant;
  for (int i = 0; i < RADIO_N; i++) {
    bool on = i == cur;
    lv_obj_set_style_bg_opa(rdRow[i], on ? LV_OPA_50 : LV_OPA_20, 0);
    lv_obj_set_style_border_width(rdRow[i], on ? 2 : 1, 0);
    lv_obj_set_style_border_color(rdRow[i], on ? C_TEXT : lv_color_hex(RADIO_CH[i].col), 0);
  }
  if (cur >= 0) {
    setText(rdNowName, RADIO_CH[cur].name);
    setText(rdNowState, radioStateText());
    lv_obj_set_style_text_color(rdNowState, radioState == RS_ERROR ? C_DANGER : radioState == RS_PLAY ? C_OK : C_MUTED, 0);
    setText(lv_obj_get_child(rdNowBtn, 0), LV_SYMBOL_STOP);
  } else {
    setText(rdNowName, DL("Chọn một kênh", "Pick a channel"));
    if (radioRelay[0]) setTextFmt(rdNowState, isVi() ? "Qua máy trung gian %s" : "Through the relay %s", radioRelay);
    else setText(rdNowState, DL("Phát qua Wi-Fi từ vovlive.vn", "Streamed over Wi-Fi from vovlive.vn"));
    lv_obj_set_style_text_color(rdNowState, C_MUTED, 0);
    setText(lv_obj_get_child(rdNowBtn, 0), LV_SYMBOL_PLAY);
  }
  // little level bars while sound comes out
  static uint8_t t = 0;
  t++;
  for (int i = 0; i < 5; i++) {
    int h = radioState == RS_PLAY ? 6 + (int)((esp_random() >> (i * 3)) % 22) : 4;
    lv_obj_set_height(rdBars[i], h);
    lv_obj_set_y(rdBars[i], 34 - h);
  }
}

static void pageRadio() {
  radioRelayLoad();
  lv_obj_t *c;
  lv_obj_t *s = pageBegin("Radio VOV", &c);
  lv_obj_del(c);

  // now playing
  lv_obj_t *now = makeCard(s, 456, 58);
  lv_obj_set_pos(now, 12, SB_H + 50);
  lv_obj_t *bars = lv_obj_create(now);
  lv_obj_remove_style_all(bars);
  lv_obj_set_size(bars, 40, 36);
  lv_obj_align(bars, LV_ALIGN_LEFT_MID, 12, 0);
  for (int i = 0; i < 5; i++) rdBars[i] = shape(bars, i * 8, 30, 5, 4, 0x38E1FF, 2);
  rdNowName = makeLabel(now, "", F_LABEL, C_TEXT);
  lv_obj_set_pos(rdNowName, 62, 6);
  rdNowState = makeLabel(now, "", F_CAPTION, C_MUTED);
  lv_obj_set_width(rdNowState, 300);
  lv_label_set_long_mode(rdNowState, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(rdNowState, 62, 30);
  rdNowBtn = smallBtn(now, LV_SYMBOL_PLAY, C_ACCENT, [](lv_event_t *) {
    if (radioActive()) radioStop();
    else radioPlay(0);
    radioRefresh();
  }, nullptr, 52);
  lv_obj_align(rdNowBtn, LV_ALIGN_RIGHT_MID, -10, 0);

  // channels: 2 x 3 cards
  for (int i = 0; i < RADIO_N; i++) {
    uint32_t col = RADIO_CH[i].col;
    lv_obj_t *card = lv_obj_create(s);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, 224, 56);
    lv_obj_set_pos(card, 12 + (i % 2) * 232, SB_H + 116 + (i / 2) * 62);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(col), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_60, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(card, lv_color_hex(col), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *nm = makeLabel(card, RADIO_CH[i].name, F_LABEL, C_TEXT);
    lv_obj_set_pos(nm, 12, 6);
    lv_obj_t *sub = makeLabel(card, isVi() ? RADIO_CH[i].vi : RADIO_CH[i].en, F_CAPTION, lv_color_hex(0xD9DEF5));
    lv_obj_set_width(sub, 204);
    lv_label_set_long_mode(sub, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(sub, 12, 31);
    lv_obj_add_event_cb(card, [](lv_event_t *e) {
      int i = (int)(intptr_t)lv_event_get_user_data(e);
      if (radioWant == i && radioState != RS_ERROR) radioStop();
      else {
        if (cfg.volume == 0) uiToast(DL("Âm lượng đang tắt: kéo thanh âm lượng ở Trung tâm điều khiển", "Volume is off: raise it in the control center"), C_WARN);
        radioPlay(i);
      }
      radioRefresh();
    }, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    rdRow[i] = card;
  }
  radioRefresh();
  pageTimer(s, radioRefresh, 250);
  pageShow(s);
}
