#pragma once
// Companion-mode glue for the 1.75C. AvatarEngine/CompanionMode are ported
// from the StopWatch fork; vibration is replaced by ES8311 sound feedback.
#include "src/companion/CompanionMode.h"

static void drawCompanionClose() {
  const int cx = 406, cy = 54, r = 17;
  aaDisc(cx, cy, r, 0x18E3);
  gfx->drawCircle(cx, cy, r, 0x4A69);
  gfx->drawLine(cx - 5, cy - 5, cx + 5, cy + 5, C_WHITE);
  gfx->drawLine(cx - 5, cy + 5, cx + 5, cy - 5, C_WHITE);
}

void companionRedraw() {
  CompanionMode::drawInitialFrame();
  drawCompanionClose();
  needFlush = true;
}

void companionEnter() {
  destSheetOpen = false;
  settingsOpen = false;
  leanOpen = false;
  companionOpen = true;
  onWaitScreen = false;
  CompanionMode::enter();
  drawCompanionClose();
  needFlush = true;
}

void companionTick() {
  CompanionMode::tick();
  drawCompanionClose();
  needFlush = true;
}

void companionTouchPoll() {
  if (!touchOK) return;
  static bool down = false;
  static unsigned long last = 0;
  if (millis() - last < 12) return;
  last = millis();
  int16_t x[1] = {0}, y[1] = {0};
  uint8_t n = touch.getPoint(x, y, 1);
  int tx = x[0], ty = y[0];
  touchPointToCanvas(tx, ty);
  if (n > 0) {
    if (!down) {
      down = true;
      if (tx > 380 && ty < 90) {
        down = false;
        companionOpen = false;
        onWaitScreen = true;
        drawWaiting(waitSt);
        return;
      }
    }
    CompanionMode::handleTouch(true, tx, ty);
  } else if (down) {
    down = false;
    CompanionMode::handleTouch(false, tx, ty);
  }
}
