#pragma once
// Toy mode for the 1.75C: four 2D games plus a water-ripple surface.
//
// Ported from curisama/The-Badge (MIT). The-Badge draws these with LVGL
// widgets on the same board; here the same rules and physics are kept and the
// drawing is done with Arduino_GFX primitives, which the rest of this firmware
// already uses. No LVGL is involved.
//
// Entry: the TOY button on either standby skin. Touch is routed here from
// touchPoll() while this overlay is open, and the main loop gives it the frame.

#include "src/toys/RippleToy.h"
#include "../audio/RecorderToy.h"
#include "src/toys/PetToy.h"

// Defined in src/map/PlaneRadar.h, which MotoNav.ino includes after this file.
void planeEnter();
void planeExit();
void planeStep();
void planeRadarDraw();
bool planeRadarTap(int x, int y);
void planeFillTest();

enum ToyKind : uint8_t {
  TOY_MENU = 0,
  TOY_BRICKS,
  TOY_PINBALL,
  TOY_MARBLE,
  TOY_POP,
  TOY_WATER,
  TOY_RADAR,
  TOY_REC,
  TOY_PET
};

struct ToyItem {
  const char *name;
  uint16_t color;
  ToyKind kind;
};

static constexpr uint16_t toyRgb(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3));
}

static const uint16_t TC_PANEL   = toyRgb(0x1A, 0x1A, 0x22);
static const uint16_t TC_EDGE    = toyRgb(0x5A, 0x5A, 0x66);
static const uint16_t TC_TEXT    = toyRgb(0x8A, 0x8A, 0x96);
static const uint16_t TC_DIM     = toyRgb(0x4A, 0x4A, 0x56);
static const uint16_t TC_WHITE   = 0xFFFF;
static const uint16_t TC_BRICK   = toyRgb(0x2E, 0x6E, 0x5A);
static const uint16_t TC_BRICK_E = toyRgb(0x5F, 0xD3, 0xA8);
static const uint16_t TC_PINK    = toyRgb(0x6E, 0x2E, 0x4A);
static const uint16_t TC_PINK_E  = toyRgb(0xE0, 0x7A, 0x9A);
static const uint16_t TC_PINK_H  = toyRgb(0xFF, 0xAF, 0xC6);
static const uint16_t TC_GOLD    = toyRgb(0xFF, 0xD2, 0x4A);
static const uint16_t TC_FLIP    = toyRgb(0xD8, 0xD8, 0xE0);
static const uint16_t TC_BUBBLE  = toyRgb(0x53, 0x70, 0x9A);
static const uint16_t TC_BUBBLE2 = toyRgb(0x8F, 0xB0, 0xD6);
static const uint16_t TC_BUBBLE3 = toyRgb(0xD8, 0xE6, 0xF5);
static const uint16_t TC_POPPED  = toyRgb(0x23, 0x26, 0x2E);
static const uint16_t TC_HOLE    = toyRgb(0x3A, 0x3A, 0x44);
static const uint16_t TC_HOLE2   = toyRgb(0x0A, 0x0A, 0x0E);
static const uint16_t TC_COUNT   = toyRgb(0x6A, 0x74, 0x86);
static const uint16_t TC_RING    = toyRgb(0x2A, 0x2A, 0x32);
static const uint16_t TC_BALL_H  = toyRgb(0xBF, 0xE8, 0xFF);
static const uint16_t TC_MARBLE  = toyRgb(0x6E, 0x5A, 0x2E);
static const uint16_t TC_POPC    = toyRgb(0x4A, 0x3A, 0x6E);
static const uint16_t TC_WATER   = toyRgb(0x1E, 0x5A, 0x7A);
static const uint16_t TC_RADAR   = toyRgb(0x1E, 0x4A, 0x7A);
static const uint16_t TC_REC     = toyRgb(0x6A, 0x2A, 0x2A);
static const uint16_t TC_PET     = toyRgb(0x2A, 0x5A, 0x3A);

static const ToyItem TOY_ITEMS[8] = {
  {"BRICKS",  TC_BRICK, TOY_BRICKS},
  {"PINBALL", TC_PINK, TOY_PINBALL},
  {"MARBLE",  TC_MARBLE, TOY_MARBLE},
  {"POP",     TC_POPC, TOY_POP},
  {"WATER",   TC_WATER, TOY_WATER},
  {"RADAR",   TC_RADAR, TOY_RADAR},
  {"REC",     TC_REC, TOY_REC},
  {"PET",     TC_PET, TOY_PET},
};

static const int TOY_ROW_Y[8] = {76, 116, 156, 196, 236, 276, 316, 356};

