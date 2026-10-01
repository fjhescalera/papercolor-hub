#include "mode_menu.h"

#include <Preferences.h>

#include "native_display.h"

namespace {
constexpr AppMode kOrder[kAppModeCount] = {
    AppMode::Stocks, AppMode::Reader, AppMode::Calendar, AppMode::PhotoFrame, AppMode::Sudoku,
};
// Plain ASCII labels, matching every other screen in this codebase (e.g.
// stock_app.cpp's "TW STOCK WATCHLIST"): the i18n TextId/tr() strings are
// Traditional Chinese by default locale, and the fonts used here and
// everywhere else in the app (FreeSansBold12pt7b etc.) have no CJK glyphs,
// which rendered as blank boxes.
constexpr const char* kLabelFor[kAppModeCount] = {
    "Stocks", "Reader", "Calendar", "Photo Frame", "Sudoku",
};

constexpr int kRowX = 40;
constexpr int kRowWidth = 520;
constexpr int kRowHeight = 50;
constexpr int kRowGap = 8;
constexpr int kFirstRowY = 64;

int rowY(uint8_t index) { return kFirstRowY + index * (kRowHeight + kRowGap); }

void drawRow(uint8_t index, bool highlighted) {
  const uint32_t bg = highlighted ? BLACK : WHITE;
  const uint32_t fg = highlighted ? WHITE : BLACK;
  M5.Display.fillRect(kRowX, rowY(index), kRowWidth, kRowHeight, bg);
  M5.Display.setTextColor(fg, bg);
  M5.Display.setTextDatum(middle_left);
  M5.Display.setFont(&fonts::FreeSansBold12pt7b);
  M5.Display.drawString(kLabelFor[index], kRowX + 20, rowY(index) + kRowHeight / 2);
}

// Only the two rows that actually changed, in one SPI transaction, so the
// panel refreshes just that strip instead of the whole screen — this is
// what makes nav fast; the full nativeHeader/nativeFooter paint happens
// once, on entry, at epd_quality.
void moveHighlight(uint8_t from, uint8_t to) {
  const uint32_t start = millis();
  M5.Display.startWrite();
  drawRow(from, false);
  drawRow(to, true);
  M5.Display.endWrite();
  Serial.printf("[diag] moveHighlight epdMode=%d took=%lums\n",
                static_cast<int>(M5.Display.getEpdMode()), millis() - start);
}
}  // namespace

AppMode runModeMenu() {
  Preferences prefs;
  prefs.begin("papercolor", false);
  uint8_t cursor = prefs.getUChar("mode", 0) % kAppModeCount;
  prefs.end();

  M5.Display.setEpdMode(epd_mode_t::epd_quality);
  M5.Display.startWrite();
  M5.Display.fillScreen(WHITE);
  nativeHeader("SELECT MODE");
  for (uint8_t i = 0; i < kAppModeCount; ++i) drawRow(i, i == cursor);
  nativeFooter("A prev   B next   C select");
  M5.Display.endWrite();

  // For this panel (ED2208), epd_mode only changes the dithering math in
  // Panel_ED2208::_exec_transfer() -- the actual physical refresh sequence
  // in _turn_on_display() (POWER_ON / DISPLAY_REFRESH / POWER_OFF, each
  // waiting on the panel's own busy pin) is identical regardless of mode.
  // epd_fast's simpler dithering is a little cheaper to compute than
  // epd_text's, but neither changes the dominant cost: the panel's own
  // fixed refresh time. True fast partial refresh on this panel only
  // exists in the separate driver Reader uses, not here.
  M5.Display.setEpdMode(epd_mode_t::epd_fast);

  for (;;) {
    M5.update();
    if (M5.BtnA.wasReleased()) {
      const uint8_t next = static_cast<uint8_t>((cursor + kAppModeCount - 1) % kAppModeCount);
      moveHighlight(cursor, next);
      cursor = next;
    } else if (M5.BtnB.wasReleased()) {
      const uint8_t next = static_cast<uint8_t>((cursor + 1) % kAppModeCount);
      moveHighlight(cursor, next);
      cursor = next;
    } else if (M5.BtnC.wasReleased()) {
      prefs.begin("papercolor", false);
      prefs.putUChar("mode", cursor);
      prefs.end();
      return kOrder[cursor];
    }
    delay(20);
  }
}
