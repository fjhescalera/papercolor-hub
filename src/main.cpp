#include <Arduino.h>
#include <Preferences.h>
#include <esp_system.h>
#include <M5Pm1.h>
#include <PowerManager.h>

#include "app_mode.h"
#include "apps.h"
#include "chime.h"
#include "led_status.h"
#include "mode_menu.h"
#include "native_display.h"

namespace {
AppMode mode = AppMode::Stocks;

bool takePendingFlag(const char* key) {
  Preferences prefs;
  prefs.begin("papercolor", false);
  const bool pending = prefs.getBool(key, false);
  if (pending) prefs.putBool(key, false);
  prefs.end();
  return pending;
}

AppMode persistedMode() {
  Preferences prefs;
  prefs.begin("papercolor", false);
  const uint8_t value = prefs.getUChar("mode", 0) % kAppModeCount;
  prefs.end();
  return static_cast<AppMode>(value);
}

void chooseMode() {
  if (takePendingFlag("pendingShutdown")) {
    // Reader asked for the chime + sleep hand-off (see reader_app.cpp's
    // showSleepScreenAndPowerOff()): play it here, where M5Unified can be
    // safely brought up, then deep-sleep. Never returns; the next wake is
    // detected via "pendingWake" below, not esp_reset_reason() — on this
    // board a plain reset (button or esptool's RTS toggle) does not fully
    // clear RTC-domain state, so esp_reset_reason() can keep reporting
    // ESP_RST_DEEPSLEEP on every later boot even without a real sleep/wake
    // cycle, which silently skipped the menu and chime on every reset.
    Preferences prefs;
    prefs.begin("papercolor", false);
    prefs.putBool("pendingWake", true);
    prefs.end();
    beginNativeDisplay();
    playShutdownChime();
    ledStatusFlashShutdown();
    freeink::PowerManager::deepSleepUntilPowerButton();
  }
  if (takePendingFlag("pendingReader")) {
    // Set by the menu when the user picks Reader; read back here on the
    // very next boot, before any M5Unified/menu/chime code runs. Reader
    // drives the panel directly via freeink-sdk's Ed2208M5Driver, which
    // must not share a process with M5Unified's own display/I2C init — a
    // quick software restart gives it the same pristine hardware state it
    // has today.
    mode = AppMode::Reader;
    return;
  }
  if (takePendingFlag("pendingWake")) {
    // Confirm this against the PMIC's own latched wake-source register
    // rather than trusting the flag alone: Preferences survives an
    // interrupted shutdown handoff or a reflash, and a stale flag with no
    // real wake behind it would otherwise skip the menu and chime forever.
    freeink::m5pm1::beginBus();
    uint8_t wakeSource = 0;
    const bool gotWakeSource = freeink::m5pm1::readWakeSource(&wakeSource);
    freeink::m5pm1::clearWakeSource();
    if (gotWakeSource && (wakeSource & freeink::m5pm1::WAKE_PWR_BUTTON)) {
      // Waking Reader's own sleep screen: resume instantly, no menu, no chime.
      mode = persistedMode();
      return;
    }
    // Flag was stale with no matching wake event — fall through to the
    // normal cold-boot path below instead of staying silently stuck.
  }
  beginNativeDisplay();
  playBootChime();
  ledStatusFlashBoot();
  mode = runModeMenu();
  if (mode == AppMode::Reader) {
    Preferences prefs;
    prefs.begin("papercolor", false);
    prefs.putBool("pendingReader", true);
    prefs.end();
    esp_restart();
  }
}
}

void setup() {
  Serial.begin(115200);
  chooseMode();
  switch (mode) {
    case AppMode::Stocks: stockSetup(); break;
    case AppMode::Reader: readerSetup(); break;
    case AppMode::Calendar: calendarSetup(); break;
    case AppMode::PhotoFrame: photoFrameSetup(); break;
    case AppMode::Sudoku: sudokuSetup(); break;
  }
}

void loop() {
  switch (mode) {
    case AppMode::Stocks: stockLoop(); break;
    case AppMode::Reader: readerLoop(); break;
    case AppMode::Calendar: calendarLoop(); break;
    case AppMode::PhotoFrame: photoFrameLoop(); break;
    case AppMode::Sudoku: sudokuLoop(); break;
  }
}
