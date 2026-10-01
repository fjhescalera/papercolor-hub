#include "led_status.h"

#include <Arduino.h>
#include <M5Pm1.h>
#include <M5Unified.h>
#include <esp_cpu.h>

// M5Stack PaperColor: two WS2812/SK6812-compatible GRB LEDs on GPIO21,
// powered through the M5PM1's RGB LED rail (PWR_CFG's LDO_EN bit — the same
// values freeink-sdk's LedManager/BoardConfig use). Reimplemented here
// instead of depending on LedManager: that library's power management calls
// freeink::m5pm1::setRgbRail(), which does its own raw Wire.begin() on the
// PMIC bus every time it is used. Called after beginNativeDisplay() (as the
// boot/shutdown call sites here do), that collides with M5Unified's own
// In_I2C, already active on the same physical pins — a second, unsynchronized
// I2C controller with no handoff between them. Routing the one needed
// register touch through M5.In_I2C instead avoids that entirely; the WS2812
// bit-bang timing below has no I2C in it at all, so it carries no such risk.
namespace {
constexpr uint8_t kLedPin = 21;
constexpr uint8_t kLedCount = 2;
constexpr uint16_t kT0hNs = 300;
constexpr uint16_t kT1hNs = 900;
constexpr uint16_t kBitNs = 1200;

uint32_t nsToCycles(uint32_t ns) {
  const uint32_t mhz = ESP.getCpuFreqMHz();
  return (mhz * ns + 999) / 1000;
}

inline void IRAM_ATTR waitUntilCycle(uint32_t target) {
  while (static_cast<int32_t>(esp_cpu_get_cycle_count() - target) < 0) {
  }
}

void IRAM_ATTR sendFrame(const uint8_t* bytes, uint32_t len, uint32_t t0h, uint32_t t1h, uint32_t bit) {
  for (uint32_t i = 0; i < len; ++i) {
    const uint8_t value = bytes[i];
    for (uint8_t m = 0x80; m != 0; m >>= 1) {
      const uint32_t start = esp_cpu_get_cycle_count();
      digitalWrite(kLedPin, HIGH);
      waitUntilCycle(start + ((value & m) ? t1h : t0h));
      digitalWrite(kLedPin, LOW);
      waitUntilCycle(start + bit);
    }
  }
}

void setLedRail(bool on) {
  using namespace freeink::m5pm1;
  const uint8_t current = M5.In_I2C.readRegister8(ADDR, REG_PWR_CFG, I2C_HZ);
  const uint8_t updated = static_cast<uint8_t>(on ? (current | LDO_EN) : (current & ~LDO_EN));
  M5.In_I2C.writeRegister8(ADDR, REG_PWR_CFG, updated, I2C_HZ);
}

void showColor(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t bytes[kLedCount * 3];
  for (uint8_t i = 0; i < kLedCount; ++i) {
    bytes[i * 3 + 0] = g;  // GRB order, bench-verified by freeink-sdk's BoardConfig
    bytes[i * 3 + 1] = r;
    bytes[i * 3 + 2] = b;
  }
  const uint32_t t0h = nsToCycles(kT0hNs);
  const uint32_t t1h = nsToCycles(kT1hNs);
  const uint32_t bit = nsToCycles(kBitNs);
  noInterrupts();
  sendFrame(bytes, sizeof(bytes), t0h, t1h, bit);
  interrupts();
  delayMicroseconds(280);  // latch
}

void flashTwice(uint8_t r, uint8_t g, uint8_t b) {
  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, LOW);
  setLedRail(true);
  delay(10);  // rail ramp + LED power-on reset before the first frame
  for (uint8_t i = 0; i < 2; ++i) {
    showColor(r, g, b);
    delay(150);
    showColor(0, 0, 0);
    delay(150);
  }
  setLedRail(false);
}
}  // namespace

void ledStatusFlashBoot() { flashTwice(0, 255, 0); }
void ledStatusFlashShutdown() { flashTwice(0, 0, 255); }
