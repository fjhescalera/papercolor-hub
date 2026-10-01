#pragma once

// Status-LED flashes for the two power events that already have a chime
// (see chime.h). Colors are an arbitrary firmware convention, not reverse
// engineered from the factory firmware: that device's own LED behavior
// runs on the M5PM1 power chip's separate, uninspectable firmware, not
// anything visible in the ESP32 app image. Green = boot, blue = shutdown.
// Each call blocks briefly (a couple hundred milliseconds) until its flash
// sequence finishes, same as the chime beeps in chime.cpp.
void ledStatusFlashBoot();
void ledStatusFlashShutdown();
