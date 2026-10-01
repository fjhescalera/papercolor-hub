#include "led_status.h"

#include <Arduino.h>
#include <LedManager.h>

namespace {
LedManager ledManager;

void flashBlocking(LedColor color) {
  // flash() self-initializes (begin() runs lazily on first use) and is
  // non-blocking by design; pump update() here since this runs before
  // loop() ever starts (boot) or right before deep sleep (shutdown), with
  // no other loop available to drive it.
  ledManager.flash(color, /*count=*/2, /*onMs=*/150, /*offMs=*/150);
  while (ledManager.isFlashing()) {
    ledManager.update();
    delay(5);
  }
}
}  // namespace

void ledStatusFlashBoot() { flashBlocking(LedColor::green()); }
void ledStatusFlashShutdown() { flashBlocking(LedColor::blue()); }
