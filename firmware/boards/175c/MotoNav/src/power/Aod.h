#pragma once
// Always-on display and wrist wake for the 1.75C.
//
// AOD is deliberately a "fake" AOD first: the panel stays on at a low
// brightness, the clock is redrawn once per second and the whole clock is
// shifted a couple of pixels every two minutes. This avoids the PSRAM/BLE/USB
// light-sleep constraints while still giving a watch-like standby screen.
//
// Included after HomePage/IncomingCall/PwrKeyScreen so it can call their
// redraw helpers.

#include <Arduino.h>
#include "WristWake.h"

extern bool notifyOpen;
extern bool notifyBanner;

static uint32_t aodIdleAt = 0;
static uint32_t aodLastDraw = 0;

void aodResetIdle() { aodIdleAt = millis(); }

static bool aodIdleAllowed() {
  return !screenDark && !screenAod && !inNav && !cruiseOn && !callOn && !pairOn &&
         !pairReq && !settingsOpen && !destSheetOpen && !leanOpen && !toyIsOpen() &&
         !companionOpen && !notifyOpen && !notifyBanner && !sliding && !touchDown && !fxOn &&
         page == 0;
}

static void aodDraw() {
  char timeBuf[6] = "--:--";
  char dateBuf[11] = "";
  const bool valid = homeClock.format(millis(), timeBuf, dateBuf);
  // 2x2 px burn-in drift, changing every two minutes.
  const uint32_t phase = (millis() / 120000UL) & 3;
  const int dx = phase == 1 ? 2 : phase == 2 ? -2 : 0;
  const int dy = phase == 1 ? 1 : phase == 2 ? -1 : 0;
  gfx->fillScreen(0);
  printCentered(valid ? String(timeBuf) : String("--:--"), CX + dx, 184 + dy, 8, 0x5AEB);
  if (valid) printCentered(String(dateBuf), CX + dx, 282 + dy, 2, 0x4208);
  if (batPct >= 0) printCentered(String(batPct) + "%", CX, 344, 2, 0x3186);
  printCentered("AOD", CX, 404, 1, 0x18E3);
  needFlush = true;
}

void enterAod() {
  if (screenDark || screenAod || !aodEnabled) return;
  if (sliding) dragAbort();
  screenAod = true;
  screenDark = true;                 // CPU governor / normal draw paths skip
  panel->displayOn();
  panel->setBrightness(aodBright);
  aodDraw();
  Serial.printf("AOD on bright=%u\n", (unsigned)aodBright);
}

void wakeFromAod() {
  if (!screenAod) return;
  screenAod = false;
  aodResetIdle();
  pwrKeyBusLocked(false);            // restores brightness and redraws current page
  Serial.println("AOD wake");
}

// Call once per loop, before touchPoll(). Handles wrist events and the
// auto-AOD timeout; draws the clock while AOD is active.
void aodTick() {
  const uint8_t ev = wristTakeEvent();
  if (ev == WristRaise::RAISE) {
    if (screenAod) wakeFromAod();
    else if (screenDark) pwrKeyBusLocked(false);
    aodResetIdle();
  } else if (ev == WristRaise::LOWER) {
    if (aodIdleAllowed()) enterAod();
  }

  if (screenAod) {
    const uint32_t now = millis();
    if (now - aodLastDraw >= 1000) {
      aodLastDraw = now;
      aodDraw();
    }
    return;
  }

  if (!aodIdleAllowed()) { aodResetIdle(); return; }
  if (!aodIdleAt) aodResetIdle();
  if (aodEnabled && aodAfterMs && millis() - aodIdleAt >= aodAfterMs) enterAod();
}
