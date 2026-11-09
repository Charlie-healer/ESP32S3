#pragma once
// Destination drawer overlay for the 1.75C port.
//
// App protocol:
//   firmware -> phone: T,DEST_Q          request the destination list
//   firmware -> phone: T,DEST_GO,<idx>   route to the selected item
//   phone -> firmware: #DEST,CLR
//   phone -> firmware: #DEST,ADD,<type>,<name>,<addr>

extern String pendingEvt;   // defined in IncomingCall.h

#define DEST_STAR_Y 408
#define DEST_STAR_R 16
// destStarY is defined in HomePage.h: 408 = rider home, 316 = full standby.
#define DEST_PANEL_SIZE 368
#define DEST_PANEL_RADIUS 70
#define DEST_ITEM_GAP 12

int destSheetShown() {
  int s = destItemCount;
  if (s > 4) s = 4;
  if (s < 0) s = 0;
  return s;
}

int destItemH(int shown) {
  if (shown >= 4) return 54;
  if (shown == 3) return 72;
  return 76;
}

int destSheetX() { return CX - DEST_PANEL_SIZE / 2; }
int destSheetY() { return CX - DEST_PANEL_SIZE / 2; }

int destListStartY(int shown) {
  if (shown <= 0) return destSheetY() + 60;
  int ih = destItemH(shown);
  int gap = shown >= 4 ? 10 : DEST_ITEM_GAP;
  int total = shown * ih + (shown - 1) * gap;
  int areaTop = destSheetY() + 60;
  int areaBottom = destSheetY() + DEST_PANEL_SIZE - 44;
  int y = areaTop + (areaBottom - areaTop - total) / 2;
  if (y < areaTop) y = areaTop;
  return y;
}

static void destStarPoints(int cx, int cy, int r, int *px, int *py) {
  for (int i = 0; i < 10; i++) {
    float a = -PI / 2.0f + i * PI / 5.0f;
    float rr = (i % 2 == 0) ? r : r * 0.45f;
    px[i] = cx + (int)(cosf(a) * rr);
    py[i] = cy + (int)(sinf(a) * rr);
  }
}

static void destFillStar(int cx, int cy, int r, uint16_t color) {
  int px[10], py[10];
  destStarPoints(cx, cy, r, px, py);
  for (int i = 0; i < 10; i++) {
    int j = (i + 1) % 10;
    gfx->fillTriangle(cx, cy, px[i], py[i], px[j], py[j], color);
  }
}

void drawDestStar() {
  destFillStar(CX, destStarY, DEST_STAR_R, 0xFDE0);
  printCentered("DEST", CX, destStarY + 21, 1, C_GREY);
}

bool destStarHit(int x, int y) {
  int dx = x - CX, dy = y - destStarY;
  return dx * dx + dy * dy <= 30 * 30;
}

int destCloseX() { return destSheetX() + DEST_PANEL_SIZE - 30; }
int destCloseY() { return destSheetY() + 30; }
bool destCloseHit(int x, int y) {
  int dx = x - destCloseX(), dy = y - destCloseY();
  return dx * dx + dy * dy <= 22 * 22;
}

void drawDestClose() {
  int cx = destCloseX(), cy = destCloseY();
  aaDisc(cx, cy, 15, 0x18E3);
  gfx->drawLine(cx - 6, cy - 6, cx + 6, cy + 6, C_WHITE);
  gfx->drawLine(cx - 6, cy + 6, cx + 6, cy - 6, C_WHITE);
}

int destSheetItemAt(int x, int y) {
  if (!destSheetOpen) return -1;
  int shown = destSheetShown();
  int ih = destItemH(shown);
  int gap = shown >= 4 ? 10 : DEST_ITEM_GAP;
  int ix = destSheetX() + 16;
  int iw = DEST_PANEL_SIZE - 32;
  int startY = destListStartY(shown);
  for (int i = 0; i < shown; i++) {
    int iy = startY + i * (ih + gap);
    if (x >= ix && x <= ix + iw && y >= iy && y <= iy + ih) return i;
  }
  return -1;
}

