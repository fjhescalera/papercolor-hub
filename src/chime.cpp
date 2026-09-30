#include "chime.h"

#include <esp_system.h>

#include "native_display.h"

namespace {
constexpr const char* kChimePath = "/chimes/ding.wav";

uint8_t* psAlloc(size_t n) {
  uint8_t* p = static_cast<uint8_t*>(ps_malloc(n));
  return p != nullptr ? p : static_cast<uint8_t*>(malloc(n));
}

void beep(uint16_t freq, uint32_t ms) {
  M5.Speaker.tone(freq, ms);
  while (M5.Speaker.isPlaying()) delay(5);
}

void playFallbackBootBeep() {
  beep(880, 80);
  beep(1320, 120);
}
}  // namespace

void playBootChime() {
  if (esp_reset_reason() == ESP_RST_DEEPSLEEP) return;  // silent resume, not a fresh boot

  if (!beginSharedSd()) {
    playFallbackBootBeep();
    return;
  }
  File f = SD.open(kChimePath, FILE_READ);
  if (!f) {
    playFallbackBootBeep();
    return;
  }
  const size_t len = f.size();
  uint8_t* buf = psAlloc(len);
  const bool ok = buf != nullptr && f.read(buf, len) == len;
  f.close();
  if (!ok) {
    if (buf != nullptr) free(buf);
    playFallbackBootBeep();
    return;
  }
  // ponytail: buf is intentionally never freed — played once per boot,
  // a few hundred KB against 8MB PSRAM; add tracked free() if this ever
  // needs to play more than once per runtime.
  M5.Speaker.playWav(buf, len);
}

void playShutdownChime() {
  beep(660, 120);
  beep(440, 160);
}
