#pragma once
// AXP2101 rail and charge configuration for the 1.75C.
//
// Ported from curisama/The-Badge main/port_esp.c::axp_init (MIT), which was
// written against this exact board (Waveshare ESP32-S3-Touch-AMOLED-1.75C,
// 32 MB flash, 8 MB PSRAM, AXP2101).
//
// Two things are corrected here:
// 1. Only DCDC1 (VCC3V3) and ALDO1 (A3V3) are populated on this board. The
//    other rails are unused, but the AXP2101 boots from EFUSE defaults that can
//    leave enabled bucks without inductors switching forever. Keep DCDC1 and
//    ALDO1, switch everything else off.
// 2. The chip's charge defaults terminate at 125 mA (over half the 200 mA
//    charge current), so the battery is reported full at roughly 60%. Lower
//    pre-charge/termination to 25 mA and raise the cutoff from 2.6 V to 3.0 V.

#include <Arduino.h>
#include "Battery.h"

static bool pmuWriteReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(PMU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static void pmicConfigure() {
  if (!pmuOK) {
    Serial.println("PMIC: not found, rails unchanged");
    return;
  }
  const int d0 = pmuRead(0x80);
  const int l0 = pmuRead(0x90);
  const int l1 = pmuRead(0x91);
  if (d0 < 0 || l0 < 0 || l1 < 0) {
    Serial.println("PMIC: rail read failed, rails unchanged");
    return;
  }
  Serial.printf("PMIC rails before DCDC=0x%02X LDO=0x%02X/0x%02X\n", d0, l0, l1);

  pmuWriteReg(0x80, (uint8_t)((d0 & ~0x1E) | 0x01));  // DCDC1 only
  pmuWriteReg(0x90, 0x01);                             // ALDO1 only
  pmuWriteReg(0x91, (uint8_t)(l1 & ~0x01));            // DLDO off

  const int d1 = pmuRead(0x80);
  const int l2 = pmuRead(0x90);
  const int l3 = pmuRead(0x91);
  Serial.printf("PMIC rails after  DCDC=0x%02X LDO=0x%02X/0x%02X\n", d1, l2, l3);

  const int ipre = pmuRead(0x61);
  const int iterm = pmuRead(0x63);
  const int voff = pmuRead(0x24);
  if (ipre >= 0) pmuWriteReg(0x61, (uint8_t)((ipre & 0xF0) | 0x01));
  if (iterm >= 0) pmuWriteReg(0x63, (uint8_t)((iterm & 0xF0) | 0x01));
  if (voff >= 0) pmuWriteReg(0x24, (uint8_t)((voff & 0xF8) | 0x04));

  const int ip = pmuRead(0x61);
  const int it = pmuRead(0x63);
  const int vo = pmuRead(0x24);
  if (ip >= 0 && it >= 0 && vo >= 0) {
    Serial.printf("PMIC charge pre=%dmA term=%dmA cutoff=%.1fV\n",
                  (ip & 0x0F) * 25, (it & 0x0F) * 25, 2.6f + (vo & 0x07) * 0.1f);
  }
}
