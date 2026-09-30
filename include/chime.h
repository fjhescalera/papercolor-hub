#pragma once

// Plays the /chimes/ding.wav asset from the SD card once (falls back to a
// synthesized two-tone beep if the card or file is missing). No-op if this
// boot is a deep-sleep wake, so it fires once per real power-on, not on
// every resume-from-sleep. Call after beginNativeDisplay().
void playBootChime();

// Two short descending tones, blocks until playback finishes. Call right
// before PowerManager::deepSleepUntilPowerButton() cuts power.
void playShutdownChime();