static bool toyOpen = false;
static ToyKind toyKind = TOY_MENU;
static bool toyDirty = true;
static uint32_t toyLastMs = 0;
static float toyRight0 = 0, toyDown0 = 0;
// The touch that opens the overlay (or picks a menu row) is still down when
// the next screen appears. Ignore input until that finger lifts, otherwise
// the TOY button at (120,350) lands on the WATER row and starts it at once.
static bool toyWaitRelease = false;

bool toyIsOpen() { return toyOpen; }

// ── shared drawing ───────────────────────────────────────────
static void toyText(const char *s, int x, int y, int size, uint16_t c) {
  printCentered(String(s), x, y, size, c);
}

static void toyThick(float x0, float y0, float x1, float y1, float w, uint16_t c) {
  float dx = x1 - x0, dy = y1 - y0;
  float len = sqrtf(dx * dx + dy * dy);
  if (len < 0.01f) {
    gfx->fillCircle((int)x0, (int)y0, (int)(w * 0.5f), c);
    return;
  }
  float nx = -dy / len * w * 0.5f, ny = dx / len * w * 0.5f;
  gfx->fillTriangle(x0 + nx, y0 + ny, x0 - nx, y0 - ny, x1 + nx, y1 + ny, c);
  gfx->fillTriangle(x0 - nx, y0 - ny, x1 - nx, y1 - ny, x1 + nx, y1 + ny, c);
  gfx->fillCircle((int)x0, (int)y0, (int)(w * 0.5f), c);
  gfx->fillCircle((int)x1, (int)y1, (int)(w * 0.5f), c);
}

static void toyArc(float cx, float cy, float r, float a0, float a1, float w, uint16_t c) {
  float span = a1 - a0;
  int steps = (int)(fabsf(span) / 0.12f) + 2;
  float px = cx + cosf(a0) * r, py = cy + sinf(a0) * r;
  for (int i = 1; i <= steps; i++) {
    float a = a0 + span * (float)i / (float)steps;
    float x = cx + cosf(a) * r, y = cy + sinf(a) * r;
    toyThick(px, py, x, y, w, c);
    px = x; py = y;
  }
}

// The round panel only shows about r=222 around (233,233). The old corner
// position (410,54) is 252 px out and sits under the bezel, so the X was
// invisible. Bottom centre (233,436) is 203 px out and safely visible.
static void toyBackDraw() {
  gfx->fillCircle(233, 436, 18, TC_PANEL);
  gfx->drawCircle(233, 436, 18, TC_EDGE);
  gfx->drawLine(226, 429, 240, 443, TC_FLIP);
  gfx->drawLine(226, 443, 240, 429, TC_FLIP);
}

static bool toyBackHit(int x, int y) {
  int dx = x - 233, dy = y - 436;
  return dx * dx + dy * dy <= 32 * 32;
}

// Screen rotation only, without the touch-panel mirror flips.
static void toyRotate(float &dx, float &dy) {
  float ox = dx, oy = dy;
  if (rotFree) { dx = ox * rotCosF + oy * rotSinF; dy = -ox * rotSinF + oy * rotCosF; }
  else if (screenRot == 1) { dx = oy; dy = -ox; }
  else if (screenRot == 2) { dx = -ox; dy = -oy; }
  else if (screenRot == 3) { dx = -oy; dy = ox; }
}

// right/down in gravity units (1.0 = one g), already canvas-oriented.
static bool toyReadTilt(float &right, float &down) {
  if (!imuOK) return false;
  AccelerometerData a;
  if (!imu.readAccel(a)) return false;
  float ax = a.mps2.x / 9.80665f;
  float ay = a.mps2.y / 9.80665f;
  // The QMI8658 sits rotated 90 degrees from the screen: sensor Y is the
  // screen's horizontal axis and sensor X is vertical.
  // Measured on the 1.75C: tilting right makes +ay, so screen-right is +ay.
  // (The-Badge's -gy mapping did not carry over to this driver's sign.)
  float rx = ay, ry = -ax;
  toyRotate(rx, ry);
  right = rx;
  down = ry;
  return true;
}

// ── menu ─────────────────────────────────────────────────────
static void toyMenuDraw() {
  gfx->fillScreen(0);
  toyText("TOYS", CX, 18, 4, TC_TEXT);
  for (int i = 0; i < 8; i++) {
    gfx->fillRoundRect(CX - 130, TOY_ROW_Y[i] - 18, 260, 36, 18, TOY_ITEMS[i].color);
    toyText(TOY_ITEMS[i].name, CX, TOY_ROW_Y[i] - 12, 3, TC_WHITE);
  }
  toyBackDraw();
}

static bool toyMenuHit(int x, int y, int &idx) {
  if (x < CX - 130 || x > CX + 130) return false;
  for (int i = 0; i < 8; i++) {
    if (abs(y - TOY_ROW_Y[i]) <= 19) { idx = i; return true; }
  }
  return false;
}

