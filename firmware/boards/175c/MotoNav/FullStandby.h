#pragma once
// StopWatch-style full standby dashboard (VICE skin).
// This is a second home renderer; the rider home and all of its state stay in
// HomePage.h. Only the drawing changes, so the drawer/settings/lean/HomeClock
// logic is shared by both skins.

#include "FullStandbyUi.h"

#define FULL_STAR_Y 316
// The standby background follows the map style chosen on the phone (all 12
// themes, day/night). Palette data lives in src/map/MapState.h.

static int64_t fullDrawnMinute = -2;

static const char *fullMonthName(int m) {
  static const char *const months[] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
  };
  return (m >= 1 && m <= 12) ? months[m - 1] : "---";
}

static const char *fullWeekdayName(int w) {
  static const char *const days[] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
  };
  return (w >= 0 && w <= 6) ? days[w] : "---";
}

// 0 = Sunday.
static int fullWeekday(int y, int m, int d) {
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) --y;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

// Adapted from the StopWatch themed standby map, but keyed by the 1.75C
// settings THEME row instead of the navigation map state.
static void drawFullBackground() {
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  const int W = LCD_WIDTH;
  const int H = LCD_HEIGHT;
  const int tid = styleIdx();
  const MapTheme &th = nightOn ? THEMES_NIGHT[tid] : THEMES[tid];

  gfx->fillScreen(th.block);

  if (tid == ST_DRAGON) DragonRadar::drawGrid(gfx, W, H, nightOn);
  if (tid == ST_RADAR) {  // RADAR
    gfx->drawCircle(CX, CX, 75, th.road);
    gfx->drawCircle(CX, CX, 150, th.road);
    gfx->drawCircle(CX, CX, 210, th.road);
  }

  if (th.water != 0 && th.water != th.block) {
    for (int y = 0; y < H; y++) {
      float ny = (float)y / 466.0f;
      int wx = (int)(78 + 32.0f * sinf(ny * 3.14159f * 1.5f));
      int x0 = max(0, wx - 22);
      int x1 = min(W - 1, wx + 22);
      for (int x = x0; x <= x1; x++) fb[y * W + x] = th.water;
    }
  }

  if (th.park != 0 && th.park != th.block && th.park != th.water) {
    gfx->fillRect(140, 70, 52, 44, th.park);
    gfx->fillRect(276, 268, 64, 52, th.park);
    gfx->fillRect(160, 360, 48, 38, th.park);
  }

  if (th.road != 0 && th.road != th.block) {
    for (int x = 42; x < W; x += 48) gfx->fillRect(x - 2, 0, 4, H, th.road);
    for (int y = 42; y < H; y += 48) gfx->fillRect(0, y - 2, W, 4, th.road);
  }

  gfx->fillRect(CX - 1, 280, 3, 140, th.done);

  if (th.route != 0) {
    gfx->fillRect(CX - 3, 186, 6, 94, th.route);
    gfx->fillRect(CX - 3, 183, 148, 6, th.route);
    aaDisc((float)CX, 186.0f, 6.0f, th.route2);
    aaDisc((float)CX, 186.0f, 3.0f, th.route);
  }

  const int blipX = CX, blipY = 276;
  gfx->fillTriangle(blipX, blipY - 14, blipX - 8, blipY + 6, blipX + 8, blipY + 6, C_BLACK);
  gfx->fillTriangle(blipX, blipY - 11, blipX - 6, blipY + 4, blipX + 6, blipY + 4, th.blip);
  gfx->fillTriangle(blipX, blipY - 1, blipX - 4, blipY + 4, blipX + 4, blipY + 4, C_BLACK);

  // Darken and add the round-screen vignette, same as the original standby.
  uint32_t r_b = (th.block >> 11) & 0x1F;
  uint32_t g_b = (th.block >> 5) & 0x3F;
  uint32_t b_b = th.block & 0x1F;
  uint32_t lum = r_b * 8 + g_b * 4 + b_b * 8;
  uint32_t baseRatio = (lum > 180) ? 118 : 205;

  for (int y = 0; y < H; y++) {
    int dy = y - H / 2;
    uint16_t *row = &fb[y * W];
    for (int x = 0; x < W; x++) {
      int dx = x - W / 2;
      int r2 = dx * dx + dy * dy;
      uint32_t ratio = baseRatio;
      if (r2 > 175 * 175) {
        int dist = (int)sqrtf((float)r2) - 175;
        ratio = (uint32_t)max(0, (int)baseRatio - dist * 3);
      }
      uint16_t c = row[x];
      uint32_t r = ((c >> 11) & 0x1F) * ratio >> 8;
      uint32_t g = ((c >> 5) & 0x3F) * ratio >> 8;
      uint32_t b = (c & 0x1F) * ratio >> 8;
      row[x] = (uint16_t)((r << 11) | (g << 5) | b);
    }
  }
}

