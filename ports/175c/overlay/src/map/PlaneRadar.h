#pragma once
// ADS-B aircraft radar page for the 1.75C.
//
// The device has no network and no absolute lat/lon (the nav protocol only
// carries local metres), so the phone does the geo work: it polls adsb.fi or
// adsb.lol, converts every aircraft to dx/dy metres relative to the bike, and
// sends one bulk type 5 frame. This file only parses that frame and draws.
//
// Memory: fixed arrays only. 30 aircraft * 20 B = 600 B static RAM, one 768 B
// PSRAM bulk buffer, no per-frame allocation beyond the small text Strings
// used by printCentered().

#include <math.h>
#include <stdio.h>

#define PLANE_MAX 30

struct Plane {
  int16_t dx;         // metres east of the bike
  int16_t dy;         // metres north
  int16_t altFt;      // 0x7FFF unknown
  uint16_t trackCdeg; // 0.01 deg clockwise from north, 0xFFFF unknown
  uint16_t gsKt10;    // 0.1 kt, 0xFFFF unknown
  uint16_t squawk;    // 0xFFFF unknown
  uint8_t flags;      // bit0 ground, bit1 emergency, bit2 military
  char callsign[7];
};

static Plane planes[PLANE_MAX];
static uint8_t planeCount = 0;
static uint8_t planeSource = 0;      // 0 adsb.fi, 1 adsb.lol
static uint16_t planeRangeM = 20000;
static uint32_t planeAt = 0;
static bool planeValid = false;
static int8_t planeSel = -1;

static constexpr uint16_t planeRgb(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3));
}

static uint16_t planeAltColor(int16_t altFt, uint8_t flags) {
  if (flags & 1) return planeRgb(0x88, 0x88, 0x88);       // ground
  if (altFt == 0x7FFF) return planeRgb(0x6A, 0x74, 0x86); // unknown
  if (altFt < 3000) return planeRgb(0xFF, 0x5A, 0x3C);
  if (altFt < 10000) return planeRgb(0xFF, 0xB2, 0x3C);
  if (altFt < 20000) return planeRgb(0xC8, 0xFF, 0x3C);
  if (altFt < 30000) return planeRgb(0x39, 0xFF, 0x14);
  return planeRgb(0x3C, 0xE0, 0xFF);
}

// Header: ver u8, count u8, source u8, reserved u8, range_m u16.
// Aircraft: dx i16, dy i16, alt i16, track u16, gs u16, squawk u16,
//           flags u8, reserved u8, callsign[6]  = 20 B.
bool parsePlanes(const uint8_t *d, int n) {
  if (n < 6) return false;
  const uint8_t ver = d[0];
  const uint8_t cnt = d[1];
  const uint8_t src = d[2];
  const uint16_t range = (uint16_t)rd16(d + 4);
  if (ver != 1 || cnt > PLANE_MAX || range < 1000 || range > 100000) return false;
  if (n < 6 + (int)cnt * 20) return false;

  for (int i = 0; i < cnt; i++) {
    const uint8_t *p = d + 6 + i * 20;
    Plane &q = planes[i];
    q.dx = rd16(p);
    q.dy = rd16(p + 2);
    q.altFt = rd16(p + 4);
    q.trackCdeg = (uint16_t)rd16(p + 6);
    q.gsKt10 = (uint16_t)rd16(p + 8);
    q.squawk = (uint16_t)rd16(p + 10);
    q.flags = p[12];
    for (int k = 0; k < 6; k++) q.callsign[k] = (char)p[14 + k];
    q.callsign[6] = 0;
  }
  planeCount = cnt;
  planeSource = src;
  planeRangeM = range;
  planeAt = millis();
  planeValid = true;
  planeSel = -1;
  return true;
}

void planeEnter() {
  planeCount = 0;
  planeValid = false;
  planeSel = -1;
  planeAt = 0;
  pendingEvt = "T,AC,1";
}

void planeExit() {
  planeSel = -1;
  pendingEvt = "T,AC,0";
}

static void planeCycleRange() {
  if (planeRangeM < 15000) planeRangeM = 20000;
  else if (planeRangeM < 35000) planeRangeM = 50000;
  else planeRangeM = 10000;
  pendingEvt = String("T,AC,R,") + String(planeRangeM);
}

void planeStep() {
  if (planeValid && (uint32_t)(millis() - planeAt) > 60000) {
    planeValid = false;
    planeCount = 0;
    planeSel = -1;
  }
  if (planeSel >= planeCount) planeSel = -1;
}

