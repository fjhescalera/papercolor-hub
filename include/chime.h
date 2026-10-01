#pragma once

// Plays the /chimes/ding.wav asset from the SD card once (falls back to a
// synthesized two-tone beep if the card or file is missing). Call after
// beginNativeDisplay(), only on a genuine cold boot — main.cpp gates that
// with its own "pendingWake" flag, not this function.
void playBootChime();

// Two short descending tones, blocks until playback finishes. Call right
// before PowerManager::deepSleepUntilPowerButton() cuts power.
void playShutdownChime();