// ── bubble wrap ──────────────────────────────────────────────
#define POP_MAX 25
static int popN = 0, popLeft = 0;
static int16_t popX[POP_MAX], popY[POP_MAX];
static bool popPopped[POP_MAX];
static bool popDirty = true;
static uint32_t popRefillAt = 0;

static void popLayout() {
  popN = 0; popLeft = 0;
  const int step = 70;
  for (int row = -2; row <= 2 && popN < POP_MAX; row++) {
    int y = CX + row * step;
    int off = (row & 1) ? step / 2 : 0;
    for (int col = -2; col <= 2 && popN < POP_MAX; col++) {
      int x = CX + col * step + off;
      int dx = x - CX, dy = y - CX;
      if (dx * dx + dy * dy > 200 * 200) continue;
      popX[popN] = (int16_t)x;
      popY[popN] = (int16_t)y;
      popPopped[popN] = false;
      popN++;
      popLeft++;
    }
  }
}

static void popStart() {
  popLayout();
  popRefillAt = 0;
  popDirty = true;
}

static void popTap(int x, int y) {
  for (int i = 0; i < popN; i++) {
    if (popPopped[i]) continue;
    int dx = x - popX[i], dy = y - popY[i];
    if (dx * dx + dy * dy <= 34 * 34) {
      popPopped[i] = true;
      popLeft--;
      popDirty = true;
      if (popLeft == 0) popRefillAt = millis() + 900;
      return;
    }
  }
}

static void popStep() {
  if (popRefillAt && millis() >= popRefillAt) {
    popRefillAt = 0;
    popLayout();
    popDirty = true;
  }
}

static void popDraw() {
  gfx->fillScreen(0);
  for (int i = 0; i < popN; i++) {
    int x = popX[i], y = popY[i];
    if (popPopped[i]) {
      gfx->fillCircle(x, y, 20, TC_POPPED);
    } else {
      gfx->fillCircle(x, y, 32, TC_BUBBLE);
      gfx->fillCircle(x, y, 26, TC_BUBBLE2);
      gfx->fillCircle(x - 9, y - 10, 9, TC_BUBBLE3);
    }
  }
  toyText(String(popLeft).c_str(), CX, 392, 3, TC_COUNT);
  toyBackDraw();
}

// ── marble ───────────────────────────────────────────────────
static float mzX = 0, mzY = 0, mzVX = 0, mzVY = 0;
static float mzHoleX = 0, mzHoleY = 0;
static int mzScore = 0;

static void mzNewHole() {
  float a = (float)random(360) * PI / 180.0f;
  float r = 60.0f + (float)random(110);
  mzHoleX = CX + cosf(a) * r;
  mzHoleY = CX + sinf(a) * r;
}

static void marbleStart() {
  mzX = CX; mzY = CX; mzVX = mzVY = 0;
  mzScore = 0;
  mzNewHole();
  toyReadTilt(toyRight0, toyDown0);
}

static void marbleStep(float dt) {
  float r = 0, d = 0;
  if (toyReadTilt(r, d)) { r -= toyRight0; d -= toyDown0; }
  mzVX += r * 320.0f * dt;
  mzVY += d * 320.0f * dt;
  float f = expf(-2.2f * dt);
  mzVX *= f; mzVY *= f;
  float sp = sqrtf(mzVX * mzVX + mzVY * mzVY);
  if (sp > 420.0f) { mzVX = mzVX / sp * 420.0f; mzVY = mzVY / sp * 420.0f; }
  mzX += mzVX * dt;
  mzY += mzVY * dt;
  float dx = mzX - CX, dy = mzY - CX;
  float rr = sqrtf(dx * dx + dy * dy);
  if (rr > 202.0f) {
    float nx = dx / rr, ny = dy / rr;
    float dp = mzVX * nx + mzVY * ny;
    mzVX = (mzVX - 2.0f * dp * nx) * 0.45f;
    mzVY = (mzVY - 2.0f * dp * ny) * 0.45f;
    mzX = CX + nx * 201.0f;
    mzY = CX + ny * 201.0f;
  }
  float hx = mzX - mzHoleX, hy = mzY - mzHoleY;
  if (hx * hx + hy * hy < 30 * 30) {
    mzScore++;
    mzVX = mzVY = 0;
    mzNewHole();
  }
}