static void drawFullReady(int st) {
  if (st == WAIT_READY || (bleConnected && bleAuthed)) {
    aaDisc((float)(CX - 38), 58.0f, 4.5f, 0x760D);
    drawAaString(CX + 6, 58, "READY", 1.8f, 1.05f, 11.5f, 0x760D, true);
  } else if (st == WAIT_NOLINK || !bleConnected) {
    aaDisc((float)(CX - 42), 58.0f, 4.5f, C_GREY);
    drawAaString(CX + 6, 58, "NO LINK", 1.8f, 1.05f, 11.5f, C_GREY, true);
  } else {
    aaDisc((float)(CX - 48), 58.0f, 4.5f, C_RED);
    drawAaString(CX + 6, 58, "LINK LOST", 1.8f, 1.05f, 11.5f, C_RED, true);
  }
}

static void drawFullClock() {
  HomeClock::Reading r;
  bool valid = homeClock.read(millis(), r);
  char clockBuf[8];
  char dateBuf[24];
  if (valid) {
    snprintf(clockBuf, sizeof(clockBuf), "%02d:%02d", r.hour, r.minute);
    int wd = fullWeekday(r.year, r.month, r.day);
    snprintf(dateBuf, sizeof(dateBuf), "%s %02d  %s",
             fullMonthName(r.month), r.day, fullWeekdayName(wd));
  } else {
    strcpy(clockBuf, "--:--");
    strcpy(dateBuf, "--- --  ---");
  }

  drawTranslucentRoundRect(CX - 120, 136, 240, 90, 18, 0x0000, 200);
  gfx->drawRoundRect(CX - 120, 136, 240, 90, 18, 0x39E7);
  drawAaString(CX, 168, clockBuf, 4.0f, 2.3f, 30.0f, C_WHITE, true);
  drawAaString(CX, 206, dateBuf, 1.8f, 1.05f, 12.0f, 0xAFBD, true);
}

static void drawFullStar() {
  drawTranslucentCircle(CX, FULL_STAR_Y, 32, 0x0000, 180);
  gfx->drawCircle(CX, FULL_STAR_Y, 32, 0x4A69);
  gfx->drawCircle(CX, FULL_STAR_Y, 33, 0x2124);
  drawRockstarStar(CX, FULL_STAR_Y, 18.0f, 7.5f, C_WHITE, 0xFDE0);
  drawAaString(CX, FULL_STAR_Y + 44, "DESTINATIONS", 1.4f, 0.85f, 9.5f, 0x7BEF, true);
}

void drawWaitingFull(int st) {
  static bool logged = false;
  if (!logged) { logged = true; Serial.println("HOME full standby"); }
  rtcClockInit();
  waitSt = st;
  if (sliding) return;
  if (page == 2) { onWaitScreen = false; musicDirty = true; return; }
  if (page == 1) { onWaitScreen = false; wxDirty = true; return; }

  onWaitScreen = true;
  destStarY = FULL_STAR_Y;

  drawFullBackground();
  drawBattery(CX - 28, 20, true);
  drawFullReady(st);
  drawFullClock();
  drawFullStar();

  if (!inNav) {
    drawSettingsGear();
    drawLeanButton();
    toyButtonDraw();
  }
  drawPageDots();

  if (destSheetOpen) drawDestSheet();
  if (settingsOpen) drawSettingsSheet();
  if (leanOpen) drawLeanSheet();

  fullDrawnMinute = homeClock.minuteKey(millis());
  needFlush = true;
}

void fullHomeTick() {
  if (screenDark) return;
  if (!onWaitScreen || sliding || page != 0) return;
  if (destSheetOpen || settingsOpen || leanOpen) return;

  static int lastPct = -2;
  static bool lastChg = false;
  if (batPct != lastPct || batChg != lastChg) {
    lastPct = batPct;
    lastChg = batChg;
    drawBattery(CX - 28, 20, true);
    needFlush = true;
  }

  int64_t minute = homeClock.minuteKey(millis());
  static int lastTheme = -1;
  static bool lastNight = false;
  if (styleIdx() != lastTheme || nightOn != lastNight) {
    lastTheme = styleIdx();
    lastNight = nightOn;
    drawWaitingFull(waitSt);
    return;
  }
  if (minute == fullDrawnMinute) return;
  drawWaitingFull(waitSt);
}
