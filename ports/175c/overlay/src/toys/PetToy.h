#pragma once
// TamaPoke-inspired pet for the 1.75C.
//
// The real TamaPoke loads 151 animated Pokémon from a microSD card; this board
// has no SD slot. This port keeps the care loop (fullness / joy / energy /
// hygiene, egg -> baby -> child -> adult, offline decay, NVS persistence) and
// draws a procedural creature instead of the sprite set. It lives as its own
// TOY entry, so CompanionMode remains available independently.
#include <Preferences.h>
#include <math.h>
#include <time.h>

static bool petLoaded = false;
static uint8_t petFull = 80, petJoy = 80, petEnergy = 80, petHyg = 100;
static uint8_t petStage = 0;          // 0 egg, 1 baby, 2 child, 3 adult
static uint8_t petTaps = 0;
static uint32_t petAge = 0;           // minutes since hatch
static uint32_t petEpoch = 0;         // last wall-clock second
static uint32_t petAt = 0;            // millis fallback
static bool petSleep = false;
static uint32_t petHeartAt = 0;

static inline uint8_t petClamp(int v) { return (uint8_t)(v < 0 ? 0 : v > 100 ? 100 : v); }

static uint32_t petNowEpoch() {
  time_t t = time(nullptr);
  return (t > 1600000000) ? (uint32_t)t : 0;
}

static void petSave() {
  Preferences pr;
  if (!pr.begin("motonav", false)) return;
  pr.putUChar("petF", petFull);
  pr.putUChar("petJ", petJoy);
  pr.putUChar("petE", petEnergy);
  pr.putUChar("petH", petHyg);
  pr.putUChar("petS", petStage);
  pr.putUChar("petT", petTaps);
  pr.putUInt("petA", petAge);
  pr.putUInt("petX", petEpoch);
  pr.putBool("petZ", petSleep);
  pr.end();
}

static void petApplyMinutes(uint32_t mins) {
  if (mins > 10080) mins = 10080;   // cap offline progression at 7 days
  for (uint32_t i = 0; i < mins; i++) {
    if (petStage == 0) break;
    petAge++;
    if (petSleep) {
      petEnergy = petClamp(petEnergy + 6);
    } else {
      if (petFull > 0) petFull--;
      if (petFull < 30 && petJoy > 0) petJoy--;
      if (petEnergy > 0) petEnergy--;
      if (petHyg > 0 && (petAge % 3) == 0) petHyg--;
    }
    if (petStage == 1 && petAge >= 60 && petFull >= 40 && petJoy >= 40) petStage = 2;
    if (petStage == 2 && petAge >= 240 && petFull >= 50 && petJoy >= 50) petStage = 3;
  }
}

static void petLoad() {
  if (petLoaded) return;
  petLoaded = true;
  Preferences pr;
  bool ok = pr.begin("motonav", true);
  if (ok) {
    petFull = pr.getUChar("petF", 80);
    petJoy = pr.getUChar("petJ", 80);
    petEnergy = pr.getUChar("petE", 80);
    petHyg = pr.getUChar("petH", 100);
    petStage = pr.getUChar("petS", 0);
    petTaps = pr.getUChar("petT", 0);
    petAge = pr.getUInt("petA", 0);
    petEpoch = pr.getUInt("petX", 0);
    petSleep = pr.getBool("petZ", false);
    pr.end();
  }
  if (petFull > 100) petFull = 100;
  if (petJoy > 100) petJoy = 100;
  if (petEnergy > 100) petEnergy = 100;
  if (petHyg > 100) petHyg = 100;
  if (petStage > 3) petStage = 3;
  petAt = millis();
}

static void petTick() {
  petLoad();
  uint32_t nowEpoch = petNowEpoch();
  if (nowEpoch && petEpoch && nowEpoch > petEpoch) {
    uint32_t mins = (nowEpoch - petEpoch) / 60;
    if (mins) {
      petApplyMinutes(mins);
      petEpoch += mins * 60;
      petSave();
    }
  } else if (!nowEpoch) {
    uint32_t now = millis();
    if (petAt && now - petAt >= 60000) {
      petApplyMinutes((now - petAt) / 60000);
      petAt = now;
      petSave();
    }
  }
  if (nowEpoch) petEpoch = nowEpoch;
}

static void petHatch() {
  petStage = 1;
  petAge = 0;
  petFull = 70;
  petJoy = 75;
  petEnergy = 90;
  petHyg = 100;
  petTaps = 0;
}

static void petFeed() {
  if (petStage == 0) { petTaps++; if (petTaps >= 3) petHatch(); petSave(); return; }
  if (petSleep) return;
  petFull = petClamp(petFull + 25);
  petJoy = petClamp(petJoy + 5);
  petHyg = petClamp(petHyg - 2);
  petSave();
}

static void petPlay() {
  if (petStage == 0 || petSleep || petEnergy < 10) return;
  petJoy = petClamp(petJoy + 20);
  petEnergy = petClamp(petEnergy - 12);
  petFull = petClamp(petFull - 4);
  petHeartAt = millis();
  petSave();
}

static void petSleepToggle() {
  if (petStage == 0) return;
  petSleep = !petSleep;
  petSave();
}

static void petClean() {
  if (petStage == 0) return;
  petHyg = 100;
  petJoy = petClamp(petJoy + 3);
  petSave();
}

static void petCaress() {
  if (petStage == 0) {
    petTaps++;
    if (petTaps >= 3) petHatch();
    petSave();
    return;
  }
  petJoy = petClamp(petJoy + 3);
  petHeartAt = millis();
}

