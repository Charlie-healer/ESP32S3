#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "ScreenState.h"

// AXP2101 PWR short press = screen off/on. Long press remains the PMIC's
// hardware shutdown; this file never touches the power-off registers.
// INTEN2=0x41 (enable), INTSTS2=0x49 (status), PKEY_SHORT=bit3, write 1 clears.
#define AXP2101_INTEN2  0x41
#define AXP2101_INTSTS2 0x49

static bool pwrKeyReady = false;

void pwrKeyBusLocked(bool dark) {
  if (flushDone) xSemaphoreTake(flushDone, portMAX_DELAY);
  if (dark) {
    panel->setBrightness(0);
    panel->displayOff();
    screenDark = true;
    Serial.println("PWR screen off");
  } else {
    cpuBoost();
    panel->displayOn();
    panel->setBrightness(screenBright);
    screenDark = false;
    Serial.println("PWR screen on");
    // Restore whichever screen was active when the panel was switched off.
    if (pairOn) {
      drawPairScreen();
    } else if (callOn) {
      drawCallScreen();
    } else if (page == 2) {
      musicDirty = true;
    } else if (page == 1) {
      wxDirty = true;
    } else if (inNav) {
      navDirty = true;
    } else {
      drawWaiting((bleConnected && bleAuthed) ? WAIT_READY : WAIT_NOLINK);
    }
  }
  if (flushDone) xSemaphoreGive(flushDone);
}

void pwrKeyPoll() {
  if (!pmuOK) return;
  if (!pwrKeyReady) {
    pwrKeyReady = true;
    pmuSetBit(AXP2101_INTEN2, 3);       // enable PKEY short-press event
  }
  static unsigned long pkeyAt = 0;
  if (millis() - pkeyAt < 50) return;
  pkeyAt = millis();
  const int sts = pmuRead(AXP2101_INTSTS2);
  if (sts < 0 || !(sts & 0x08)) return;  // bit3 = PKEY short press
  Wire.beginTransmission(PMU_ADDR);
  Wire.write(AXP2101_INTSTS2);
  Wire.write((uint8_t)0xFF);             // write 1 to clear
  Wire.endTransmission();
  if (screenDark) {
    pwrKeyBusLocked(false);
  } else {
    if (sliding) dragAbort();
    pwrKeyBusLocked(true);
  }
}