static void planeIcon(int x, int y, float trackDeg, uint16_t col) {
  const float t = trackDeg * PI / 180.0f;
  const float c = cosf(t), s = sinf(t);
  static const float LX[4] = {0, 7, 0, -7};
  static const float LY[4] = {-11, 5, 8, 5};
  int px[4], py[4];
  for (int i = 0; i < 4; i++) {
    px[i] = x + (int)(LX[i] * c - LY[i] * s);
    py[i] = y + (int)(LX[i] * s + LY[i] * c);
  }
  gfx->fillTriangle(px[0], py[0], px[1], py[1], px[2], py[2], col);
  gfx->fillTriangle(px[0], py[0], px[2], py[2], px[3], py[3], col);
}

static void planeEdge(int x, int y, uint16_t col) {
  gfx->fillCircle(x, y, 3, col);
  gfx->drawCircle(x, y, 6, col);
}

static void planeScreenPos(const Plane &p, int &x, int &y, bool &inRange) {
  float mx = (float)p.dx, my = (float)p.dy;
  // Dead-reckon up to 2 s past the last frame from ground speed and track, so
  // the icon glides between the phone's 2 s updates instead of jumping.
  if (planeValid && p.gsKt10 != 0xFFFF && p.trackCdeg != 0xFFFF) {
    float t = (uint32_t)(millis() - planeAt) / 1000.0f;
    if (t > 2.0f) t = 2.0f;
    const float v = (p.gsKt10 / 10.0f) * 0.514444f;   // kt -> m/s
    const float a = (p.trackCdeg / 100.0f) * PI / 180.0f;
    mx += v * t * sinf(a);
    my += v * t * cosf(a);
  }
  const float scale = 210.0f / (float)planeRangeM;
  const float d2 = mx * mx + my * my;
  const float r2 = (float)planeRangeM * (float)planeRangeM;
  inRange = d2 <= r2;
  if (inRange || d2 < 1.0f) {
    x = CX + (int)(mx * scale);
    y = CX - (int)(my * scale);
  } else {
    const float len = sqrtf(d2);
    x = CX + (int)(mx / len * 210.0f);
    y = CX - (int)(my / len * 210.0f);
  }
}

void planeRadarDraw() {
  gfx->fillScreen(0);
  const int R = 210;
  for (int i = 1; i <= 4; i++) {
    gfx->drawCircle(CX, CX, R * i / 4, i == 4 ? 0x2E4A5A : 0x1A2A34);
  }
  gfx->drawLine(CX - R, CX, CX + R, CX, 0x1A2A34);
  gfx->drawLine(CX, CX - R, CX, CX + R, 0x1A2A34);
  printCentered("N", CX, 18, 1, 0x7A8A96);
  printCentered("E", 440, 229, 1, 0x7A8A96);
  printCentered("W", 20, 229, 1, 0x7A8A96);

  char head[24];
  snprintf(head, sizeof(head), "ADS-B %uKM", (unsigned)(planeRangeM / 1000));
  printCentered(head, CX, 34, 2, 0xE8F4FF);

  const char *src = planeSource == 0 ? "ADSB.FI" : planeSource == 1 ? "ADSB.LOL" : "----";
  printCentered(src, 150, 56, 1, 0x4A6A7A);
  if (planeValid) {
    char age[12];
    const uint32_t s = (uint32_t)(millis() - planeAt) / 1000;
    snprintf(age, sizeof(age), s > 15 ? "STALE" : "%us", (unsigned)s);
    printCentered(age, 316, 56, 1, s > 15 ? 0xFB65 : 0x4A6A7A);
  }

  int labelBudget = 14;
  for (int i = 0; i < planeCount; i++) {
    const Plane &p = planes[i];
    int x, y;
    bool inRange;
    planeScreenPos(p, x, y, inRange);
    const uint16_t col = planeAltColor(p.altFt, p.flags);
    if (!inRange) {
      planeEdge(x, y, col);
      continue;
    }
    const float track = p.trackCdeg == 0xFFFF ? 0.0f : p.trackCdeg / 100.0f;
    planeIcon(x, y, track, col);
    if (p.flags & 2) {
      gfx->drawCircle(x, y, 14, 0xF800);
      gfx->drawCircle(x, y, 13, 0xF800);
    }
    if (i == planeSel) gfx->drawCircle(x, y, 17, 0xFFFFFF);
    if (labelBudget > 0 && p.callsign[0]) {
      labelBudget--;
      printCentered(p.callsign, x + 8, y - 8, 1, col);
      if (p.altFt != 0x7FFF) {
        char alt[12];
        snprintf(alt, sizeof(alt), "%d", (int)p.altFt);
        printCentered(alt, x + 8, y + 2, 1, 0x5A6A76);
      }
    }
  }

  gfx->fillTriangle(CX, CX - 10, CX - 6, CX + 7, CX + 6, CX + 7, 0xFFFFFF);
  gfx->fillCircle(CX, CX + 4, 2, 0xFFFFFF);

  if (!planeValid || planeCount == 0) {
    printCentered("NO DATA", CX, CX - 10, 3, 0x6A7486);
    printCentered("PHONE RADAR OFF", CX, CX + 20, 1, 0x4A5A66);
  }

  if (planeSel >= 0 && planeSel < planeCount) {
    const Plane &p = planes[planeSel];
    gfx->fillRoundRect(50, 292, 366, 104, 14, 0x0C141C);
    gfx->drawRoundRect(50, 292, 366, 104, 14, 0x2E4A5A);
    printCentered(p.callsign[0] ? p.callsign : "----", CX, 300, 3, 0xFFFFFF);
    char line[48];
    if (p.altFt == 0x7FFF) snprintf(line, sizeof(line), "ALT ----  GS ----");
    else if (p.gsKt10 == 0xFFFF) snprintf(line, sizeof(line), "ALT %dFT  GS ----", (int)p.altFt);
    else snprintf(line, sizeof(line), "ALT %dFT  GS %uKT", (int)p.altFt, (unsigned)(p.gsKt10 / 10));
    printCentered(line, CX, 332, 1, planeAltColor(p.altFt, p.flags));
    const float distKm = sqrtf((float)p.dx * p.dx + (float)p.dy * p.dy) / 1000.0f;
    if (p.trackCdeg == 0xFFFF) snprintf(line, sizeof(line), "TRK ---  D %.1fKM", distKm);
    else snprintf(line, sizeof(line), "TRK %03u  D %.1fKM", (unsigned)(p.trackCdeg / 100), distKm);
    printCentered(line, CX, 348, 1, 0x8A9AA6);
    if (p.squawk != 0xFFFF) snprintf(line, sizeof(line), "SQ %04u%s", (unsigned)p.squawk, (p.flags & 4) ? "  MIL" : "");
    else snprintf(line, sizeof(line), "SQ ----%s", (p.flags & 4) ? "  MIL" : "");
    printCentered(line, CX, 364, 1, (p.flags & 2) ? 0xFAC7 : 0x5A6A76);
    printCentered("TAP TO CLOSE", CX, 380, 1, 0x3A4A56);
  }
}

