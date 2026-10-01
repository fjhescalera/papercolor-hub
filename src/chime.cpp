#include "chime.h"

#include <cstring>

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

// M5Unified's playWav() trusts the chunk sizes inside the file and reads
// past the buffer on a truncated or corrupt one, which can crash the boot.
// An SD card asset is untrusted input, so walk and bounds-check every chunk
// against the actual buffer length before ever handing it to playWav().
bool isValidWav(const uint8_t* buf, size_t len) {
  if (buf == nullptr || len < 44) return false;
  if (memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0) return false;

  size_t offset = 12;
  bool haveFmt = false;
  bool haveData = false;
  while (offset + 8 <= len) {
    uint32_t chunkSize;
    memcpy(&chunkSize, buf + offset + 4, 4);
    const size_t chunkStart = offset + 8;
    if (chunkSize > len - chunkStart) return false;  // chunk claims to run past the buffer
    if (memcmp(buf + offset, "fmt ", 4) == 0 && chunkSize >= 16) haveFmt = true;
    if (memcmp(buf + offset, "data", 4) == 0) haveData = true;
    const size_t advance = chunkSize + (chunkSize & 1);  // chunks are word-aligned
    if (advance > len - chunkStart) return false;
    offset = chunkStart + advance;
  }
  return haveFmt && haveData;
}
}  // namespace

void playBootChime() {
  // main.cpp only calls this on a genuine cold boot (its own "pendingWake"
  // flag, not esp_reset_reason(), gates the silent-resume path), so no
  // reset-reason check is needed here.
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
  const bool ok = buf != nullptr && f.read(buf, len) == len && isValidWav(buf, len);
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
