#pragma once
// Companion data that is not navigation: phone battery, rain, next fuel,
// speed breaker and the ride summary. Drawn as a small overlay after the
// normal page so it never changes the navigation state machine.

#include <Arduino.h>

static void phoneRedrawCurrent() {
  if (page == 2) musicDirty = true;
  else if (page == 1) wxDirty = true;
  else if (page == 3) { /* compass redraws on its own */ }
  else if (page == 4) notifyDirty = true;
  else if (inNav) navDirty = true;
  else drawWaiting(bleConnected && bleAuthed ? WAIT_READY : WAIT_NOLINK);
}

void drawPhoneStatus() {
  if (screenDark || page != 0 || notifyBanner) return;
  int lines = 0;
  char buf[40];
  uint16_t colors[4];
  String text[4];
  if (phonePct >= 0) {
    snprintf(buf, sizeof(buf), "PH %d%%%s", phonePct, phoneChg ? "+" : "");
    text[lines] = buf; colors[lines++] = phoneChg ? C_CYAN : 0x8C71;
  }
  if (rainMin >= 0) {
    snprintf(buf, sizeof(buf), "RAIN %dm", rainMin);
    text[lines] = buf; colors[lines++] = C_CYAN;
  }
  if (fuelNextM >= 0) {
    snprintf(buf, sizeof(buf), "FUEL %.1fkm", fuelNextM / 1000.0f);
    text[lines] = buf; colors[lines++] = C_GREEN;
  }
  if (bumpM >= 0) {
    snprintf(buf, sizeof(buf), "BUMP %dm", bumpM);
    text[lines] = buf; colors[lines++] = C_ORANGE;
  }
  if (!lines) return;
  const int h = 10 + lines * 14;
  gfx->fillRoundRect(18, 58, 150, h, 9, 0x0000);
  gfx->drawRoundRect(18, 58, 150, h, 9, 0x31A6);
  for (int i = 0; i < lines; i++) {
    printCentered(text[i], 93, 64 + i * 14, 1, colors[i]);
  }
  needFlush = true;
}

void drawRideSummary() {
  onWaitScreen = false;
  frameIsMap = false;
  gfx->fillScreen(C_BLACK);
  printCentered("RIDE COMPLETE", CX, 54, 4, C_GREEN);
  char buf[40];
  snprintf(buf, sizeof(buf), "%.2f KM", rideKm);
  printCentered(buf, CX, 116, 5, C_WHITE);
  snprintf(buf, sizeof(buf), "%d MIN", rideMin);
  printCentered(buf, CX, 166, 3, C_CYAN);
  snprintf(buf, sizeof(buf), "AVG %.1f   MAX %.1f KM/H", rideAvg, rideMax);
  printCentered(buf, CX, 210, 2, C_GREY);
  snprintf(buf, sizeof(buf), "LEAN L %.0f   R %.0f", rideLeanL, rideLeanR);
  printCentered(buf, CX, 244, 2, C_GREY);
  snprintf(buf, sizeof(buf), "MAX G %.2f", rideMaxG);
  printCentered(buf, CX, 278, 2, C_GREY);
  printCentered("TAP TO CLOSE", CX, 360, 2, 0x6B4D);
  needFlush = true;
}

bool rideSummaryTap(int, int) {
  if (!rideSummaryOpen) return false;
  rideSummaryOpen = false;
  // Start the next ride's lean/G maxima from a clean slate.
  leanMaxL = leanMaxR = 0;
  leanMaxG = 0;
  phoneRedrawCurrent();
  needFlush = true;
  return true;
}

void phoneStatusTick() {
  if (screenDark) return;
  if (rideSummaryOpen) {
    if (millis() - rideSummaryAt > 10000) {
      rideSummaryOpen = false;
      phoneRedrawCurrent();
    } else {
      drawRideSummary();
      return;
    }
  }
  drawPhoneStatus();
}
