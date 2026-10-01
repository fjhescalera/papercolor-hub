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

struct WavPcm {
  const uint8_t* data = nullptr;
  size_t len = 0;
  uint32_t sampleRate = 0;
  uint16_t channels = 0;
  uint16_t bitsPerSample = 0;
};

// M5Unified's own playWav() trusts the file's RIFF size as its chunk-walk
// bound instead of the actual buffer length, and does not account for
// RIFF padding when advancing between chunks, so a crafted or merely
// corrupt file can make it read past the buffer. An SD card asset is
// untrusted input, so parse it ourselves with every chunk bounds-checked
// against the real buffer length, and hand only the verified, in-bounds
// PCM data to playRaw() — never the raw file to playWav().
bool parseWavPcm(const uint8_t* buf, size_t len, WavPcm* out) {
  if (buf == nullptr || len < 44) return false;
  if (memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0) return false;

  size_t offset = 12;
  bool haveFmt = false;
  uint16_t audioFormat = 0;
  while (offset + 8 <= len) {
    uint32_t chunkSize;
    memcpy(&chunkSize, buf + offset + 4, 4);
    const size_t chunkStart = offset + 8;
    if (chunkSize > len - chunkStart) return false;  // chunk claims to run past the buffer

    if (!haveFmt && memcmp(buf + offset, "fmt ", 4) == 0 && chunkSize >= 16) {
      memcpy(&audioFormat, buf + chunkStart, 2);
      memcpy(&out->channels, buf + chunkStart + 2, 2);
      memcpy(&out->sampleRate, buf + chunkStart + 4, 4);
      memcpy(&out->bitsPerSample, buf + chunkStart + 14, 2);
      haveFmt = true;
    } else if (haveFmt && memcmp(buf + offset, "data", 4) == 0) {
      out->data = buf + chunkStart;
      out->len = chunkSize;
      return audioFormat == 1 &&  // PCM
             (out->channels == 1 || out->channels == 2) &&
             (out->bitsPerSample == 8 || out->bitsPerSample == 16);
    }
    const size_t advance = chunkSize + (chunkSize & 1);  // chunks are word-aligned
    if (advance > len - chunkStart) return false;
    offset = chunkStart + advance;
  }
  return false;  // ran off the end without finding fmt then data, in that order
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
  WavPcm pcm;
  const bool ok = buf != nullptr && f.read(buf, len) == len && parseWavPcm(buf, len, &pcm);
  f.close();
  if (!ok) {
    if (buf != nullptr) free(buf);
    playFallbackBootBeep();
    return;
  }
  // ponytail: buf is intentionally never freed (pcm.data points inside it)
  // — played once per boot, a few hundred KB against 8MB PSRAM; add tracked
  // free() if this ever needs to play more than once per runtime.
  const bool stereo = pcm.channels == 2;
  if (pcm.bitsPerSample == 16) {
    M5.Speaker.playRaw(reinterpret_cast<const int16_t*>(pcm.data), pcm.len / 2, pcm.sampleRate, stereo);
  } else {
    M5.Speaker.playRaw(pcm.data, pcm.len, pcm.sampleRate, stereo);
  }
}

void playShutdownChime() {
  beep(660, 120);
  beep(440, 160);
}
