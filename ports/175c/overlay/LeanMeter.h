#pragma once
// Full lean / G dashboard for the 1.75C (QMI8658 on I2C 0x6B).
// Visual layout follows the StopWatch PageLeanMeter; the IMU source is the
// 1.75C QMI8658 and the page remains a toggleable overlay.

#include <sensor/imu/qmi8658/SensorQMI8658.hpp>
#include "FullStandbyUi.h"

SensorQMI8658 imu;

bool leanOpen = false;
bool imuOK = false;
bool imuTried = false;
float leanZero = 0;
float leanRaw = 0;
float leanAngle = 0;
float leanG = 1;
float leanLateral = 0;
float leanLongitudinal = 0;
float leanMaxG = 0;
float leanMaxL = 0, leanMaxR = 0;
float gyroX = 0, gyroY = 0, gyroZ = 0;
uint32_t leanCalibUntil = 0;

#define LEAN_COLOR_SAFE   0x4ECB
#define LEAN_COLOR_WARN   0xFB65
#define LEAN_COLOR_ALERT  0xFA68
#define LEAN_RING_05G     0x1908
#define LEAN_RING_10G     0x29CE
#define LEAN_RING_15G     0x3A32
#define LEAN_CROSSHAIR    0x2106

void leanInit() {
  if (imuTried) return;
  imuTried = true;
  if (!imu.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    if (!imu.begin(Wire, QMI8658_H_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
      Serial.println("LEAN: QMI8658 not found");
      return;
    }
  }
  imu.configAccel(SensorQMI8658::AccelRange::FS_4G, 1000.0f, SensorQMI8658::LpfMode::MODE_0);
  imu.enableAccel();
  imu.configGyro(SensorQMI8658::GyroRange::FS_512DPS, 112.1f, SensorQMI8658::LpfMode::MODE_0);
  imu.enableGyro();
  imuOK = true;
  Serial.println("LEAN: QMI8658 ready");
}

void leanRead() {
  if (!imuOK) return;
  static unsigned long last = 0;
  if (millis() - last < 50) return;
  last = millis();
  if (!imu.isDataReady()) return;
  AccelerometerData accel;
  GyroscopeData gyro;
  if (!imu.readAccel(accel)) return;
  imu.readGyro(gyro);
  float ax = accel.mps2.x / 9.80665f;
  float ay = accel.mps2.y / 9.80665f;
  float az = accel.mps2.z / 9.80665f;
  gyroX = gyro.dps.x; gyroY = gyro.dps.y; gyroZ = gyro.dps.z;
  leanLateral = ax;
  leanLongitudinal = -ay;
  leanRaw = atan2f(ax, az) * 180.0f / PI;
  leanAngle = leanRaw - leanZero;
  leanG = sqrtf(ax * ax + ay * ay + az * az);
  if (leanG > leanMaxG) leanMaxG = leanG;
  if (leanAngle < leanMaxL) leanMaxL = leanAngle;
  if (leanAngle > leanMaxR) leanMaxR = leanAngle;
}

void drawLeanButton() {
  const int gx = 346, gy = 350, r = 14;
  aaDisc(gx, gy, r, C_GREY);
  aaDisc(gx, gy, r - 5, C_BLACK);
  printCentered("G", gx, gy - 6, 2, C_GREY);
  printCentered("LEAN", gx, gy + 15, 1, C_GREY);
}

bool leanButtonHit(int x, int y) {
  int dx = x - 346, dy = y - 350;
  return dx * dx + dy * dy <= 50 * 50;
}

static void drawLeanZeroButton() {
  gfx->fillRoundRect(100, 420, 100, 26, 10, 0x18E3);
  gfx->drawRoundRect(100, 420, 100, 26, 10, 0x4208);
  printCentered("ZERO", 150, 427, 1, C_WHITE);
}

static void drawLeanClose() {
  drawTranslucentCircle(410, 54, 17, 0x18E3, 220);
  gfx->drawCircle(410, 54, 17, 0x4A69);
  gfx->drawLine(405, 49, 415, 59, C_WHITE);
  gfx->drawLine(405, 59, 415, 49, C_WHITE);
}

void drawLeanSheet() {
  if (!leanOpen) return;
  onWaitScreen = false;
  frameIsMap = false;
  gfx->fillScreen(C_BLACK);
  aaDisc(CX - 150, 28, 4.0f, bleConnected ? LEAN_COLOR_SAFE : 0x632C);

  const float cx = (float)CX, cy = (float)CX;
  const float rOut = 206.0f, rIn = 154.0f;
  float cur = leanAngle;
  float absCur = fabsf(cur);
  if (absCur >= 1.0f) {
    float a1 = 270.0f, a2 = 270.0f + cur;
    if (a1 > a2) { float t = a1; a1 = a2; a2 = t; }
    uint16_t col = absCur <= 35.0f ? LEAN_COLOR_SAFE : (absCur <= 50.0f ? LEAN_COLOR_WARN : LEAN_COLOR_ALERT);
    for (float a = a1; a < a2; a += 0.5f) {
      float n = a + 0.5f; if (n > a2) n = a2;
      float r1 = a * (float)PI / 180.0f, r2 = n * (float)PI / 180.0f;
      int xi1 = (int)lroundf(cx + rIn * cosf(r1)), yi1 = (int)lroundf(cy + rIn * sinf(r1));
      int xo1 = (int)lroundf(cx + rOut * cosf(r1)), yo1 = (int)lroundf(cy + rOut * sinf(r1));
      int xi2 = (int)lroundf(cx + rIn * cosf(r2)), yi2 = (int)lroundf(cy + rIn * sinf(r2));
      int xo2 = (int)lroundf(cx + rOut * cosf(r2)), yo2 = (int)lroundf(cy + rOut * sinf(r2));
      gfx->fillTriangle(xi1, yi1, xo1, yo1, xo2, yo2, col);
      gfx->fillTriangle(xi1, yi1, xo2, yo2, xi2, yi2, col);
    }
    float rad = (270.0f + cur) * (float)PI / 180.0f;
    int x1 = (int)lroundf(cx + (rIn - 4) * cosf(rad)), y1 = (int)lroundf(cy + (rIn - 4) * sinf(rad));
    int x2 = (int)lroundf(cx + (rOut + 4) * cosf(rad)), y2 = (int)lroundf(cy + (rOut + 4) * sinf(rad));
    gfx->drawLine(x1, y1, x2, y2, C_WHITE);
    gfx->drawLine(x1 + 1, y1, x2 + 1, y2, C_WHITE);
    gfx->drawLine(x1 - 1, y1, x2 - 1, y2, C_WHITE);
  }

  for (int a = -60; a <= 60; ++a) {
    int aa = abs(a);
    float len = (aa % 30 == 0) ? 26.0f : ((aa % 10 == 0) ? 16.0f : ((aa % 5 == 0) ? 8.0f : 0.0f));
    if (!len) continue;
    float rad = (270.0f + a) * (float)PI / 180.0f;
    uint16_t col = aa == 0 ? C_WHITE : (aa <= 35 ? LEAN_COLOR_SAFE : (aa <= 50 ? LEAN_COLOR_WARN : LEAN_COLOR_ALERT));
    int x1 = (int)lroundf(cx + (rOut - len) * cosf(rad));
    int y1 = (int)lroundf(cy + (rOut - len) * sinf(rad));
    int x2 = (int)lroundf(cx + rOut * cosf(rad));
    int y2 = (int)lroundf(cy + rOut * sinf(rad));
    gfx->drawLine(x1, y1, x2, y2, col);
  }
  drawAaString(cx, cy - rOut - 12, "0", 1.8f, 1.0f, 10.0f, C_WHITE, true);
  drawAaString(cx + (rOut + 15) * cosf(300.0f * PI / 180.0f), cy + (rOut + 15) * sinf(300.0f * PI / 180.0f), "30", 1.6f, 0.95f, 10.0f, LEAN_COLOR_SAFE, true);
  drawAaString(cx + (rOut + 16) * cosf(240.0f * PI / 180.0f), cy + (rOut + 16) * sinf(240.0f * PI / 180.0f), "-30", 1.6f, 0.95f, 10.0f, LEAN_COLOR_SAFE, true);
  drawAaString(cx + (rOut + 15) * cosf(330.0f * PI / 180.0f), cy + (rOut + 15) * sinf(330.0f * PI / 180.0f), "60", 1.6f, 0.95f, 10.0f, LEAN_COLOR_ALERT, true);
  drawAaString(cx + (rOut + 16) * cosf(210.0f * PI / 180.0f), cy + (rOut + 16) * sinf(210.0f * PI / 180.0f), "-60", 1.6f, 0.95f, 10.0f, LEAN_COLOR_ALERT, true);

  const int gr = 104;
  const float maxG = 1.5f;
  gfx->drawCircle(CX, CX, 35, LEAN_RING_05G);
  gfx->drawCircle(CX, CX, 70, LEAN_RING_10G);
  gfx->drawCircle(CX, CX, gr, LEAN_RING_15G);
  gfx->drawLine(CX - gr, CX, CX - 8, CX, LEAN_CROSSHAIR);
  gfx->drawLine(CX + 8, CX, CX + gr, CX, LEAN_CROSSHAIR);
  gfx->drawLine(CX, CX - gr, CX, CX - 8, LEAN_CROSSHAIR);
  gfx->drawLine(CX, CX + 8, CX, CX + gr, LEAN_CROSSHAIR);
  drawAaString(CX, CX - gr + 12, "^ ACC", 1.1f, 0.7f, 8.0f, 0x7BEF, true);
  drawAaString(CX, CX + gr - 12, "v BRK", 1.1f, 0.7f, 8.0f, 0x7BEF, true);

  float dx = constrain(leanLateral / maxG, -1.0f, 1.0f) * (gr - 13);
  float dy = -constrain(leanLongitudinal / maxG, -1.0f, 1.0f) * (gr - 13);
  int bx = (int)lroundf(cx + dx), by = (int)lroundf(cy + dy);
  aaDisc((float)bx, (float)by, 8.5f, 0x1A29);
  gfx->drawCircle(bx, by, 8, LEAN_COLOR_SAFE);
  aaDisc((float)bx, (float)by, 3.5f, C_WHITE);
  char gbuf[16]; snprintf(gbuf, sizeof(gbuf), "%.2f G", leanG);
  drawAaString(CX, CX + 52, gbuf, 2.2f, 1.25f, 14.0f, LEAN_COLOR_SAFE, true);

  drawTranslucentRoundRect(CX - 169 - 43, CX - 21, 86, 42, 14, 0, 185);
  gfx->drawRoundRect(CX - 169 - 43, CX - 21, 86, 42, 14, 0x31A6);
  drawAaString(CX - 169, CX - 10, "MAX L", 1.2f, 0.75f, 8.5f, 0x7BEF, true);
  char lbuf[16]; snprintf(lbuf, sizeof(lbuf), "-%d*", (int)lroundf(fabsf(leanMaxL)));
  drawAaString(CX - 169, CX + 7, lbuf, 1.8f, 1.05f, 11.5f, C_WHITE, true);
  drawTranslucentRoundRect(CX + 169 - 43, CX - 21, 86, 42, 14, 0, 185);
  gfx->drawRoundRect(CX + 169 - 43, CX - 21, 86, 42, 14, 0x31A6);
  drawAaString(CX + 169, CX - 10, "MAX R", 1.2f, 0.75f, 8.5f, 0x7BEF, true);
  char rbuf[16]; snprintf(rbuf, sizeof(rbuf), "+%d*", (int)lroundf(fabsf(leanMaxR)));
  drawAaString(CX + 169, CX + 7, rbuf, 1.8f, 1.05f, 11.5f, C_WHITE, true);

  char mbuf[16]; snprintf(mbuf, sizeof(mbuf), "%.2f G", leanMaxG);
  drawTranslucentRoundRect(CX - 93, 346, 186, 30, 15, 0, 185);
  gfx->drawRoundRect(CX - 93, 346, 186, 30, 15, 0x31A6);
  drawAaString(CX - 42, 361, "MAX G", 1.3f, 0.8f, 9.0f, 0x7BEF, true);
  drawAaString(CX + 34, 361, mbuf, 1.7f, 1.0f, 11.0f, LEAN_COLOR_SAFE, true);

  char abuf[16];
  if (fabsf(leanAngle) < 0.5f) snprintf(abuf, sizeof(abuf), "0*");
  else snprintf(abuf, sizeof(abuf), "%+d*", (int)lroundf(leanAngle));
  drawTranslucentRoundRect(CX - 93, 382, 186, 30, 15, 0, 185);
  gfx->drawRoundRect(CX - 93, 382, 186, 30, 15, 0x31A6);
  drawAaString(CX - 42, 397, "LEAN", 1.3f, 0.8f, 9.0f, 0x7BEF, true);
  drawAaString(CX + 34, 397, abuf, 1.9f, 1.15f, 12.0f, LEAN_COLOR_SAFE, true);

  drawLeanZeroButton();
  drawLeanClose();
  needFlush = true;
}

void leanTick() {
  if (screenDark || !leanOpen) return;
  leanRead();
  static unsigned long lastDraw = 0;
  if (millis() - lastDraw < 100) return;
  lastDraw = millis();
  drawLeanSheet();
}

bool leanTap(int x, int y) {
  Serial.printf("LEAN tap x=%d y=%d open=%d\n", x, y, leanOpen ? 1 : 0);
  if (leanOpen) {
    // Large exit target: any tap on the top strip, plus the visible X.
    if (y < 110) {
      leanOpen = false;
      drawWaiting(waitSt);
      return true;
    }
    if (x >= 100 && x <= 200 && y >= 420 && y <= 446) {
      leanZero = leanRaw;
      leanMaxL = 0; leanMaxR = 0; leanMaxG = 0;
      leanCalibUntil = millis() + 1200;
      drawLeanSheet();
      return true;
    }
    return true;
  }
  if (destSheetOpen || settingsOpen || companionOpen) return false;
  if (leanButtonHit(x, y)) {
    destSheetOpen = false;
    settingsOpen = false;
    leanOpen = true;
    leanInit();
    drawLeanSheet();
    return true;
  }
  return false;
}
