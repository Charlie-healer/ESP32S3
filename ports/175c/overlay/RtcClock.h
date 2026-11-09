#pragma once
#include <esp_attr.h>
#include <esp_private/esp_clk.h>
#include <stdint.h>
#include <stdio.h>

// 1.75C uses the ESP32-S3 internal RTC (32.768kHz crystal, powered by AXP2101
// RTCLDO/VBAT2). settimeofday() alone is lost on reset, so keep the last synced
// epoch and the RTC counter value in RTC_NOINIT memory: .rtc.data is reloaded on
// a normal reset, .rtc_noinit is not.
static const uint32_t RTC_STATE_MAGIC = 0x4D4F544FUL;  // "MOTO"

RTC_NOINIT_ATTR uint32_t rtcStateMagic;
RTC_NOINIT_ATTR uint32_t rtcStateBootCount;
RTC_NOINIT_ATTR uint32_t rtcStateBaseEpoch;  // local epoch, valid until 2106
RTC_NOINIT_ATTR uint64_t rtcStateBaseUs;     // esp_clk_rtc_time() at sync

void rtcClockSave(int64_t localEpoch) {
  if (localEpoch < 1577836800LL || localEpoch >= 4102444800LL) return;
  rtcStateBaseEpoch = static_cast<uint32_t>(localEpoch);
  rtcStateBaseUs = esp_clk_rtc_time();
  rtcStateMagic = RTC_STATE_MAGIC;
  Serial.printf("RTC save epoch=%lu rtcUs=%llu\n",
                (unsigned long)rtcStateBaseEpoch,
                (unsigned long long)rtcStateBaseUs);
}

void rtcClockInit() {
  static bool done = false;
  if (done) return;
  done = true;

  uint64_t nowUs = esp_clk_rtc_time();
  rtcStateBootCount++;
  Serial.printf("RTC boot #%lu magic=%08lX rtcUs=%llu baseEpoch=%lu baseUs=%llu\n",
                (unsigned long)rtcStateBootCount,
                (unsigned long)rtcStateMagic,
                (unsigned long long)nowUs,
                (unsigned long)rtcStateBaseEpoch,
                (unsigned long long)rtcStateBaseUs);

  if (rtcStateMagic != RTC_STATE_MAGIC ||
      rtcStateBaseEpoch < 1577836800UL ||
      nowUs < rtcStateBaseUs) {
    Serial.println("RTC no valid base");
    return;
  }

  uint64_t elapsedSec = (nowUs - rtcStateBaseUs) / 1000000ULL;
  int64_t localEpoch = static_cast<int64_t>(rtcStateBaseEpoch) +
                       static_cast<int64_t>(elapsedSec);
  char packet[48];
  snprintf(packet, sizeof(packet), "#TIME,%lld,0", (long long)localEpoch);
  bool ok = homeClock.receive(packet, millis());
  Serial.printf("RTC restore epoch=%lld elapsed=%llus ok=%d\n",
                (long long)localEpoch,
                (unsigned long long)elapsedSec,
                ok ? 1 : 0);
}
