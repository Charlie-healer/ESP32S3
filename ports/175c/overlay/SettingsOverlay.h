#pragma once
// Full-screen on-device settings page for the 1.75C.
//
// Layout follows the StopWatch branch's 4-card settings page (brightness /
// map style / rotation / auto sleep), with five bottom pills (home skin /
// companion / sound / music layout / AOD) and a back pill.
// The background follows the map theme selected on the phone (12 styles).
// ROTATION has a fifth GRAV state: a rotation-calibration view with crosshairs
// that follows the QMI8658 until the user taps to lock.

#include "HomeVariant.h"
#include "src/power/ScreenState.h"

extern String pendingEvt;
extern int soundVolume;
extern bool musicDirty;
void audioSetVolume(int volume);
void wakeFromAod();
void aodResetIdle();

bool settingsOpen = false;
uint8_t brightPct = 50;
int settingsThemeIdx = 0;
bool settingsLoaded = false;
uint32_t gIdlePowerOffMs = 300000;
int sleepMin = 5;
bool screenAutoRot = false;
bool musicMinimal = false;
float autoRotAngle = 0;
uint32_t autoRotAt = 0;

// On-device MAP STYLE cycle. Raw codes are translated by the phone app's
// CallWatcher SET_THEME handler: 0 VICE -> app theme 1, 1 ORIG -> 0,
// 2 BW -> 5, 3 RADAR -> 8, 5 GTA5 -> 2. DRAGON(11) stays phone-picker only,
// and raw 4 remains reserved for it so older/newer sides stay compatible.
static const int THEME_CYCLE[] = {0, 1, 5, 2, 3};
static const char *THEME_CYCLE_NAME[] = {"VICE", "ORIG", "GTA5", "BW", "RADAR"};
static const int THEME_CYCLE_N = (int)(sizeof(THEME_CYCLE) / sizeof(THEME_CYCLE[0]));

static const char *themeName(int idx) {
  for (int i = 0; i < THEME_CYCLE_N; i++)
    if (THEME_CYCLE[i] == idx) return THEME_CYCLE_NAME[i];
  return "VICE";
}

static inline int themeCycleNext(int idx) {
  for (int i = 0; i < THEME_CYCLE_N; i++)
    if (THEME_CYCLE[i] == idx) return THEME_CYCLE[(i + 1) % THEME_CYCLE_N];
  return THEME_CYCLE[0];
}