// Returns true when the tap was consumed by the radar page.
bool planeRadarTap(int x, int y) {
  if (abs(y - 46) <= 30 && abs(x - CX) <= 110) {
    planeCycleRange();
    return true;
  }
  if (planeSel >= 0 && y >= 292 && y <= 396) {
    planeSel = -1;
    return true;
  }
  int best = -1;
  float bestD = 24.0f * 24.0f;
  for (int i = 0; i < planeCount; i++) {
    int px, py;
    bool inRange;
    planeScreenPos(planes[i], px, py, inRange);
    const float dx = (float)(x - px), dy = (float)(y - py);
    const float d2 = dx * dx + dy * dy;
    if (d2 < bestD) {
      bestD = d2;
      best = i;
    }
  }
  if (best >= 0) {
    planeSel = (best == planeSel) ? -1 : best;
    return true;
  }
  planeSel = -1;
  return false;
}

// Boot-test fixture: twelve aircraft around the bike, no phone needed.
void planeFillTest() {
  static const char *NAMES[12] = {
    "CCA123", "CES456", "CSN789", "CHH111", "CSC222", "CQH333",
    "JAL444", "ANA555", "KAL666", "SIA777", "UAE888", "BAW999"};
  planeCount = 12;
  planeRangeM = 20000;
  planeSource = 0;
  planeAt = millis();
  planeValid = true;
  planeSel = -1;
  for (int i = 0; i < 12; i++) {
    Plane &p = planes[i];
    p.dx = (int16_t)((i - 6) * 2800);
    p.dy = (int16_t)(((i % 3) - 1) * 6000 + 2000);
    p.altFt = (int16_t)(2500 + i * 3200);
    p.trackCdeg = (uint16_t)((i * 30) * 100);
    p.gsKt10 = (uint16_t)(3800 + i * 120);
    p.squawk = (uint16_t)(i == 4 ? 7700 : 2000 + i);
    p.flags = (uint8_t)(i == 4 ? 2 : (i == 7 ? 4 : 0));
    for (int k = 0; k < 6; k++) p.callsign[k] = NAMES[i][k];
    p.callsign[6] = 0;
  }
}
