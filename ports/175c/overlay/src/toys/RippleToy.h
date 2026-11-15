#pragma once
// Water-ripple toy for the 1.75C.
//
// The-Badge's water toy is a 380-particle PBF fluid (app_water.c, 1705 lines)
// with a boat and a core-1 physics task. That whole engine is far more than a
// ripple needs, so this is a half-resolution height-field wave simulation:
// 233x233 cells, two int16 buffers (~216 KB PSRAM), rendered as 2x2 blocks
// straight into the Arduino_GFX canvas. Touch makes drops; a slow ambientdrop
// keeps the surface alive when nobody touches it.
//
// The wave kernel is the standard explicit scheme:
//   v += c*(neighbour average - h);  v *= damping;  h += v
// c = 0.25 and damping = 252/256 are stable for the 30 Hz update below.

#include <math.h>
#include <stdint.h>
#include <string.h>

#define RIPPLE_N 233
#define RIPPLE_CELLS (RIPPLE_N * RIPPLE_N)
#define RIPPLE_PX 466

static int16_t *rippleH = nullptr;
static int16_t *rippleV = nullptr;
static bool rippleReady = false;
static uint32_t rippleDropAt = 0;
static uint32_t rippleAmbientAt = 0;

static inline int rippleIndex(int x, int y) { return y * RIPPLE_N + x; }

static void rippleClear() {
  if (!rippleReady) return;
  memset(rippleH, 0, RIPPLE_CELLS * sizeof(int16_t));
  memset(rippleV, 0, RIPPLE_CELLS * sizeof(int16_t));
  rippleDropAt = 0;
  rippleAmbientAt = 0;
}

bool rippleBegin() {
  if (rippleReady) return true;
  rippleH = (int16_t *)ps_malloc(RIPPLE_CELLS * sizeof(int16_t));
  rippleV = (int16_t *)ps_malloc(RIPPLE_CELLS * sizeof(int16_t));
  if (!rippleH || !rippleV) {
    if (rippleH) { free(rippleH); rippleH = nullptr; }
    if (rippleV) { free(rippleV); rippleV = nullptr; }
    Serial.println("RIPPLE: PSRAM allocation failed");
    return false;
  }
  rippleReady = true;
  rippleClear();
  return true;
}

void rippleEnd() {
  if (rippleH) { free(rippleH); rippleH = nullptr; }
  if (rippleV) { free(rippleV); rippleV = nullptr; }
  rippleReady = false;
}

// canvas coordinates, 0..465
void rippleDrop(int cx, int cy, int amp) {
  if (!rippleReady) return;
  const int gx = cx >> 1;
  const int gy = cy >> 1;
  for (int dy = -7; dy <= 7; dy++) {
    int y = gy + dy;
    if (y < 1 || y >= RIPPLE_N - 1) continue;
    for (int dx = -7; dx <= 7; dx++) {
      int x = gx + dx;
      if (x < 1 || x >= RIPPLE_N - 1) continue;
      int d2 = dx * dx + dy * dy;
      if (d2 > 49) continue;
      int add = amp - (amp * d2) / 49;
      int i = rippleIndex(x, y);
      int h = rippleH[i] + add;
      if (h > 4000) h = 4000;
      if (h < -4000) h = -4000;
      rippleH[i] = (int16_t)h;
    }
  }
}

void rippleStep(float dt) {
  if (!rippleReady) return;
  const uint32_t now = millis();
  if (rippleAmbientAt == 0) rippleAmbientAt = now;
  if (now - rippleAmbientAt > 2400) {
    rippleAmbientAt = now;
    rippleDrop(60 + (int)(now % 340), 90 + (int)((now / 7) % 280), 900);
  }

  for (int y = 1; y < RIPPLE_N - 1; y++) {
    const int row = y * RIPPLE_N;
    for (int x = 1; x < RIPPLE_N - 1; x++) {
      const int i = row + x;
      const int lap = rippleH[i - 1] + rippleH[i + 1] + rippleH[i - RIPPLE_N] +
                      rippleH[i + RIPPLE_N] - 4 * rippleH[i];
      int v = rippleV[i] + (lap >> 2);
      v = (v * 252) >> 8;
      if (v > 3000) v = 3000;
      else if (v < -3000) v = -3000;
      rippleV[i] = (int16_t)v;
      int h = rippleH[i] + (v >> 2);
      if (h > 4000) h = 4000;
      else if (h < -4000) h = -4000;
      rippleH[i] = (int16_t)h;
    }
  }
}

void rippleDraw() {
  if (!rippleReady) {
    gfx->fillScreen(0);
    printCentered("NO PSRAM", CX, CX - 12, 3, 0xF800);
    return;
  }
  gfx->fillScreen(0);
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  const int c = RIPPLE_N / 2;
  const int r2 = 111 * 111;
  for (int y = 1; y < RIPPLE_N - 1; y++) {
    const int dy = y - c;
    const int sy = y * 2;
    for (int x = 1; x < RIPPLE_N - 1; x++) {
      const int dx = x - c;
      if (dx * dx + dy * dy > r2) continue;
      const int i = y * RIPPLE_N + x;
      const int h = rippleH[i];
      const int gx = rippleH[i + 1] - rippleH[i - 1];
      const int gy = rippleH[i + RIPPLE_N] - rippleH[i - RIPPLE_N];
      int lit = (h >> 3) + ((gx + gy) >> 6);
      if (lit > 255) lit = 255;
      else if (lit < 0) lit = 0;
      int r = 1 + (lit >> 5);
      int g = 6 + (lit >> 3);
      int b = 14 + (lit >> 2);
      if (r > 31) r = 31;
      if (g > 63) g = 63;
      if (b > 31) b = 31;
      const uint16_t col = (uint16_t)((r << 11) | (g << 5) | b);
      const int sx = x * 2;
      uint16_t *p0 = fb + sy * RIPPLE_PX + sx;
      p0[0] = col;
      p0[1] = col;
      uint16_t *p1 = p0 + RIPPLE_PX;
      p1[0] = col;
      p1[1] = col;
    }
  }
}
