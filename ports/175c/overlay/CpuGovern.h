#pragma once
#include <Arduino.h>
#include "ScreenState.h"

// Manual DFS for the precompiled Arduino-ESP32 core (no CONFIG_PM_ENABLE).
// Idle waiting screen and screen-off run at 80MHz; touch/BLE/pages boost first.
void cpuBoost() {
  if (getCpuFrequencyMhz() == 240) return;
  setCpuFrequencyMhz(240);
  Serial.printf("CPU %dMHz\n", (int)getCpuFrequencyMhz());
}

void cpuGovern() {
  static bool low = false;
  const bool busy = touchDown || dragging || sliding ||
                    pairReq || callOn || fxOn || rxTail != rxHead;
  const bool wantLow = screenDark || (onWaitScreen && !busy);
  if (wantLow == low) return;
  low = wantLow;
  setCpuFrequencyMhz(low ? 80 : 240);
  Serial.printf("CPU %dMHz\n", (int)getCpuFrequencyMhz());
}
