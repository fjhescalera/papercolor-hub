#pragma once

#include "app_mode.h"

// Boot-time mode picker. Caller must already have brought the display up via
// beginNativeDisplay() (M5.begin() done, epd_quality set). Blocks until the
// user confirms a mode with button B, persisting the choice to the same
// Preferences("papercolor","mode") key chooseMode() used to write, then
// returns it. Does not launch the chosen app itself.
AppMode runModeMenu();
