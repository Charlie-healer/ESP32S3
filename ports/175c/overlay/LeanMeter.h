#pragma once
// Lean / G meter overlay for the 1.75C port (QMI8658 on I2C 0x6B).

#include <sensor/imu/qmi8658/SensorQMI8658.hpp>

SensorQMI8658 imu;

bool leanOpen = false;
bool imuOK = false;
bool imuTried = false;
float leanZero = 0;
float leanRaw = 0;
float leanAngle = 0;
float leanG = 1;
float leanMaxL = 0, leanMaxR = 0;
float gyroX = 0, gyroY = 0, gyroZ = 0;

void leanInit() {
  if (imuTried) return;
  imuTried = true;
  // The deprecated SensorQMI8658.hpp defines the same class name as the new
  // driver, which breaks the vtable when both translation units are linked.
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
  float ax = accel.mps2.x, ay = accel.mps2.y, az = accel.mps2.z;
  gyroX = gyro.dps.x; gyroY = gyro.dps.y; gyroZ = gyro.dps.z;
  leanRaw = atan2f(ax, az) * 180.0f / PI;
  leanAngle = leanRaw - leanZero;
  leanG = sqrtf(ax * ax + ay * ay + az * az) / 9.80665f;
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

void drawLeanSheet() {
  if (!leanOpen) return;
  const int x0 = 49, y0 = 49, S = 368;
  gfx->fillRoundRect(x0, y0, S, S, 70, 0x1082);
  gfx->drawRoundRect(x0, y0, S, S, 70, 0x4208);
  printCentered("LEAN / G", CX, y0 + 20, 2, C_WHITE);

  const int closeX = x0 + S - 30, closeY = y0 + 30;
  aaDisc(closeX, closeY, 15, 0x18E3);
  gfx->drawLine(closeX - 6, closeY - 6, closeX + 6, closeY + 6, C_WHITE);
  gfx->drawLine(closeX - 6, closeY + 6, closeX + 6, closeY - 6, C_WHITE);

  const int cx = CX, cy = 200, R = 110;
  gfx->drawLine(cx - R, cy, cx + R, cy, 0x4208);
  for (int deg = -60; deg <= 60; deg += 10) {
    float a = deg * PI / 180.0f;
    int x1 = cx + (int)(sinf(a) * R), y1 = cy - (int)(cosf(a) * R);
    int x2 = cx + (int)(sinf(a) * (R - 8)), y2 = cy - (int)(cosf(a) * (R - 8));
    gfx->drawLine(x2, y2, x1, y1, 0x4208);
  }
  float a = leanAngle;
  if (a < -60) a = -60;
  if (a > 60) a = 60;
  a = a * PI / 180.0f;
  int nx = cx + (int)(sinf(a) * R), ny = cy - (int)(cosf(a) * R);
  uint16_t needle = fabsf(leanAngle) > 35 ? C_RED : (fabsf(leanAngle) > 20 ? C_ORANGE : C_GREEN);
  gfx->drawLine(cx, cy, nx, ny, needle);
  aaDisc(cx, cy, 5, C_WHITE);

  printCentered(String("LEAN ") + String(leanAngle, 1) + " DEG", CX, 318, 2, C_WHITE);
  printCentered(String("G ") + String(leanG, 2), CX, 344, 2, C_GREY);
  printCentered(String("MAX L ") + String((int)leanMaxL) + " / R " + String((int)leanMaxR), CX, 362, 1, C_GREY);
  if (!imuOK) printCentered("IMU NOT FOUND", CX, 370, 2, C_RED);

  gfx->drawRoundRect(100, 376, 100, 28, 10, 0x4208);
  printCentered("ZERO", 150, 382, 1, C_WHITE);
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
    // Any top-strip tap exits; the visible X is at the top-right.
    if (y < 110) {
      leanOpen = false;
      drawWaiting(waitSt);
      return true;
    }
    if (x >= 100 && x <= 200 && y >= 376 && y <= 404) {
      leanZero = leanRaw;
      leanMaxL = 0; leanMaxR = 0;
      drawLeanSheet();
      return true;
    }
    if (x < 49 || x > 417 || y < 49 || y > 417) {
      leanOpen = false;
      drawWaiting(waitSt);
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
