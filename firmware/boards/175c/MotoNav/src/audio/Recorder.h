#pragma once
// ES7210 microphone recorder for the 1.75C.
//
// The 1.75C has an ES8311 DAC and an ES7210 ADC on the same I2S pins. Only one
// can own the bus at a time, so starting a recording shuts the speaker I2S
// down, installs the legacy I2S RX driver on I2S_NUM_1 with the official
// Waveshare ES7210 configuration, and restores the speaker on exit.
//
// Audio is encoded with the same IMA-ADPCM block format as The-Badge
// (256 B = 505 samples, 16 kHz mono, ~8.1 KB/s) into PSRAM. A 2 MB buffer is
// about 4.3 minutes. Nothing is written to flash, so no partition change.

#include <Arduino.h>
#include <math.h>
#include "AudioFeedback.h"

// Legacy I2S / ES7210 live in MicI2S.cpp so their i2s_mode_t does not clash
// with ESP_I2S.h in this translation unit.
bool micBegin();
int micReadMono(int16_t *dst, int maxSamples, uint32_t timeoutMs);
void micEnd();

#define REC_BLOCK_BYTES 256
#define REC_BLOCK_SAMPLES 505
#define REC_BUF_BYTES (2 * 1024 * 1024)
#define REC_MAX_BLOCKS (REC_BUF_BYTES / REC_BLOCK_BYTES)
#define REC_SAMPLE_RATE 16000

// Set from AudioFeedback.h via extern; soundFeedback() must stay silent while
// the microphone/playback owns the I2S bus.
bool recActive = false;
volatile bool recRecording = false;
volatile bool recPlaying = false;
volatile uint32_t recBlocks = 0;
volatile uint32_t recPlayBlock = 0;
volatile int16_t recPeak = 0;
static uint8_t *recBuf = nullptr;
static int16_t recPcm[REC_BLOCK_SAMPLES];
static int recPcmN = 0;
static TaskHandle_t recTaskHandle = nullptr;
static volatile bool recTaskStop = false;
static uint32_t recStartedAt = 0;

// ---- IMA ADPCM (The-Badge adpcm.c, MIT) ----
static const int16_t REC_STEP[89] = {
    7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,
    80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,
    494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,
    2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,
    8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,
    27086,29794,32767,
};
static const int8_t REC_INDEX[16] = {
    -1,-1,-1,-1,2,4,6,8, -1,-1,-1,-1,2,4,6,8,
};
static int32_t recPred = 0;
static int8_t recIndex = 0;

static uint8_t recEncodeOne(int16_t sample) {
  int step = REC_STEP[recIndex];
  int diff = sample - recPred;
  uint8_t code = 0;
  if (diff < 0) { code = 8; diff = -diff; }
  int delta = step >> 3;
  if (diff >= step) { code |= 4; diff -= step; delta += step; }
  step >>= 1;
  if (diff >= step) { code |= 2; diff -= step; delta += step; }
  step >>= 1;
  if (diff >= step) { code |= 1; delta += step; }
  recPred += (code & 8) ? -delta : delta;
  if (recPred > 32767) recPred = 32767;
  if (recPred < -32768) recPred = -32768;
  recIndex += REC_INDEX[code];
  if (recIndex < 0) recIndex = 0;
  if (recIndex > 88) recIndex = 88;
  return code;
}

static void recEncodeBlock(const int16_t *pcm, int n, uint8_t *out) {
  memset(out, 0, REC_BLOCK_BYTES);
  if (n <= 0) return;
  recPred = pcm[0];
  out[0] = (uint8_t)(recPred & 0xFF);
  out[1] = (uint8_t)((recPred >> 8) & 0xFF);
  out[2] = (uint8_t)recIndex;
  out[3] = 0;
  for (int i = 1; i < REC_BLOCK_SAMPLES; i++) {
    uint8_t c = recEncodeOne(i < n ? pcm[i] : (int16_t)recPred);
    int pos = i - 1;
    if (pos & 1) out[4 + pos / 2] |= (uint8_t)(c << 4);
    else out[4 + pos / 2] = c;
  }
}

static void recDecodeBlock(const uint8_t *in, int16_t *out) {
  int32_t pred = (int16_t)(in[0] | (in[1] << 8));
  int idx = in[2];
  if (idx < 0) idx = 0;
  if (idx > 88) idx = 88;
  out[0] = (int16_t)pred;
  for (int i = 1; i < REC_BLOCK_SAMPLES; i++) {
    int pos = i - 1;
    int code = (pos & 1) ? (in[4 + pos / 2] >> 4) : (in[4 + pos / 2] & 0x0F);
    int step = REC_STEP[idx];
    int delta = step >> 3;
    if (code & 4) delta += step;
    if (code & 2) delta += step >> 1;
    if (code & 1) delta += step >> 2;
    pred += (code & 8) ? -delta : delta;
    if (pred > 32767) pred = 32767;
    if (pred < -32768) pred = -32768;
    idx += REC_INDEX[code];
    if (idx < 0) idx = 0;
    if (idx > 88) idx = 88;
    out[i] = (int16_t)pred;
  }
}