void settingsLoad() {
  if (settingsLoaded) return;
  settingsLoaded = true;
  Preferences pr;
  pr.begin("motonav", true);
  brightPct = (uint8_t)pr.getInt("bright", 50);
  int savedTheme = pr.getInt("theme", 0);
  settingsThemeIdx = THEME_CYCLE[0];
  for (int i = 0; i < THEME_CYCLE_N; i++)
    if (THEME_CYCLE[i] == savedTheme) settingsThemeIdx = savedTheme;
  homeStyle = pr.getInt("home", 0) == 1 ? HOME_FULL : HOME_RIDER;
  sleepMin = pr.getInt("sleep", 5);
  soundVolume = pr.getInt("sound", 80);
  musicMinimal = pr.getBool("mmin", false);
  aodEnabled = pr.getBool("aod", true);
  pr.end();
  if (brightPct != 25 && brightPct != 50 && brightPct != 75 && brightPct != 100) brightPct = 50;
  if (sleepMin != 0 && sleepMin != 3 && sleepMin != 5 && sleepMin != 10) sleepMin = 5;
  gIdlePowerOffMs = sleepMin == 0 ? 0 : (uint32_t)sleepMin * 60000UL;
  if (soundVolume < 0 || soundVolume > 100) soundVolume = 80;
  audioSetVolume(soundVolume);
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

// ---------------- full-screen page ----------------

static void drawSettingsIcon(int id, int cx, int cy, uint16_t color) {
  switch (id) {
    case 0:  // brightness
      aaDisc(cx, cy, 4.5f, color);
      for (int a = 0; a < 360; a += 45) {
        float rad = a * PI / 180.0f;
        gfx->drawLine((int)lroundf(cx + 7 * cosf(rad)), (int)lroundf(cy + 7 * sinf(rad)),
                      (int)lroundf(cx + 11 * cosf(rad)), (int)lroundf(cy + 11 * sinf(rad)), color);
      }
      break;
    case 1:  // map style
      gfx->drawRoundRect(cx - 10, cy - 8, 20, 16, 2, color);
      gfx->drawLine(cx - 10, cy - 1, cx + 10, cy - 1, color);
      gfx->drawLine(cx - 3, cy - 8, cx - 3, cy + 8, color);
      aaDisc(cx + 4, cy + 3, 2.0f, color);
      break;
    case 2:  // rotation
      gfx->drawCircle(cx, cy, 9, color);
      gfx->drawCircle(cx, cy, 10, color);
      gfx->fillRect(cx + 3, cy - 11, 8, 8, C_BLACK);
      gfx->fillTriangle(cx + 2, cy - 11, cx + 10, cy - 11, cx + 6, cy - 5, color);
      break;
    default:  // moon / sleep
      aaDisc(cx, cy, 8.0f, color);
      aaDisc(cx + 4.0f, cy - 3.0f, 7.0f, C_BLACK);
      break;
  }
}

static void drawSettingsBackground() {
  const int tid = styleIdx();
  const MapTheme &th = nightOn ? THEMES_NIGHT[tid] : THEMES[tid];
  gfx->fillScreen(th.block);
  if (tid == ST_DRAGON) DragonRadar::drawGrid(gfx, LCD_WIDTH, LCD_HEIGHT, nightOn);
  if (tid == ST_RADAR) {
    gfx->drawCircle(CX, CX, 75, th.road);
    gfx->drawCircle(CX, CX, 150, th.road);
    gfx->drawCircle(CX, CX, 210, th.road);
  }
  if (th.water && th.water != th.block) {
    for (int y = 0; y < LCD_HEIGHT; y++) {
      float ny = (float)y / 466.0f;
      int wx = (int)(78 + 32.0f * sinf(ny * 3.14159f * 1.5f));
      int x0 = max(0, wx - 22);
      int x1 = min(LCD_WIDTH - 1, wx + 22);
      for (int x = x0; x <= x1; x++) {
        uint16_t *fb = gfx->getFramebuffer();
        if (fb) fb[y * LCD_WIDTH + x] = th.water;
      }
    }
  }
  if (th.park && th.park != th.block && th.park != th.water) {
    gfx->fillRect(140, 64, 52, 44, th.park);
    gfx->fillRect(276, 300, 64, 52, th.park);
  }
  if (th.road && th.road != th.block) {
    for (int x = 42; x < LCD_WIDTH; x += 48) gfx->fillRect(x - 2, 0, 4, LCD_HEIGHT, th.road);
    for (int y = 42; y < LCD_HEIGHT; y += 48) gfx->fillRect(0, y - 2, LCD_WIDTH, 4, th.road);
  }
  if (th.route) {
    gfx->fillRect(CX - 2, 150, 4, 120, th.route);
    gfx->fillRect(CX - 2, 150, 90, 4, th.route);
  }
}

void drawSettingsSheet() {
  if (!settingsOpen) return;
  drawSettingsBackground();
  if (screenAutoRot) {
    gfx->drawLine(0, CX, LCD_WIDTH, CX, 0x7BEF);
    gfx->drawLine(CX, 0, CX, LCD_HEIGHT, 0x7BEF);
  }

  // Title + battery (title on a dark pill so it stays readable on light themes).
  if (screenAutoRot) {
    gfx->fillRoundRect(CX - 56, 66, 112, 30, 15, 0x0000);
    gfx->drawRoundRect(CX - 56, 66, 112, 30, 15, 0x4A69);
    char ab[16];
    snprintf(ab, sizeof(ab), "%d*", screenDeg);
    printCentered(ab, CX, 73, 2, C_WHITE);
  }
  gfx->fillRoundRect(CX - 78, 20, 156, 32, 16, 0x0000);
  gfx->drawRoundRect(CX - 78, 20, 156, 32, 16, 0x4208);
  printCentered("SETTINGS", CX, 27, 2, C_WHITE);
  drawBattery(CX - 16, 56, true);

  const int cardW = 160, cardH = 106, gapX = 12, gapY = 14;
  const int leftX = CX - cardW - gapX / 2;   // 67
  const int rightX = CX + gapX / 2;          // 239
  const int topY = 84;
  const int botY = topY + cardH + gapY;      // 204

  struct Card { int x, y; int icon; const char *title; String value; };
  Card cards[4] = {
    {leftX, topY, 0, "BRIGHTNESS", String(brightPct) + "%"},
    {rightX, topY, 1, "MAP STYLE", themeName(settingsThemeIdx)},
    {leftX, botY, 2, "ROTATION", screenAutoRot ? String("GRAV") : String(screenDeg) + " DEG"},
    {rightX, botY, 3, "AUTO SLEEP", sleepMin == 0 ? String("NEVER") : String(sleepMin) + " MIN"},
  };
  for (int i = 0; i < 4; i++) {
    int cx = cards[i].x, cy = cards[i].y;
    gfx->fillRoundRect(cx, cy, cardW, cardH, 16, 0x1082);
    gfx->drawRoundRect(cx, cy, cardW, cardH, 16, 0x4208);
    drawSettingsIcon(cards[i].icon, cx + cardW / 2, cy + 22, 0xDEFB);
    printCentered(cards[i].title, cx + cardW / 2, cy + 40, 1, 0x8C71);
    gfx->fillRoundRect(cx + 14, cy + 66, cardW - 28, 26, 13, 0x0000);
    gfx->drawRoundRect(cx + 14, cy + 66, cardW - 28, 26, 13, 0x31A6);
    printCentered(cards[i].value, cx + cardW / 2, cy + 73, 2, C_WHITE);
  }

  // Bottom pills: home skin / companion / sound / music layout / AOD.
  const int pillW = 80, pillH = 32, pillY = 326;
  const int pillX[5] = {23, 113, 203, 293, 383};
  const char *pillLabel[5] = {"HOME", "COMP", "SOUND", "MUSIC", "AOD"};
  String pillValue[5] = {homeStyleName(homeStyle), String(""), String(soundVolume) + "%", musicMinimal ? "MIN" : "RICH", aodEnabled ? "ON" : "OFF"};
  for (int i = 0; i < 5; i++) {
    gfx->fillRoundRect(pillX[i], pillY, pillW, pillH, 16, 0x1082);
    gfx->drawRoundRect(pillX[i], pillY, pillW, pillH, 16, 0x4208);
    printCentered(pillLabel[i], pillX[i] + pillW / 2, pillY + 4, 1, 0x8C71);
    if (pillValue[i].length()) printCentered(pillValue[i], pillX[i] + pillW / 2, pillY + 14, 2, C_WHITE);
  }

  gfx->fillRoundRect(CX - 58, 372, 116, 30, 15, 0x1082);
  gfx->drawRoundRect(CX - 58, 372, 116, 30, 15, 0x4208);
  printCentered("BACK", CX, 379, 2, C_WHITE);
  needFlush = true;
}

bool settingsTap(int x, int y) {
  if (destSheetOpen) return false;
  if (settingsOpen) {
    if (screenAutoRot) {
      screenAutoRot = false;
      setScreenDeg(screenDeg, true);
      drawSettingsSheet();
      return true;
    }

    const int cardW = 160, cardH = 106, gapX = 12, gapY = 14;
    const int leftX = CX - cardW - gapX / 2;
    const int rightX = CX + gapX / 2;
    const int topY = 84;
    const int botY = topY + cardH + gapY;
    int card = -1;
    if (x >= leftX && x <= leftX + cardW) {
      if (y >= topY && y <= topY + cardH) card = 0;
      else if (y >= botY && y <= botY + cardH) card = 2;
    } else if (x >= rightX && x <= rightX + cardW) {
      if (y >= topY && y <= topY + cardH) card = 1;
      else if (y >= botY && y <= botY + cardH) card = 3;
    }

    if (card == 0) {
      brightPct = (brightPct == 25) ? 50 : (brightPct == 50) ? 75 : (brightPct == 75) ? 100 : 25;
      screenBright = (uint16_t)(((uint16_t)brightPct * 255 + 50) / 100);
      panel->setBrightness(screenBright);
      Preferences pr; pr.begin("motonav", false); pr.putInt("bright", brightPct); pr.end();
      drawSettingsSheet();
      return true;
    }
    if (card == 1) {
      settingsThemeIdx = themeCycleNext(settingsThemeIdx);
      Preferences pr; pr.begin("motonav", false); pr.putInt("theme", settingsThemeIdx); pr.end();
      pendingEvt = String("T,SET_THEME,") + String(settingsThemeIdx);
      drawSettingsSheet();
      return true;
    }
    if (card == 2) {
      int base = (screenDeg / 90) * 90;
      int d = (base + 90) % 360;
      if (d == 0) {
        screenAutoRot = true;
        autoRotAngle = (float)screenDeg;
        autoRotAt = 0;
      } else {
        setScreenDeg(d, true);
      }
      drawSettingsSheet();
      return true;
    }
    if (card == 3) {
      sleepMin = (sleepMin == 3) ? 5 : (sleepMin == 5) ? 10 : (sleepMin == 10) ? 0 : 3;
      gIdlePowerOffMs = sleepMin == 0 ? 0 : (uint32_t)sleepMin * 60000UL;
      Preferences pr; pr.begin("motonav", false); pr.putInt("sleep", sleepMin); pr.end();
      drawSettingsSheet();
      return true;
    }

    const int pillW = 80, pillH = 32, pillY = 326;
    const int pillX[5] = {23, 113, 203, 293, 383};
    for (int i = 0; i < 5; i++) {
      if (x >= pillX[i] && x <= pillX[i] + pillW && y >= pillY && y <= pillY + pillH) {
        if (i == 0) {
          homeStyle = (homeStyle == HOME_FULL) ? HOME_RIDER : HOME_FULL;
          Preferences pr; pr.begin("motonav", false); pr.putInt("home", (int)homeStyle); pr.end();
          settingsOpen = false;
          drawWaiting(waitSt);
        } else if (i == 1) {
          settingsOpen = false;
          companionEnter();
        } else if (i == 2) {
          const int levels[] = {0, 25, 50, 75, 100};
          int next = 0;
          for (int k = 0; k < 5; ++k) if (soundVolume == levels[k]) next = (k + 1) % 5;
          soundVolume = levels[next];
          audioSetVolume(soundVolume);
          Preferences pr; pr.begin("motonav", false); pr.putInt("sound", soundVolume); pr.end();
          drawSettingsSheet();
        } else if (i == 3) {
          musicMinimal = !musicMinimal;
          Preferences pr; pr.begin("motonav", false); pr.putBool("mmin", musicMinimal); pr.end();
          musicDirty = true;
          drawSettingsSheet();
        } else {
          aodEnabled = !aodEnabled;
          Preferences pr; pr.begin("motonav", false); pr.putBool("aod", aodEnabled); pr.end();
          if (!aodEnabled && screenAod) wakeFromAod();
          aodResetIdle();
          drawSettingsSheet();
        }
        return true;
      }
    }

    if (x >= CX - 58 && x <= CX + 58 && y >= 372 && y <= 402) {
      settingsOpen = false;
      drawWaiting(waitSt);
      return true;
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
