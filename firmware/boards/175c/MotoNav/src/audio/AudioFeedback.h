#pragma once
#include <Arduino.h>
#include <ESP_I2S.h>
#include <math.h>
#include "es8311.h"
#include "../power/ScreenState.h"

// Minimal ES8311 tone output for the 1.75C. It replaces the StopWatch
// vibration motor: short UI/companion feedback is rendered as a beep.
I2SClass audioI2S;
static es8311_handle_t audioCodec = nullptr;
static bool audioReady = false;
static bool audioFailed = false;

bool audioInit() {
  if (audioReady) return true;
  if (audioFailed) return false;

  audioI2S.setPins(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO, I2S_DI_IO, I2S_MCK_IO);
  if (!audioI2S.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT,
                      I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
    audioFailed = true;
    Serial.println("AUDIO: I2S begin failed");
    return false;
  }

  pinMode(PA, OUTPUT);
  digitalWrite(PA, LOW);

  audioCodec = es8311_create(0, ES8311_ADDRESS_0);
  if (!audioCodec) {
    audioFailed = true;
    Serial.println("AUDIO: ES8311 create failed");
    return false;
  }
  const es8311_clock_config_t clk = {
      .mclk_inverted = false,
      .sclk_inverted = false,
      .mclk_from_mclk_pin = true,
      .mclk_frequency = 16000 * 256,
      .sample_frequency = 16000,
  };
  if (es8311_init(audioCodec, &clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16) != ESP_OK ||
      es8311_sample_frequency_config(audioCodec, clk.mclk_frequency, clk.sample_frequency) != ESP_OK ||
      es8311_microphone_config(audioCodec, false) != ESP_OK ||
      es8311_voice_volume_set(audioCodec, 80, nullptr) != ESP_OK) {
    audioFailed = true;
    Serial.println("AUDIO: ES8311 init failed");
    return false;
  }
  audioReady = true;
  Serial.println("AUDIO: ES8311 ready");
  return true;
}

// Strength (0-255) and duration mirror the old vibration parameters.
void soundFeedback(uint8_t strength = 150, uint16_t durationMs = 45) {
  if (screenDark || !audioInit()) return;
  if (durationMs < 15) durationMs = 15;
  if (durationMs > 250) durationMs = 250;
  const int freq = 520 + (int)strength * 4;   // 520..1540 Hz
  const int totalFrames = 16000 * (int)durationMs / 1000;
  digitalWrite(PA, HIGH);
  int16_t chunk[400 * 2];
  int done = 0;
  while (done < totalFrames) {
    int n = min(400, totalFrames - done);
    for (int i = 0; i < n; ++i) {
      int frame = done + i;
      float env = min(1.0f, frame / 64.0f);
      env = min(env, (totalFrames - frame) / 64.0f);
      int16_t v = (int16_t)(sinf(2.0f * PI * freq * frame / 16000.0f) * 2800.0f * env);
      chunk[i * 2] = v;
      chunk[i * 2 + 1] = v;
    }
    audioI2S.write((const uint8_t *)chunk, n * 4);
    done += n;
  }
  digitalWrite(PA, LOW);
}
