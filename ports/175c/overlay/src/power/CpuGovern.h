#pragma once
#include <Arduino.h>
#include "ScreenState.h"

// Manual DFS for the precompiled Arduino-ESP32 core (no CONFIG_PM_ENABLE).
// Screen-on runs at 240MHz; only screen-off drops to 80MHz.
void cpuBoost() {
  if (getCpuFrequencyMhz() == 240) return;
  setCpuFrequencyMhz(240);
  Serial.printf("CPU %dMHz\n", (int)getCpuFrequencyMhz());
}

void cpuGovern() {
  static bool low = false;
  // Keep 240MHz whenever the panel is on. The old policy also dropped to
  // 80MHz on the idle waiting page, which made home animations, the settings
  // overlay and the destination drawer feel sluggish. Only screen-off sleeps
  // at 80MHz now; PWR wake calls cpuBoost() before repainting.
  const bool wantLow = screenDark;
  if (wantLow == low) return;
  low = wantLow;
  setCpuFrequencyMhz(low ? 80 : 240);
  Serial.printf("CPU %dMHz\n", (int)getCpuFrequencyMhz());
}
