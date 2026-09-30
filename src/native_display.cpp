#include "native_display.h"

#include <M5Pm1.h>

namespace {
constexpr int kSdSclk = 15;
constexpr int kSdMiso = 14;
constexpr int kSdMosi = 13;
constexpr int kSdCs = 47;
bool sdReady = false;

// PWR_CFG auto-clears to 0 on every esptool download-mode entry (and on a
// PMIC-issued reset/shutdown), which turns off DCDC_EN — the display's 5V
// rail — until something reasserts it. The Reader app's own driver does
// this via freeink-sdk's applyBootPowerPolicy(), but that function also
// calls Wire.begin() on the PM1's bus, which would start a second, competing
// I2C controller here (M5Unified's In_I2C already owns the same SDA/SCL
// pins on this board). Reissue just the two register writes through
// M5.In_I2C instead, so there is only ever one I2C controller on that bus.
void reassertDisplayPowerRail() {
  using namespace freeink::m5pm1;
  const uint8_t current = M5.In_I2C.readRegister8(ADDR, REG_PWR_CFG, I2C_HZ);
  const uint8_t updated = static_cast<uint8_t>((current & ~LDO_EN) | CHG_EN | DCDC_EN | BOOST_EN);
  M5.In_I2C.writeRegister8(ADDR, REG_PWR_CFG, updated, I2C_HZ);
  M5.In_I2C.writeRegister8(ADDR, REG_NEO_CFG, 0x00, I2C_HZ);
}
}

bool beginNativeDisplay() {
  auto cfg = M5.config();
  cfg.clear_display = false;
  cfg.fallback_board = m5::board_t::board_M5PaperColor;
  M5.begin(cfg);
  reassertDisplayPowerRail();
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

