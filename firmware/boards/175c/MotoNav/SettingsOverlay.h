#pragma once
// Minimal on-device settings overlay for the 1.75C port.
// Brightness / rotation / map style. Sleep stays on the existing idle power-off.

#include "HomeVariant.h"
#include "src/power/ScreenState.h"

extern String pendingEvt;

bool settingsOpen = false;
uint8_t brightPct = 50;
int settingsThemeIdx = 0;
bool settingsLoaded = false;
uint32_t gIdlePowerOffMs = 300000;
int sleepMin = 5;

#define SET_PANEL_X 68
#define SET_PANEL_Y 64
#define SET_PANEL_W 330
#define SET_PANEL_H 352
#define SET_ROW_X (SET_PANEL_X + 16)
#define SET_ROW_W (SET_PANEL_W - 32)
#define SET_ROW0_Y 112
#define SET_ROW_STEP 52

static const char *themeName(int idx) {
  switch (idx & 3) {
    case 0: return "VICE";
    case 1: return "ORIG";
    case 2: return "BW";
    default: return "RADAR";
  }
}

void settingsLoad() {
  if (settingsLoaded) return;
  settingsLoaded = true;
  Preferences pr;
  pr.begin("motonav", true);
  brightPct = (uint8_t)pr.getInt("bright", 50);
  settingsThemeIdx = pr.getInt("theme", 0) & 3;
  homeStyle = pr.getInt("home", 0) == 1 ? HOME_FULL : HOME_RIDER;
  sleepMin = pr.getInt("sleep", 5);
  pr.end();
  if (brightPct != 25 && brightPct != 50 && brightPct != 75 && brightPct != 100) brightPct = 50;
  if (sleepMin != 0 && sleepMin != 3 && sleepMin != 5 && sleepMin != 10) sleepMin = 5;
  gIdlePowerOffMs = sleepMin == 0 ? 0 : (uint32_t)sleepMin * 60000UL;
  screenBright = (uint16_t)(((uint16_t)brightPct * 255 + 50) / 100);
  panel->setBrightness(screenBright);
}

void drawSettingsGear() {
  settingsLoad();
  const int gx = 120, gy = 396, r = 14;
  for (int i = 0; i < 8; i++) {
    float a = i * PI / 4.0f;
    gfx->fillCircle(gx + (int)(cosf(a) * r), gy + (int)(sinf(a) * r), 4, C_GREY);
  }
  aaDisc(gx, gy, r, C_GREY);
  aaDisc(gx, gy, r - 5, C_BLACK);
  printCentered("SET", gx, gy + 21, 1, C_GREY);
}

bool settingsGearHit(int x, int y) {
  const int gx = 120, gy = 396;
  int dx = x - gx, dy = y - gy;
  return dx * dx + dy * dy <= 30 * 30;
}

int settingsRowAt(int x, int y) {
  if (!settingsOpen) return -1;
  if (x < SET_ROW_X || x > SET_ROW_X + SET_ROW_W) return -1;
  for (int i = 0; i < 5; i++) {
    int ry = SET_ROW0_Y + i * SET_ROW_STEP;
    if (y >= ry - 22 && y <= ry + 22) return i;
  }
  return -1;
}

void drawSettingsSheet() {
  if (!settingsOpen) return;
  gfx->fillRoundRect(SET_PANEL_X, SET_PANEL_Y, SET_PANEL_W, SET_PANEL_H, 20, 0x1082);
  gfx->drawRoundRect(SET_PANEL_X, SET_PANEL_Y, SET_PANEL_W, SET_PANEL_H, 20, 0x4208);
  printCentered("SETTINGS", CX, SET_PANEL_Y + 16, 2, C_WHITE);

  for (int i = 0; i < 5; i++) {
    int ry = SET_ROW0_Y + i * SET_ROW_STEP;
    gfx->fillRoundRect(SET_ROW_X, ry - 22, SET_ROW_W, 44, 12, 0x18E3);
  }
  printCentered(String("BRIGHT   ") + String((int)brightPct) + "%", CX, SET_ROW0_Y - 10, 2, C_WHITE);
  printCentered(String("ROTATE   ") + screenDeg + " DEG", CX, SET_ROW0_Y + SET_ROW_STEP - 10, 2, C_WHITE);
  printCentered(String("THEME    ") + themeName(settingsThemeIdx), CX, SET_ROW0_Y + 2 * SET_ROW_STEP - 10, 2, C_WHITE);
  printCentered(String("SLEEP    ") + (sleepMin == 0 ? String("NEVER") : String(sleepMin) + " MIN"), CX, SET_ROW0_Y + 3 * SET_ROW_STEP - 10, 2, C_WHITE);
  printCentered(String("HOME     ") + homeStyleName(homeStyle), CX, SET_ROW0_Y + 4 * SET_ROW_STEP - 10, 2, C_WHITE);

  printCentered("tap row to change / outside closes", CX, SET_PANEL_Y + SET_PANEL_H - 24, 1, C_GREY);
  needFlush = true;
}

bool settingsTap(int x, int y) {
  if (destSheetOpen) return false;
  if (settingsOpen) {
    int idx = settingsRowAt(x, y);
    if (idx == 0) {
      brightPct = (brightPct == 25) ? 50 : (brightPct == 50) ? 75 : (brightPct == 75) ? 100 : 25;
      screenBright = (uint16_t)(((uint16_t)brightPct * 255 + 50) / 100);
      panel->setBrightness(screenBright);
      Preferences pr; pr.begin("motonav", false); pr.putInt("bright", brightPct); pr.end();
      drawSettingsSheet();
      return true;
    }
    if (idx == 1) {
      int d = (screenDeg + 90) % 360;
      setScreenDeg(d, true);
      drawSettingsSheet();
      return true;
    }
    if (idx == 2) {
      settingsThemeIdx = (settingsThemeIdx + 1) & 3;
      Preferences pr; pr.begin("motonav", false); pr.putInt("theme", settingsThemeIdx); pr.end();
      pendingEvt = String("T,SET_THEME,") + String(settingsThemeIdx);
      drawSettingsSheet();
      return true;
    }
    if (idx == 3) {
      sleepMin = (sleepMin == 3) ? 5 : (sleepMin == 5) ? 10 : (sleepMin == 10) ? 0 : 3;
      gIdlePowerOffMs = sleepMin == 0 ? 0 : (uint32_t)sleepMin * 60000UL;
      Preferences pr; pr.begin("motonav", false); pr.putInt("sleep", sleepMin); pr.end();
      drawSettingsSheet();
      return true;
    }
    if (idx == 4) {
      homeStyle = (homeStyle == HOME_FULL) ? HOME_RIDER : HOME_FULL;
      Preferences pr; pr.begin("motonav", false); pr.putInt("home", (int)homeStyle); pr.end();
      settingsOpen = false;
      drawWaiting(waitSt);
      return true;
    }
    if (x < SET_PANEL_X || x > SET_PANEL_X + SET_PANEL_W ||
        y < SET_PANEL_Y || y > SET_PANEL_Y + SET_PANEL_H) {
      settingsOpen = false;
      drawWaiting(waitSt);
    }
    return true;
  }
  if (settingsGearHit(x, y)) {
    destSheetOpen = false;
    settingsOpen = true;
    drawSettingsSheet();
    return true;
  }
  return false;
}
