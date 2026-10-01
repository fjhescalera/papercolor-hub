#include "native_display.h"

#include <M5Pm1.h>

namespace {
constexpr int kSdSclk = 15;
constexpr int kSdMiso = 14;
constexpr int kSdMosi = 13;
constexpr int kSdCs = 47;
bool sdReady = false;
}

bool beginNativeDisplay() {
  // PWR_CFG auto-clears to 0 on every esptool download-mode entry (and on a
  // PMIC-issued reset/shutdown), which turns off DCDC_EN — the display's
  // 5V rail. M5.begin() probes and initializes the EPD panel as part of
  // its own startup, so the rail must already be back on *before* that
  // call: reasserting it afterward (as an earlier version of this function
  // did, via M5.In_I2C) was too late — the panel could be probed/reset
  // while unpowered and nothing re-runs that init once power returns,
  // leaving the screen blank for the rest of the boot. Use the SDK's own
  // raw-Wire helpers here, since M5Unified's In_I2C is not configured yet
  // at this point in boot — this is a one-shot, sequential use of the bus
  // before M5Unified claims it, not a second concurrent owner.
  freeink::m5pm1::beginBus();
  freeink::m5pm1::applyBootPowerPolicy();
  delay(50);  // let the 5V rail settle before the panel is probed

  auto cfg = M5.config();
  cfg.clear_display = false;
  cfg.fallback_board = m5::board_t::board_M5PaperColor;
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.setEpdMode(epd_mode_t::epd_quality);
  M5.Display.fillScreen(WHITE);
  M5.Display.setTextColor(BLACK, WHITE);
  return true;
}

bool beginSharedSd() {
  if (sdReady) return true;
  SPI.begin(kSdSclk, kSdMiso, kSdMosi, kSdCs);
  sdReady = SD.begin(kSdCs, SPI, 20000000);
  return sdReady;
}

void nativeHeader(const char* title, uint32_t accent) {
  M5.Display.fillRect(0, 0, 600, 54, accent);
  M5.Display.setTextColor(WHITE, accent);
  M5.Display.setTextDatum(middle_left);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString(title, 18, 28);
  M5.Display.setTextColor(BLACK, WHITE);
}

void nativeFooter(const char* text) {
  M5.Display.drawFastHLine(16, 370, 568, BLACK);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_center);
  M5.Display.drawString(text, 300, 384);
}

