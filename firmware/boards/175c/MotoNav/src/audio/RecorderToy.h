#pragma once
// Recorder page: 16 kHz mono IMA-ADPCM in PSRAM (~4.3 min), record/play.
#include "../audio/Recorder.h"

static void recToyEnter() {
  recEnsureBuf();
  recRecording = false;
  recPlaying = false;
  recPlayBlock = 0;
  recPeak = 0;
}

static void recToyLeave() {
  recStopAll();
  if (recBuf) {
    free(recBuf);
    recBuf = nullptr;
  }
  recActive = false;
}

static void recToyDraw() {
  gfx->fillScreen(0);
  printCentered("RECORDER", CX, 36, 3, C_WHITE);
  if (!recBuf) {
    printCentered("NO PSRAM", CX, CX - 12, 3, C_RED);
    return;
  }

  uint32_t sec = recSeconds();
  char tbuf[24];
  snprintf(tbuf, sizeof(tbuf), "%02u:%02u", (unsigned)(sec / 60), (unsigned)(sec % 60));
  printCentered(tbuf, CX, 86, 4, recRecording ? C_RED : C_WHITE);

  int lvl = (int)recPeak * 240 / 32767;
  if (lvl > 240) lvl = 240;
  gfx->drawRect(112, 146, 242, 18, 0x4208);
  gfx->fillRect(114, 148, lvl, 14, recRecording ? 0xF800 : 0x07E0);

  const char *st = recPlaying ? "PLAYING" : recRecording ? "RECORDING" : "READY";
  printCentered(st, CX, 178, 2, recRecording ? C_RED : recPlaying ? C_GREEN : C_GREY);

  aaDisc(140, 320, 46, recRecording ? 0xF800 : 0x18E3);
  printCentered(recRecording ? "STOP" : "REC", 140, 312, 2, C_WHITE);
  aaDisc(326, 320, 46, recPlaying ? 0x07E0 : 0x18E3);
  printCentered(recPlaying ? "STOP" : "PLAY", 326, 312, 2, C_WHITE);

  printCentered("16K ADPCM / PSRAM", CX, 390, 1, 0x6B4D);
  printCentered(recRecording ? "TAP REC TO STOP" : "TAP REC TO START", CX, 404, 1, 0x6B4D);
  needFlush = true;
}

static bool recToyTouch(int x, int y) {
  int dx = x - 140, dy = y - 320;
  if (dx * dx + dy * dy <= 46 * 46) {
    if (recRecording) recStop();
    else if (!recPlaying) recStart();
    return true;
  }
  dx = x - 326;
  if (dx * dx + dy * dy <= 46 * 46) {
    if (recPlaying) recStop();
    else if (!recRecording) recPlay();
    return true;
  }
  return false;
}