// ---- capture / playback tasks ----
static void recTask(void *) {
  while (!recTaskStop) {
    int got = micReadMono(recPcm + recPcmN, REC_BLOCK_SAMPLES - recPcmN, 20);
    if (got <= 0) continue;
    for (int i = 0; i < got; i++) {
      int a = abs(recPcm[recPcmN + i]);
      if (a > recPeak) recPeak = (int16_t)a;
    }
    recPcmN += got;
    if (recPcmN >= REC_BLOCK_SAMPLES) {
      if (recBlocks < REC_MAX_BLOCKS) {
        recEncodeBlock(recPcm, REC_BLOCK_SAMPLES, recBuf + recBlocks * REC_BLOCK_BYTES);
        recBlocks++;
      } else {
        recTaskStop = true;
        break;
      }
      recPcmN = 0;
    }
  }
  if (recPcmN > 0 && recBlocks < REC_MAX_BLOCKS) {
    for (int i = recPcmN; i < REC_BLOCK_SAMPLES; i++) recPcm[i] = recPcm[recPcmN - 1];
    recEncodeBlock(recPcm, REC_BLOCK_SAMPLES, recBuf + recBlocks * REC_BLOCK_BYTES);
    recBlocks++;
    recPcmN = 0;
  }
  recTaskHandle = nullptr;
  vTaskDelete(nullptr);
}

static void recPlayTask(void *) {
  int16_t pcm[REC_BLOCK_SAMPLES];
  int16_t out[REC_BLOCK_SAMPLES * 2];
  while (!recTaskStop && recPlayBlock < recBlocks) {
    recDecodeBlock(recBuf + recPlayBlock * REC_BLOCK_BYTES, pcm);
    for (int i = 0; i < REC_BLOCK_SAMPLES; i++) {
      out[i * 2] = pcm[i];
      out[i * 2 + 1] = pcm[i];
    }
    audioI2S.write((const uint8_t *)out, sizeof(out));
    recPlayBlock++;
  }
  recPlaying = false;
  recActive = false;
  recTaskHandle = nullptr;
  vTaskDelete(nullptr);
}

static bool recEnsureBuf() {
  if (recBuf) return true;
  recBuf = (uint8_t *)ps_malloc(REC_BUF_BYTES);
  if (!recBuf) Serial.println("REC: PSRAM allocation failed");
  return recBuf != nullptr;
}

static void recStopAll() {
  if (!recRecording && !recPlaying) return;
  recTaskStop = true;
  uint32_t t = millis();
  while (recTaskHandle && millis() - t < 1000) delay(5);
  if (recRecording) {
    recRecording = false;
    micEnd();
  }
  recPlaying = false;
  recActive = false;
  audioInit();                    // restore the ES8311 speaker path
}

static bool recStart() {
  if (recRecording || recPlaying) return false;
  if (!recEnsureBuf()) return false;
  recBlocks = 0;
  recPcmN = 0;
  recPeak = 0;
  recTaskStop = false;

  audioI2S.end();                 // release the shared I2S pins
  audioReady = false;
  pinMode(PA, OUTPUT);
  digitalWrite(PA, LOW);          // mute the speaker amp while capturing

  if (!micBegin()) {
    Serial.println("REC: mic begin failed");
    audioInit();
    return false;
  }

  recActive = true;
  recRecording = true;
  recStartedAt = millis();
  if (xTaskCreatePinnedToCore(recTask, "rec", 4096, nullptr, 3, &recTaskHandle, 1) != pdPASS) {
    Serial.println("REC: task create failed");
    recStopAll();
    return false;
  }
  Serial.println("REC: start");
  return true;
}

static bool recPlay() {
  if (recActive || recBlocks == 0) return false;
  if (!audioInit()) return false;
  recPlaying = true;
  recActive = true;
  recPlayBlock = 0;
  recTaskStop = false;
  if (xTaskCreatePinnedToCore(recPlayTask, "play", 4096, nullptr, 3, &recTaskHandle, 1) != pdPASS) {
    recPlaying = false;
    recActive = false;
    return false;
  }
  Serial.println("REC: play");
  return true;
}

static void recStop() {
  if (recRecording) {
    recTaskStop = true;
    uint32_t t = millis();
    while (recTaskHandle && millis() - t < 1000) delay(5);
    recRecording = false;
    micEnd();
    recActive = false;
    audioInit();
    Serial.printf("REC: stop blocks=%u\n", (unsigned)recBlocks);
  } else if (recPlaying) {
    recTaskStop = true;
  }
}

static uint32_t recSeconds() {
  return (uint32_t)((uint64_t)recBlocks * REC_BLOCK_SAMPLES / REC_SAMPLE_RATE);
}
