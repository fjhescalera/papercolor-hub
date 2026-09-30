#include <Arduino.h>
#include <Preferences.h>
#include <esp_system.h>
#include <PowerManager.h>

#include "app_mode.h"
#include "apps.h"
#include "chime.h"
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
    // safely brought up, then deep-sleep. Never returns; the next wake goes
    // through the ESP_RST_DEEPSLEEP branch below.
    beginNativeDisplay();
    playShutdownChime();
    PowerManager::deepSleepUntilPowerButton();
  }
  if (takePendingFlag("pendingReader")) {
    // Set by the menu when the user picks Reader; read back here on the
    // very next boot, before any M5Unified/menu/chime code runs. Reader
    // drives the panel directly via freeink-sdk's Ed2208M5Driver, which
    // must not share a process with M5Unified's own display/I2C init (see
    // native_display.cpp's reassertDisplayPowerRail() comment for the I2C
    // half of this) — a quick software restart gives it the same pristine
    // hardware state it has today.
    mode = AppMode::Reader;
    return;
  }
  if (esp_reset_reason() == ESP_RST_DEEPSLEEP) {
    // Waking Reader's own sleep screen: resume instantly, no menu, no chime.
    mode = persistedMode();
    return;
  }
  beginNativeDisplay();
  playBootChime();
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
