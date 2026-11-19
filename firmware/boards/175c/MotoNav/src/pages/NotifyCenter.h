#pragma once
// Notification center for the 1.75C (page 4).
//
// The phone forwards notifications as "#N|<app>|<title>|<body>" (UTF-8,
// truncated by the phone). The screen keeps the newest 20, shows a 4 s banner
// on the home page and a scrollable list on page 4. No images/fonts are sent,
// so Chinese text is intentionally not rendered here; the app name and body
// are shown in the phone's own language by the Android notification pipeline
// only when it can use ASCII/Latin. This is the same limitation as the
// existing weather/call text-image channel, but the list is still useful for
// Western app names and short messages.

#include <Arduino.h>

#define NOTE_MAX 20
#define NOTE_APP_MAX 20
#define NOTE_TITLE_MAX 40
#define NOTE_BODY_MAX 80

struct NoteItem {
  char app[NOTE_APP_MAX];
  char title[NOTE_TITLE_MAX];
  char body[NOTE_BODY_MAX];
  uint32_t at;
};

NoteItem noteItems[NOTE_MAX];
int noteCount = 0;
int noteScroll = 0;
bool notifyBanner = false;
uint32_t notifyBannerUntil = 0;
bool notifyDirty = true;
bool notifyOpen = false;          // retained for AOD compatibility

static void noteCopy(char *dst, size_t cap, const String &src) {
  if (!cap) return;
  size_t n = src.length();
  if (n >= cap) n = cap - 1;
  memcpy(dst, src.c_str(), n);
  dst[n] = 0;
  for (size_t i = 0; i < n; i++) if (dst[i] == '\n' || dst[i] == '\r' || dst[i] == '|') dst[i] = ' ';
}

void notifyPush(const String &app, const String &title, const String &body) {
  for (int i = NOTE_MAX - 1; i > 0; i--) noteItems[i] = noteItems[i - 1];
  noteCopy(noteItems[0].app, sizeof(noteItems[0].app), app);
  noteCopy(noteItems[0].title, sizeof(noteItems[0].title), title);
  noteCopy(noteItems[0].body, sizeof(noteItems[0].body), body);
  noteItems[0].at = millis();
  if (noteCount < NOTE_MAX) noteCount++;
  noteScroll = 0;
  notifyBanner = true;
  notifyBannerUntil = millis() + 5000;
  notifyDirty = true;
  needFlush = true;
  Serial.printf("NOTIFY %s | %s\n", noteItems[0].app, noteItems[0].title);
}

void notifyClear() {
  noteCount = 0;
  noteScroll = 0;
  notifyBanner = false;
  notifyDirty = true;
  needFlush = true;
}

void drawNotifyBanner() {
  if (!notifyBanner || !noteCount || page != 0 || inNav || screenDark) return;
  const int y = 50, h = 58;
  gfx->fillRoundRect(CX - 176, y, 352, h, 14, 0x1082);
  gfx->drawRoundRect(CX - 176, y, 352, h, 14, 0x4208);
  printCentered(String(noteItems[0].app), CX, y + 8, 2, C_CYAN);
  printCentered(String(noteItems[0].title), CX, y + 28, 2, C_WHITE);
  if (noteItems[0].body[0]) printCentered(String(noteItems[0].body), CX, y + 44, 1, 0x8C71);
  needFlush = true;
}

void drawNotifyBell() {
  if (!noteCount || page != 0 || inNav || screenDark) return;
  const int x = CX - 84, y = 30;
  aaDisc(x, y, 10, 0x18E3);
  gfx->drawCircle(x, y, 10, 0x4208);
  gfx->fillTriangle(x - 4, y + 5, x + 4, y + 5, x, y - 5, C_WHITE);
  gfx->fillRect(x - 5, y + 5, 10, 3, C_WHITE);
  gfx->fillCircle(x, y + 9, 2, 0xF800);
  printCentered(String(noteCount), x + 12, y - 6, 1, C_WHITE);
}

bool notifyBellHit(int x, int y) {
  if (!noteCount || page != 0 || inNav) return false;
  const int dx = x - (CX - 84), dy = y - 30;
  return dx * dx + dy * dy <= 24 * 24;
}

void notifyScrollBy(int delta) {
  if (!noteCount) return;
  const int maxScroll = max(0, noteCount - 5);
  noteScroll = constrain(noteScroll + delta, 0, maxScroll);
  notifyDirty = true;
}

void drawNotifyList() {
  onWaitScreen = false;
  frameIsMap = false;
  notifyOpen = true;
  gfx->fillScreen(C_BLACK);
  printCentered("NOTIFICATIONS", CX, 24, 3, C_WHITE);
  if (!noteCount) {
    printCentered("NO NOTIFICATIONS", CX, 190, 3, C_GREY);
  } else {
    const int first = 70;
    for (int i = 0; i < 5; i++) {
      int idx = noteScroll + i;
      if (idx >= noteCount) break;
      int y = first + i * 62;
      gfx->fillRoundRect(34, y, 398, 54, 10, 0x1082);
      gfx->drawRoundRect(34, y, 398, 54, 10, 0x31A6);
      printCentered(String(noteItems[idx].app), 58, y + 6, 1, C_CYAN);
      printCentered(String(noteItems[idx].title), 58 + 80, y + 6, 2, C_WHITE);
      if (noteItems[idx].body[0]) printCentered(String(noteItems[idx].body), CX, y + 30, 1, 0x8C71);
    }
    if (noteCount > 5) {
      printCentered(noteScroll + 5 < noteCount ? "TAP LOWER HALF: MORE" : "TAP LOWER HALF: FIRST",
                    CX, 392, 1, 0x6B4D);
    }
  }
  gfx->fillRoundRect(CX - 130, 414, 120, 30, 15, 0x1082);
  gfx->drawRoundRect(CX - 130, 414, 120, 30, 15, 0x4208);
  printCentered("BACK", CX - 70, 421, 2, C_WHITE);
  gfx->fillRoundRect(CX + 10, 414, 120, 30, 15, 0x1082);
  gfx->drawRoundRect(CX + 10, 414, 120, 30, 15, 0x4208);
  printCentered("CLEAR", CX + 70, 421, 2, C_WHITE);
  drawPageDots();
  notifyDirty = false;
  needFlush = true;
}

bool notifyTap(int x, int y) {
  if (y >= 414 && y <= 446) {
    if (x < CX) { page = 0; notifyOpen = false; drawWaiting(bleConnected && bleAuthed ? WAIT_READY : WAIT_NOLINK); }
    else notifyClear();
    return true;
  }
  if (noteCount > 5 && y >= 220) {
    const int maxScroll = max(0, noteCount - 5);
    noteScroll = (noteScroll + 5 > maxScroll) ? 0 : noteScroll + 5;
    notifyDirty = true;
    return true;
  }
  return false;
}

void notifyTick() {
  if (screenDark) return;
  if (page != 4) notifyOpen = false;
  if (notifyBanner && (int32_t)(millis() - notifyBannerUntil) >= 0) notifyBanner = false;
  if (page == 4) {
    if (notifyDirty) drawNotifyList();
    return;
  }
  if (page == 0 && !inNav && notifyBanner) drawNotifyBanner();
}
