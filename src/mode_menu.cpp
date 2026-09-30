#include "mode_menu.h"

#include <Preferences.h>

#include "i18n.h"
#include "native_display.h"

namespace {
constexpr AppMode kOrder[kAppModeCount] = {
    AppMode::Stocks, AppMode::Reader, AppMode::Calendar, AppMode::PhotoFrame, AppMode::Sudoku,
};
// Explicit table rather than casting AppMode <-> TextId: the two enums
// currently share ordinal order by coincidence, and a cast would silently
// break if either is reordered independently later.
constexpr TextId kLabelFor[kAppModeCount] = {
    TextId::Stocks, TextId::Reader, TextId::Calendar, TextId::PhotoFrame, TextId::Sudoku,
};

constexpr int kRowX = 40;
constexpr int kRowWidth = 520;
constexpr int kRowHeight = 50;
constexpr int kRowGap = 8;
constexpr int kFirstRowY = 64;

int rowY(uint8_t index) { return kFirstRowY + index * (kRowHeight + kRowGap); }

void drawRow(uint8_t index, bool highlighted, Locale locale) {
  const uint32_t bg = highlighted ? BLACK : WHITE;
  const uint32_t fg = highlighted ? WHITE : BLACK;
  M5.Display.fillRect(kRowX, rowY(index), kRowWidth, kRowHeight, bg);
  M5.Display.setTextColor(fg, bg);
  M5.Display.setTextDatum(middle_left);
  M5.Display.setFont(&fonts::FreeSansBold12pt7b);
  M5.Display.drawString(tr(kLabelFor[index], locale), kRowX + 20, rowY(index) + kRowHeight / 2);
}

// Full repaint, same startWrite/fillScreen/.../endWrite shape every other
// app in this codebase uses (see stock_app.cpp/sudoku_app.cpp render()) —
// epd_fast (set by the caller during navigation) makes this fast enough to
// redo on every keypress without a hand-rolled partial-redraw path.
void renderMenu(uint8_t cursor, Locale locale) {
  M5.Display.startWrite();
  M5.Display.fillScreen(WHITE);
  nativeHeader("SELECT MODE");
  for (uint8_t i = 0; i < kAppModeCount; ++i) drawRow(i, i == cursor, locale);
  nativeFooter("A prev   C next   B select");
  M5.Display.endWrite();
}
}  // namespace

AppMode runModeMenu() {
  Preferences prefs;
  prefs.begin("papercolor", false);
  uint8_t cursor = prefs.getUChar("mode", 0) % kAppModeCount;
  prefs.end();

  const Locale locale = detectLocale();
  M5.Display.setEpdMode(epd_mode_t::epd_quality);
  renderMenu(cursor, locale);
  M5.Display.setEpdMode(epd_mode_t::epd_fast);

  for (;;) {
    M5.update();
    if (M5.BtnA.wasReleased()) {
      cursor = static_cast<uint8_t>((cursor + kAppModeCount - 1) % kAppModeCount);
      renderMenu(cursor, locale);
    } else if (M5.BtnC.wasReleased()) {
      cursor = static_cast<uint8_t>((cursor + 1) % kAppModeCount);
      renderMenu(cursor, locale);
    } else if (M5.BtnB.wasReleased()) {
      prefs.begin("papercolor", false);
      prefs.putUChar("mode", cursor);
      prefs.end();
      return kOrder[cursor];
    }
    delay(20);
  }
}