static void marbleDraw() {
  gfx->fillScreen(0);
  gfx->drawCircle(CX, CX, 204, TC_RING);
  gfx->drawCircle(CX, CX, 203, TC_RING);
  gfx->fillCircle((int)mzHoleX, (int)mzHoleY, 18, TC_HOLE);
  gfx->fillCircle((int)mzHoleX, (int)mzHoleY, 11, TC_HOLE2);
  gfx->fillCircle((int)mzX, (int)mzY, 12, TC_FLIP);
  gfx->fillCircle((int)mzX - 4, (int)mzY - 4, 4, TC_WHITE);
  toyText(String(mzScore).c_str(), CX, 48, 3, TC_TEXT);
  toyText("TAP LEVEL", CX, 404, 1, TC_DIM);
  toyBackDraw();
}

// ── brick breaker ────────────────────────────────────────────
#define BRK_ROWS 3
#define BRK_COLS 5
#define BRK_COUNT (BRK_ROWS * BRK_COLS)
#define BRK_BALL_R 8.0f
#define BRK_RIM 206.0f
#define BRK_PADDLE_R 196.0f
#define BRK_HALF 26.0f

static bool brkAlive[BRK_COUNT];
static int16_t brkX[BRK_COUNT], brkY[BRK_COUNT];
static int brkLeft = 0, brkScore = 0, brkLives = 3;
static float brkBX = CX, brkBY = CX, brkVX = 0, brkVY = 0;
static float brkAng = 90.0f, brkSpd = 250.0f;
static bool brkLaunched = false;

static void bricksLayout() {
  const int bw = 66, bh = 20, gap = 8;
  const int total = BRK_COLS * bw + (BRK_COLS - 1) * gap;
  int x0 = CX - total / 2;
  int y0 = 92;
  brkLeft = 0;
  int k = 0;
  for (int r = 0; r < BRK_ROWS; r++) {
    for (int c = 0; c < BRK_COLS; c++) {
      brkX[k] = (int16_t)(x0 + c * (bw + gap) + bw / 2);
      brkY[k] = (int16_t)(y0 + r * (bh + gap));
      brkAlive[k] = true;
      brkLeft++;
      k++;
    }
  }
}

static void bricksResetBall() {
  float a = brkAng * PI / 180.0f;
  brkBX = CX + cosf(a) * (BRK_PADDLE_R - BRK_BALL_R - 2.0f);
  brkBY = CX + sinf(a) * (BRK_PADDLE_R - BRK_BALL_R - 2.0f);
  brkVX = brkVY = 0;
  brkLaunched = false;
  brkSpd = 250.0f;
}

static void bricksStart() {
  brkScore = 0;
  brkLives = 3;
  brkAng = 90.0f;
  bricksLayout();
  toyReadTilt(toyRight0, toyDown0);
  bricksResetBall();
}

static void bricksLaunch() {
  if (brkLaunched) return;
  float a = (-90.0f + (float)(random(60) - 30)) * PI / 180.0f;
  brkVX = cosf(a) * brkSpd;
  brkVY = sinf(a) * brkSpd;
  brkLaunched = true;
}

