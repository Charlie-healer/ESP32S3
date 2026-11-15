#pragma once
// Double-tap-and-drag map zoom, ported from the StopWatch branch
// (TouchInput.h / MapEngine.h `userMapScale`). The scale is a user multiplier
// on top of the automatic speed-based zoom, clamped to 0.30x..3.00x, and is
// persisted under the `zoom` NVS key.
#include <Preferences.h>
#include <math.h>
#include <stdint.h>

bool mapZoomActive = false;
uint32_t mapZoomHintUntil = 0;
uint16_t mapZoomStartX = 0, mapZoomStartY = 0;
float mapZoomStartScale = 1.0f;
float userMapScale = 1.0f;
uint32_t mapZoomSaveAt = 0;
uint32_t lastMapTapAt = 0;
uint16_t lastMapTapX = 0, lastMapTapY = 0;

static inline void mapZoomLoad() {
  Preferences pr;
  pr.begin("motonav", true);
  userMapScale = pr.getFloat("zoom", 1.0f);
  pr.end();
  if (!(userMapScale >= 0.30f && userMapScale <= 3.00f)) userMapScale = 1.0f;
}

static inline void mapZoomSaveNow() {
  Preferences pr;
  pr.begin("motonav", false);
  pr.putFloat("zoom", userMapScale);
  pr.end();
}

static inline void mapZoomTick(uint32_t now) {
  if (!mapZoomSaveAt) return;
  if ((int32_t)(now - mapZoomSaveAt) < 0) return;
  mapZoomSaveAt = 0;
  mapZoomSaveNow();
}
