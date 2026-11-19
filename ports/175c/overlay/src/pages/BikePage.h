#pragma once
// OBD/TPMS page (page 5). The phone is the BLE central for the ELM327/TPMS
// adapter and forwards compact fields:
//   #OBD|rpm|kmh|coolantC|volt|gear|load|fuelLph
//   #TP|frontKpa|rearKpa|frontC|rearC
// The screen never opens a second BLE connection.

#include <Arduino.h>

static bool obdFresh() { return obdAt && (millis() - obdAt) < 5000; }
static bool tpFresh() { return tpAt && (millis() - tpAt) < 10000; }

void drawBikePage() {
  onWaitScreen = false;
  frameIsMap = false;
  gfx->fillScreen(C_BLACK);
  printCentered("BIKE", CX, 22, 3, C_WHITE);

  if (!obdFresh()) {
    printCentered("OBD WAIT", CX, 160, 4, 0x6B4D);
    printCentered("PAIR ELM327 / TPMS ON PHONE", CX, 210, 2, C_GREY);
    printCentered("PHONE RELAYS #OBD / #TP", CX, 240, 1, 0x4A69);
    drawPageDots();
    needFlush = true;
    return;
  }

  // RPM bar + number.
  int rpm = max(0, obdRpm);
  printCentered(String(rpm), CX, 58, 7, rpm > 9000 ? C_RED : C_WHITE);
  printCentered("RPM", CX, 112, 2, C_GREY);
  const int barW = 320, barX = CX - barW / 2, barY = 138;
  gfx->drawRoundRect(barX, barY, barW, 10, 5, 0x4208);
  int rw = (int)constrain((long)barW * rpm / 12000L, 0L, (long)barW);
  if (rw > 0) gfx->fillRoundRect(barX, barY, rw, 10, 5, rpm > 9000 ? C_RED : C_CYAN);

  // Speed + gear.
  printCentered(String(obdSpeed >= 0 ? obdSpeed : 0), CX - 60, 174, 6, C_WHITE);
  printCentered("KM/H", CX - 60, 226, 2, C_GREY);
  char gear[8];
  if (obdGear > 0) snprintf(gear, sizeof(gear), "%d", obdGear);
  else if (obdGear == 0) snprintf(gear, sizeof(gear), "N");
  else snprintf(gear, sizeof(gear), "--");
  printCentered(gear, CX + 96, 180, 7, C_ORANGE);
  printCentered("GEAR", CX + 96, 234, 2, C_GREY);

  // Small status grid.
  char buf[40];
  snprintf(buf, sizeof(buf), "COOL %d C", obdCoolant);
  printCentered(buf, CX - 110, 272, 2, obdCoolant > 105 ? C_RED : C_GREY);
  snprintf(buf, sizeof(buf), "VOLT %.1f", obdVolt);
  printCentered(buf, CX + 110, 272, 2, obdVolt > 0 && obdVolt < 11.5f ? C_RED : C_GREY);
  snprintf(buf, sizeof(buf), "LOAD %d%%", obdLoad);
  printCentered(buf, CX - 110, 300, 2, C_GREY);
  snprintf(buf, sizeof(buf), "FUEL %.1f L/H", obdFuelLph);
  printCentered(buf, CX + 110, 300, 2, C_GREY);

  // TPMS.
  if (tpFresh()) {
    const bool lowF = tpFrontKpa >= 0 && tpFrontKpa < 180;
    const bool lowR = tpRearKpa >= 0 && tpRearKpa < 180;
    snprintf(buf, sizeof(buf), "F %.2f BAR", tpFrontKpa / 100.0f);
    printCentered(buf, CX - 100, 344, 2, lowF ? C_RED : C_GREEN);
    snprintf(buf, sizeof(buf), "R %.2f BAR", tpRearKpa / 100.0f);
    printCentered(buf, CX + 100, 344, 2, lowR ? C_RED : C_GREEN);
    if (lowF || lowR) {
      gfx->drawCircle(CX, 233, 210, C_RED);
      printCentered("LOW TYRE PRESSURE", CX, 370, 2, C_RED);
    }
  } else {
    printCentered("TPMS --", CX, 344, 2, 0x6B4D);
  }

  printCentered("PHONE RELAY / NO ONBOARD BLE", CX, 402, 1, 0x4A69);
  drawPageDots();
  needFlush = true;
}