static void bricksStep(float dt) {
  float r = 0, d = 0;
  if (toyReadTilt(r, d)) r -= toyRight0;
  float want = 90.0f - r * 110.0f;
  if (want < 40.0f) want = 40.0f;
  if (want > 140.0f) want = 140.0f;
  brkAng += (want - brkAng) * fminf(1.0f, dt * 10.0f);

  if (!brkLaunched) {
    float a = brkAng * PI / 180.0f;
    brkBX = CX + cosf(a) * (BRK_PADDLE_R - BRK_BALL_R - 2.0f);
    brkBY = CX + sinf(a) * (BRK_PADDLE_R - BRK_BALL_R - 2.0f);
    return;
  }

  int steps = (int)(dt / 0.008f) + 1;
  if (steps > 8) steps = 8;
  float sdt = dt / (float)steps;
  for (int s = 0; s < steps; s++) {
    brkBX += brkVX * sdt;
    brkBY += brkVY * sdt;

    for (int i = 0; i < BRK_COUNT; i++) {
      if (!brkAlive[i]) continue;
      const float hw = 33.0f, hh = 10.0f;
      if (brkBX < brkX[i] - hw - BRK_BALL_R || brkBX > brkX[i] + hw + BRK_BALL_R ||
          brkBY < brkY[i] - hh - BRK_BALL_R || brkBY > brkY[i] + hh + BRK_BALL_R) continue;
      float oR = (brkX[i] + hw + BRK_BALL_R) - brkBX;
      float oL = brkBX - (brkX[i] - hw - BRK_BALL_R);
      float oD = (brkY[i] + hh + BRK_BALL_R) - brkBY;
      float oU = brkBY - (brkY[i] - hh - BRK_BALL_R);
      if (fminf(oR, oL) < fminf(oD, oU)) {
        brkVX = -brkVX;
        brkBX += (oR < oL) ? oR : -oL;
      } else {
        brkVY = -brkVY;
        brkBY += (oD < oU) ? oD : -oU;
      }
      brkAlive[i] = false;
      brkLeft--;
      brkScore += 10;
      break;
    }

    float dx = brkBX - CX, dy = brkBY - CX;
    float rr = sqrtf(dx * dx + dy * dy);
    float ang = atan2f(dy, dx) * 180.0f / PI;
    if (ang < 0) ang += 360.0f;
    if (rr > BRK_RIM - BRK_BALL_R) {
      float da = fabsf(ang - brkAng);
      if (da > 180.0f) da = 360.0f - da;
      float nx = dx / rr, ny = dy / rr;
      float dp = brkVX * nx + brkVY * ny;
      if (da < BRK_HALF + 6.0f && rr > BRK_PADDLE_R - 6.0f) {
        if (dp > 0) { brkVX -= 2.0f * dp * nx; brkVY -= 2.0f * dp * ny; }
        float off = (ang - brkAng) / BRK_HALF;
        brkVX += off * 30.0f;
        brkSpd *= 1.015f;
        if (brkSpd > 380.0f) brkSpd = 380.0f;
        float sp = sqrtf(brkVX * brkVX + brkVY * brkVY);
        if (sp > 0.01f) { brkVX = brkVX / sp * brkSpd; brkVY = brkVY / sp * brkSpd; }
        brkBX = CX + nx * (BRK_PADDLE_R - 3.0f);
        brkBY = CX + ny * (BRK_PADDLE_R - 3.0f);
      } else {
        if (dp > 0) { brkVX -= 2.0f * dp * nx; brkVY -= 2.0f * dp * ny; }
        brkBX = CX + nx * (BRK_RIM - BRK_BALL_R - 1.0f);
        brkBY = CX + ny * (BRK_RIM - BRK_BALL_R - 1.0f);
      }
    }

    if (rr > 224.0f) {
      if (--brkLives <= 0) { brkLives = 3; brkScore = 0; }
      bricksLayout();
      bricksResetBall();
      break;
    }
  }

  if (brkLeft == 0) {
    brkScore += 100;
    bricksLayout();
    bricksResetBall();
  }
}

static void bricksDraw() {
  gfx->fillScreen(0);
  gfx->drawCircle(CX, CX, (int)BRK_RIM, TC_RING);
  gfx->drawCircle(CX, CX, (int)BRK_RIM - 3, TC_PANEL);
  for (int i = 0; i < BRK_COUNT; i++) {
    if (!brkAlive[i]) continue;
    int x = brkX[i], y = brkY[i];
    gfx->fillRoundRect(x - 33, y - 10, 66, 20, 5, TC_BRICK);
    gfx->drawRoundRect(x - 33, y - 10, 66, 20, 5, TC_BRICK_E);
  }
  toyArc(CX, CX, BRK_PADDLE_R, (brkAng - BRK_HALF) * PI / 180.0f,
         (brkAng + BRK_HALF) * PI / 180.0f, 14, TC_FLIP);
  gfx->fillCircle((int)brkBX, (int)brkBY, (int)BRK_BALL_R, TC_WHITE);
  gfx->fillCircle((int)brkBX - 3, (int)brkBY - 3, 3, TC_BALL_H);
  toyText((String("S") + String(brkScore) + "  O" + String(brkLives)).c_str(),
          CX, 42, 2, TC_TEXT);
  if (!brkLaunched) toyText("TAP TO LAUNCH", CX, 404, 1, TC_COUNT);
  toyBackDraw();
}

// ── pinball ──────────────────────────────────────────────────
#define PB_BALL_R 9.0f
#define PB_FLIP 70.0f
static const float PB_LPX = 150.0f, PB_LPY = 372.0f;
static const float PB_RPX = 316.0f, PB_RPY = 372.0f;
static const float PB_BX[3] = {233.0f, 150.0f, 316.0f};
static const float PB_BY[3] = {150.0f, 255.0f, 255.0f};
static const float PB_BR[3] = {24.0f, 22.0f, 22.0f};

static float pbX = 0, pbY = 0, pbVX = 0, pbVY = 0;
static int pbScore = 0, pbLives = 3;
static bool pbLeftDown = false, pbRightDown = false;
static float pbLAngle = 35.0f, pbRAngle = 145.0f;

static void pinballReset() {
  pbX = CX;
  pbY = 105;                 // above the top bumper, so it is not spawned inside it
  pbVX = (float)(random(120) - 60);
  pbVY = 60.0f;
}

static void pinballStart() {
  pbScore = 0;
  pbLives = 3;
  pinballReset();
  toyReadTilt(toyRight0, toyDown0);
}