static void petToyEnter() {
  petLoaded = false;
  petLoad();
  petTick();
}

static void petToyLeave() {
  petSave();
}

static const char *petStageName() {
  return petStage == 0 ? "EGG" : petStage == 1 ? "BABY" : petStage == 2 ? "CHILD" : "ADULT";
}

static void petBar(int x, int y, int w, const char *label, uint8_t v) {
  printCentered(label, x - 18, y - 4, 1, 0x6B4D);
  gfx->drawRect(x, y, w, 8, 0x4208);
  uint16_t c = v > 60 ? 0x07E0 : v > 30 ? 0xFE60 : 0xF800;
  gfx->fillRect(x + 1, y + 1, (w - 2) * v / 100, 6, c);
}

static void petDrawCreature(int cx, int cy) {
  const bool sad = petJoy < 25 || petFull < 20;
  const uint16_t body = petStage == 1 ? 0x7E8F : petStage == 2 ? 0x5DD8 : 0x7B1F;
  const int r = petStage == 1 ? 28 : petStage == 2 ? 40 : 52;
  const int bob = (int)(sinf(millis() / 350.0f) * 4);
  cy += bob;

  if (petStage == 0) {
    gfx->fillEllipse(cx, cy, 38, 48, 0xFF9B);
    gfx->drawEllipse(cx, cy, 38, 48, 0xFFF0);
    for (int i = -2; i <= 2; i++) {
      gfx->fillCircle(cx + i * 13, cy - 8 + (i & 1) * 10, 4, 0xFE73);
    }
    // cracks appear with each tap
    for (int i = 0; i < petTaps; i++) {
      gfx->drawLine(cx - 16 + i * 10, cy - 20, cx - 6 + i * 10, cy + 12, 0x7B4D);
    }
    return;
  }

  // ears
  gfx->fillTriangle(cx - r + 6, cy - r + 4, cx - r + 18, cy - r - 18, cx - r + 26, cy - r + 4, body);
  gfx->fillTriangle(cx + r - 6, cy - r + 4, cx + r - 18, cy - r - 18, cx + r - 26, cy - r + 4, body);
  gfx->fillCircle(cx, cy, r, body);
  gfx->fillCircle(cx - r / 3, cy + r / 4, r / 3, 0xFFFF);

  // eyes
  if (petSleep) {
    gfx->drawLine(cx - 18, cy - 8, cx - 6, cy - 8, 0x0000);
    gfx->drawLine(cx + 6, cy - 8, cx + 18, cy - 8, 0x0000);
  } else {
    gfx->fillCircle(cx - 12, cy - 8, 5, 0x0000);
    gfx->fillCircle(cx + 12, cy - 8, 5, 0x0000);
    gfx->fillCircle(cx - 10, cy - 10, 2, 0xFFFF);
    gfx->fillCircle(cx + 14, cy - 10, 2, 0xFFFF);
  }
  if (sad) gfx->drawLine(cx - 10, cy + 14, cx + 10, cy + 14, 0x0000);
  else {
    for (int i = -8; i <= 8; i++) gfx->drawPixel(cx + i, cy + 14 - (i * i) / 20, 0x0000);
  }
  gfx->fillCircle(cx - 24, cy + 6, 5, 0xF9B6);
  gfx->fillCircle(cx + 24, cy + 6, 5, 0xF9B6);
  if (petSleep) printCentered("Z z", cx + 52, cy - r + 4, 2, 0x7BEF);
  if ((int32_t)(millis() - petHeartAt) < 1500) {
    printCentered("<3", cx, cy - r - 26, 2, 0xF800);
  }
}

static void petToyDraw() {
  gfx->fillScreen(0);
  printCentered("PET", CX, 20, 3, C_WHITE);
  char agebuf[32];
  uint32_t h = petAge / 60, m = petAge % 60;
  snprintf(agebuf, sizeof(agebuf), "%s  %uh%02um", petStageName(), (unsigned)h, (unsigned)m);
  printCentered(agebuf, CX, 52, 1, 0x8C71);

  petBar(110, 76, 110, "FULL", petFull);
  petBar(110, 96, 110, "JOY", petJoy);
  petBar(110, 116, 110, "ENERGY", petEnergy);
  petBar(110, 136, 110, "CLEAN", petHyg);

  petDrawCreature(CX, 270);

  const char *labels[4] = {"FEED", "PLAY", petSleep ? "WAKE" : "SLEEP", "CLEAN"};
  const int bx[4] = {90, 190, 290, 390};
  for (int i = 0; i < 4; i++) {
    aaDisc(bx[i], 370, 27, 0x18E3);
    printCentered(labels[i], bx[i], 364, 1, C_WHITE);
  }
  if (petStage == 0) printCentered("TAP THE EGG x3", CX, 408, 1, 0x6B4D);
  else if (petSleep) printCentered("SLEEPING", CX, 408, 1, 0x6B4D);
  else printCentered("TAP PET TO CARESS", CX, 408, 1, 0x6B4D);
  needFlush = true;
}

static bool petToyTouch(int x, int y) {
  const int bx[4] = {90, 190, 290, 390};
  for (int i = 0; i < 4; i++) {
    int dx = x - bx[i], dy = y - 370;
    if (dx * dx + dy * dy <= 27 * 27) {
      if (i == 0) petFeed();
      else if (i == 1) petPlay();
      else if (i == 2) petSleepToggle();
      else petClean();
      return true;
    }
  }
  int dx = x - CX, dy = y - 270;
  if (dx * dx + dy * dy <= 70 * 70) {
    petCaress();
    return true;
  }
  return false;
}
