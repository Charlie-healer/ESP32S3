#pragma once
#include <stdint.h>

// Two home skins. The rider home is the original 1.75C overlay; the full
// standby skin is the StopWatch-style VICE dashboard. Keep both alive so the
// setting only changes which renderer is used.
enum HomeStyle : uint8_t {
  HOME_RIDER = 0,
  HOME_FULL  = 1
};

HomeStyle homeStyle = HOME_RIDER;

void drawWaitingFull(int st);
void fullHomeTick();

static inline const char *homeStyleName(HomeStyle s) {
  return s == HOME_FULL ? "VICE" : "RIDER";
}