static void pbReflect(float nx, float ny, float boost) {
  float dp = pbVX * nx + pbVY * ny;
  if (dp < 0) { pbVX -= 2.0f * dp * nx; pbVY -= 2.0f * dp * ny; }
  pbVX *= boost;
  pbVY *= boost;
}

static void pinballStep(float dt) {
  float r = 0, d = 0;
  if (toyReadTilt(r, d)) pbVX += (r - toyRight0) * 40.0f * dt;
  pbVY += 520.0f * dt;

  float targetL = pbLeftDown ? -10.0f : 35.0f;
  float targetR = pbRightDown ? 190.0f : 145.0f;
  pbLAngle += (targetL - pbLAngle) * fminf(1.0f, dt * 18.0f);
  pbRAngle += (targetR - pbRAngle) * fminf(1.0f, dt * 18.0f);

  int steps = (int)(dt / 0.006f) + 1;
  if (steps > 10) steps = 10;
  float sdt = dt / (float)steps;
  for (int s = 0; s < steps; s++) {
    pbX += pbVX * sdt;
    pbY += pbVY * sdt;

    float dx = pbX - CX, dy = pbY - CX;
    float rr = sqrtf(dx * dx + dy * dy);
    if (rr > 210.0f - PB_BALL_R) {
      float nx = dx / rr, ny = dy / rr;
      pbReflect(nx, ny, 0.9f);
      pbX = CX + nx * (210.0f - PB_BALL_R);
      pbY = CX + ny * (210.0f - PB_BALL_R);
    }

    for (int i = 0; i < 3; i++) {
      float ex = pbX - PB_BX[i], ey = pbY - PB_BY[i];
      float el = sqrtf(ex * ex + ey * ey);
      if (el < PB_BR[i] + PB_BALL_R && el > 0.01f) {
        float nx = ex / el, ny = ey / el;
        pbReflect(nx, ny, 1.25f);
        pbX = PB_BX[i] + nx * (PB_BR[i] + PB_BALL_R);
        pbY = PB_BY[i] + ny * (PB_BR[i] + PB_BALL_R);
        pbScore += 10;
      }
    }

    float la = pbLAngle * PI / 180.0f, ra = pbRAngle * PI / 180.0f;
    float lx1 = PB_LPX + cosf(la) * PB_FLIP, ly1 = PB_LPY + sinf(la) * PB_FLIP;
    float rx1 = PB_RPX + cosf(ra) * PB_FLIP, ry1 = PB_RPY + sinf(ra) * PB_FLIP;
    for (int side = 0; side < 2; side++) {
      float x0 = side ? PB_RPX : PB_LPX;
      float y0 = side ? PB_RPY : PB_LPY;
      float x1 = side ? rx1 : lx1;
      float y1 = side ? ry1 : ly1;
      float vx = x1 - x0, vy = y1 - y0;
      float len2 = vx * vx + vy * vy;
      float t = ((pbX - x0) * vx + (pbY - y0) * vy) / len2;
      if (t < 0) t = 0;
      if (t > 1) t = 1;
      float px = x0 + vx * t, py = y0 + vy * t;
      float ex = pbX - px, ey = pbY - py;
      float el = sqrtf(ex * ex + ey * ey);
      if (el < PB_BALL_R + 7.0f && el > 0.01f) {
        float nx = ex / el, ny = ey / el;
        pbReflect(nx, ny, 1.15f);
        bool down = side ? pbRightDown : pbLeftDown;
        if (down) {
          pbVX += -ny * (side ? -120.0f : 120.0f);
          pbVY += nx * (side ? -120.0f : 120.0f);
        }
        pbX = px + nx * (PB_BALL_R + 7.0f);
        pbY = py + ny * (PB_BALL_R + 7.0f);
      }
    }

    if (pbY > 440 || rr > 218) {
      if (--pbLives <= 0) { pbLives = 3; pbScore = 0; }
      pinballReset();
      break;
    }
  }
}

static void pinballDraw() {
  gfx->fillScreen(0);
  gfx->drawCircle(CX, CX, 210, TC_RING);
  gfx->drawCircle(CX, CX, 209, TC_PANEL);
  for (int i = 0; i < 3; i++) {
    gfx->fillCircle((int)PB_BX[i], (int)PB_BY[i], (int)PB_BR[i], TC_PINK);
    gfx->drawCircle((int)PB_BX[i], (int)PB_BY[i], (int)PB_BR[i], TC_PINK_E);
    gfx->drawCircle((int)PB_BX[i], (int)PB_BY[i], (int)PB_BR[i] - 4, TC_PINK_H);
  }
  float la = pbLAngle * PI / 180.0f, ra = pbRAngle * PI / 180.0f;
  toyThick(PB_LPX, PB_LPY, PB_LPX + cosf(la) * PB_FLIP, PB_LPY + sinf(la) * PB_FLIP,
           10, pbLeftDown ? TC_GOLD : TC_FLIP);
  toyThick(PB_RPX, PB_RPY, PB_RPX + cosf(ra) * PB_FLIP, PB_RPY + sinf(ra) * PB_FLIP,
           10, pbRightDown ? TC_GOLD : TC_FLIP);
  gfx->fillCircle((int)pbX, (int)pbY, (int)PB_BALL_R, TC_WHITE);
  gfx->fillCircle((int)pbX - 3, (int)pbY - 3, 3, TC_BALL_H);
  toyText((String("S") + String(pbScore) + "  O" + String(pbLives)).c_str(),
          CX, 42, 2, TC_TEXT);
  toyText("L / R FLIP", CX, 404, 1, TC_COUNT);
  toyBackDraw();
}

