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
  M5.Display.startWrite();
  drawRow(from, false);
  drawRow(to, true);
  M5.Display.endWrite();
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

  // Fast, black-and-white mode for the rest of navigation: epd_text is the
  // panel's text-oriented LUT (built for exactly this kind of quick partial
  // update), not the slower color/grayscale epd_quality mode used above.
  M5.Display.setEpdMode(epd_mode_t::epd_text);

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