void drawDestSheet() {
  if (!destSheetOpen) return;
  Serial.println("DEST draw sheet");

  int shown = destSheetShown();
  int x0 = destSheetX(), y0 = destSheetY();
  int ih = destItemH(shown);
  int gap = shown >= 4 ? 10 : DEST_ITEM_GAP;
  int ix = x0 + 16, iw = DEST_PANEL_SIZE - 32;

  gfx->fillRoundRect(x0, y0, DEST_PANEL_SIZE, DEST_PANEL_SIZE, DEST_PANEL_RADIUS, 0x1082);
  gfx->drawRoundRect(x0, y0, DEST_PANEL_SIZE, DEST_PANEL_SIZE, DEST_PANEL_RADIUS, 0x4208);
  printCentered("DESTINATIONS", CX, y0 + 20, 2, C_WHITE);
  drawDestClose();

  if (shown == 0) {
    printCentered("NO DESTINATIONS", CX, y0 + 150, 2, C_GREY);
    printCentered("Open app to sync", CX, y0 + 184, 1, C_GREY);
  } else {
    int startY = destListStartY(shown);
    for (int i = 0; i < shown; i++) {
      int iy = startY + i * (ih + gap);
      gfx->fillRoundRect(ix, iy, iw, ih, 12, 0x18E3);
      if (destItems[i].type == 0) {
        destFillStar(ix + 20, iy + ih / 2, 10, 0xFDE0);
      } else {
        aaDisc(ix + 20, iy + ih / 2, 8, C_CYAN);
      }
      if (imgs[13 + i].showW > 0 && imgs[13 + i].showH > 0) {
        int iy0 = iy + (ih - imgs[13 + i].showH) / 2;
        drawTextImg(13 + i, iy0, 0xFF);
      } else {
        printCentered(String("ITEM ") + (i + 1), CX, iy + (ih - 16) / 2, 2, C_WHITE);
      }
    }
  }

  printCentered("tap item to route / X closes", CX, y0 + DEST_PANEL_SIZE - 22, 1, C_GREY);
  needFlush = true;
}

void destTap(int x, int y) {
  Serial.printf("DEST tap x=%d y=%d open=%d\n", x, y, destSheetOpen ? 1 : 0);
  if (destSheetOpen) {
    if (destCloseHit(x, y)) {
      destSheetOpen = false;
      drawWaiting(waitSt);
      return;
    }
    int idx = destSheetItemAt(x, y);
    if (idx >= 0) {
      pendingEvt = String("T,DEST_GO,") + String(idx);
      destSheetOpen = false;
      drawWaiting(waitSt);
      return;
    }
    int x0 = destSheetX(), y0 = destSheetY();
    if (x < x0 || x > x0 + DEST_PANEL_SIZE || y < y0 || y > y0 + DEST_PANEL_SIZE) {
      destSheetOpen = false;
      drawWaiting(waitSt);
    }
    return;
  }
  if (destStarHit(x, y)) {
    destSheetOpen = true;
    pendingEvt = "T,DEST_Q";
    drawDestSheet();
  }
}

bool destHandleCommand(const String &m) {
  if (m == "#DEST,CLR") {
    destItemsClear();
    if (destSheetOpen) drawDestSheet();
    return true;
  }
  if (m.startsWith("#DEST,ADD,")) {
    int first = m.indexOf(',', 10);
    if (first > 0) {
      uint8_t type = (uint8_t)m.substring(10, first).toInt();
      int second = m.indexOf(',', first + 1);
      String name = (second > 0) ? m.substring(first + 1, second) : m.substring(first + 1);
      String addr = (second > 0) ? m.substring(second + 1) : "";
      destItemAdd(type, name.c_str(), addr.c_str());
      if (destSheetOpen) drawDestSheet();
    }
    return true;
  }
  return false;
}