// ── water ────────────────────────────────────────────────────
static void waterStart() {
  if (!rippleBegin()) return;
  rippleClear();
}

static void waterDraw() {
  rippleDraw();
  toyBackDraw();
}

// ── dispatch ─────────────────────────────────────────────────
static void toyStartKind(ToyKind k) {
  // Leave the previous toy cleanly even when switching without going through
  // the menu (the boot test does this, and it caught the Water PSRAM leak).
  if (toyKind == TOY_WATER && k != TOY_WATER) rippleEnd();
  if (toyKind == TOY_RADAR && k != TOY_RADAR) planeExit();
  if (toyKind == TOY_REC && k != TOY_REC) recToyLeave();
  if (toyKind == TOY_PET && k != TOY_PET) petToyLeave();
  toyKind = k;
  toyDirty = true;
  toyLastMs = 0;
  toyWaitRelease = true;
  switch (k) {
    case TOY_POP: popStart(); break;
    case TOY_MARBLE: marbleStart(); break;
    case TOY_BRICKS: bricksStart(); break;
    case TOY_PINBALL: pinballStart(); break;
    case TOY_WATER: waterStart(); break;
    case TOY_RADAR: planeEnter(); break;
    case TOY_REC: recToyEnter(); break;
    case TOY_PET: petToyEnter(); break;
    default: break;
  }
}

static void toyBackToMenu() {
  if (toyKind == TOY_WATER) rippleEnd();
  if (toyKind == TOY_RADAR) planeExit();
  if (toyKind == TOY_REC) recToyLeave();
  if (toyKind == TOY_PET) petToyLeave();
  toyKind = TOY_MENU;
  toyDirty = true;
  toyWaitRelease = true;
}

void toyEnter() {
  toyOpen = true;
  toyKind = TOY_MENU;
  toyDirty = true;
  toyLastMs = 0;
  toyWaitRelease = true;
  onWaitScreen = false;
}

void toyExit() {
  if (toyKind == TOY_WATER) rippleEnd();
  if (toyKind == TOY_RADAR) planeExit();
  if (toyKind == TOY_REC) recToyLeave();
  if (toyKind == TOY_PET) petToyLeave();
  toyOpen = false;
  toyKind = TOY_MENU;
  drawWaiting((bleConnected && bleAuthed) ? WAIT_READY : WAIT_NOLINK);
}

// One-shot boot check: start every toy, run a few frames, flush, report memory.
// Enabled by TOY_BOOT_TEST in MotoNav.ino.
void toyUpdate();

