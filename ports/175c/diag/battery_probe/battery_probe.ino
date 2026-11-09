// Read-only AXP2101 battery probe for Waveshare ESP32-S3-Touch-AMOLED-1.75C.
// Prints status, battery voltage and gauge percentage over USB CDC every 2s.
#include <Arduino.h>
#include <Wire.h>

#define PMU_ADDR 0x34
#define IIC_SDA 15
#define IIC_SCL 14

static uint8_t rd(uint8_t reg) {
  Wire.beginTransmission(PMU_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0xFF;
  if (Wire.requestFrom((uint8_t)PMU_ADDR, (uint8_t)1) != 1) return 0xFF;
  return Wire.read();
}

static void setBit(uint8_t reg, uint8_t bit) {
  uint8_t v = rd(reg);
  if (v == 0xFF || (v & (1 << bit))) return;
  Wire.beginTransmission(PMU_ADDR);
  Wire.write(reg);
  Wire.write((uint8_t)(v | (1 << bit)));
  Wire.endTransmission();
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  Wire.begin(IIC_SDA, IIC_SCL);
  Serial.println();
  Serial.println("AXP2101 battery probe (1.75C)");
  setBit(0x18, 3);  // enableGauge()
  setBit(0x68, 0);  // enableBattDetection()
  setBit(0x30, 0);  // enableBattVoltageMeasure()
  Serial.printf("after init: A2=0x%02X 18=0x%02X 30=0x%02X 68=0x%02X\n",
                rd(0xA2), rd(0x18), rd(0x30), rd(0x68));

  // XPowersLib fuelGaugeControl(writeROM=true, enable=true):
  // clear A2 bit4 (use ROM model), set A2 bit0 (enable gauge).
  Serial.printf("before gauge ctrl: A2=0x%02X pct=%d\n", rd(0xA2), rd(0xA4));
  uint8_t a2 = rd(0xA2);
  if (a2 != 0xFF) {
    a2 = (uint8_t)((a2 & ~(1 << 4)) | (1 << 0));
    Wire.beginTransmission(PMU_ADDR);
    Wire.write(0xA2);
    Wire.write(a2);
    Wire.endTransmission();
  }
  delay(250);
  Serial.printf("after gauge ctrl:  A2=0x%02X pct=%d\n", rd(0xA2), rd(0xA4));
}

void loop() {
  uint8_t s1 = rd(0x00), s2 = rd(0x01);
  uint8_t h = rd(0x34), l = rd(0x35);
  int mv = ((h & 0x1F) << 8) | l;
  int pct = rd(0xA4);
  uint8_t gauge = rd(0xA2);
  uint8_t ctrl = rd(0x18);
  Serial.printf("S1=0x%02X S2=0x%02X present=%d vbusGood=%d chgState=%d battMv=%d pct=%d A2=0x%02X 18=0x%02X\n",
                s1, s2,
                (s1 >> 3) & 1,
                (s1 >> 5) & 1,
                (s2 >> 5) & 3,
                mv, pct, gauge, ctrl);
  delay(2000);
}