void toyBootTest() {
  Serial.printf("TOY test start free=%u psram=%u\n",
                (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram());
  const ToyKind kinds[8] = {TOY_POP, TOY_MARBLE, TOY_BRICKS, TOY_PINBALL, TOY_WATER, TOY_RADAR, TOY_REC, TOY_PET};
  for (int i = 0; i < 8; i++) {
    uint32_t t0 = millis();
    toyOpen = true;
    toyStartKind(kinds[i]);
    if (kinds[i] == TOY_RADAR) planeFillTest();
    for (int f = 0; f < 12; f++) {
      toyUpdate();
      if (needFlush) { flushSync(); needFlush = false; }
      delay(18);
    }
    Serial.printf("TOY %d ok %ums free=%u psram=%u\n", (int)kinds[i],
                  (unsigned)(millis() - t0), (unsigned)ESP.getFreeHeap(),
                  (unsigned)ESP.getFreePsram());
  }
  toyBackToMenu();
  toyExit();
  pendingEvt = "";          // do not leak the boot test's T,AC,0 to a later phone
  flushSync();
  needFlush = false;
  Serial.printf("TOY test done free=%u psram=%u\n",
                (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram());
}

void toyButtonDraw() {
  const int x = 120, y = 350;
  gfx->drawRoundRect(x - 24, y - 16, 48, 32, 10, TC_EDGE);
  gfx->fillCircle(x - 12, y, 5, TC_TEXT);
  gfx->fillCircle(x + 12, y, 5, TC_TEXT);
  gfx->fillRect(x - 3, y - 11, 6, 6, TC_TEXT);
  gfx->fillRect(x - 10, y - 3, 6, 6, TC_TEXT);
  printCentered("TOY", x, y + 22, 1, C_GREY);
}

bool toyButtonHit(int x, int y) {
  int dx = x - 120, dy = y - 350;
  return dx * dx + dy * dy <= 44 * 44;
}

static void toyPointerDown(int x, int y) {
  if (toyBackHit(x, y)) {
    if (toyKind == TOY_MENU) toyExit();
    else toyBackToMenu();
    return;
  }
  if (toyKind == TOY_MENU) {
    int idx;
    if (toyMenuHit(x, y, idx)) toyStartKind(TOY_ITEMS[idx].kind);
    return;
  }
  switch (toyKind) {
    case TOY_POP: popTap(x, y); break;
    case TOY_MARBLE: toyReadTilt(toyRight0, toyDown0); break;
    case TOY_BRICKS: bricksLaunch(); break;
    case TOY_PINBALL:
      pbLeftDown = (x < CX);
      pbRightDown = !pbLeftDown;
      break;
    case TOY_WATER: rippleDrop(x, y, 1500); break;
    case TOY_RADAR: planeRadarTap(x, y); break;
    case TOY_REC: recToyTouch(x, y); break;
    case TOY_PET: petToyTouch(x, y); break;
    default: break;
  }
}

static void toyPointerMove(int x, int y) {
  if (toyKind == TOY_WATER) rippleDrop(x, y, 900);
  else if (toyKind == TOY_PINBALL) {
    pbLeftDown = (x < CX);
    pbRightDown = !pbLeftDown;
  }
}

static void toyPointerUp(int, int) {
  pbLeftDown = false;
  pbRightDown = false;
}

void toyTouchPoll() {
  static bool down = false;
  static int lx = 0, ly = 0;
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < 8) return;
  last = now;
  if (!touchOK) return;
  int16_t x[1] = {0}, y[1] = {0};
  uint8_t n = touch.getPoint(x, y, 1);
  if (toyWaitRelease) {
    if (n == 0) toyWaitRelease = false;
    return;
  }
  if (n > 0) {
    int cx = x[0], cy = y[0];
    touchPointToCanvas(cx, cy);
    if (!down) {
      down = true;
      toyPointerDown(cx, cy);
    } else if (cx != lx || cy != ly) {
      toyPointerMove(cx, cy);
    }
    lx = cx; ly = cy;
  } else if (down) {
    down = false;
    toyPointerUp(lx, ly);
  }
}

void toyUpdate() {
  uint32_t now = millis();
  float dt = toyLastMs ? (now - toyLastMs) / 1000.0f : 0.016f;
  toyLastMs = now;
  if (dt > 0.1f) dt = 0.1f;

  if (toyKind == TOY_MENU) {
    if (toyDirty) {
      toyMenuDraw();
      toyDirty = false;
      needFlush = true;
    }
    return;
  }
  if (toyKind == TOY_POP) {
    popStep();
    if (popDirty) {
      popDraw();
      popDirty = false;
      needFlush = true;
    }
    return;
  }
  if (toyKind == TOY_RADAR) {
    planeStep();
    static uint32_t radarAt = 0;
    if (now - radarAt < 200) return;   // data arrives every ~2 s
    radarAt = now;
    planeRadarDraw();
    toyBackDraw();
    needFlush = true;
    return;
  }
  if (toyKind == TOY_REC) {
    static uint32_t recAt = 0;
    if (now - recAt < 100) return;
    recAt = now;
    recToyDraw();
    toyBackDraw();
    needFlush = true;
    return;
  }
  if (toyKind == TOY_PET) {
    static uint32_t petAt = 0;
    if (now - petAt < 100) return;
    petAt = now;
    petTick();
    petToyDraw();
    toyBackDraw();
    needFlush = true;
    return;
  }

  if (toyKind == TOY_MARBLE) marbleStep(dt);
  else if (toyKind == TOY_BRICKS) bricksStep(dt);
  else if (toyKind == TOY_PINBALL) pinballStep(dt);
  else if (toyKind == TOY_WATER) rippleStep(dt);

  static uint32_t drawAt = 0;
  if (now - drawAt < 24) return;
  drawAt = now;
  if (toyKind == TOY_MARBLE) marbleDraw();
  else if (toyKind == TOY_BRICKS) bricksDraw();
  else if (toyKind == TOY_PINBALL) pinballDraw();
  else if (toyKind == TOY_WATER) waterDraw();
  needFlush = true;
}
