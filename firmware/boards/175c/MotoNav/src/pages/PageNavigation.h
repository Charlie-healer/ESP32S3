#pragma once
#include "../../MusicCover.h"
// 音乐页面、手势换页和数据中断提示。由 MotoNav.ino 按依赖顺序包含，仅编译一次。
// ---- v5.5 跟手换页 ----
// 触摸坐标和画面方向相反时改成 1（拖动时画面往手指反方向走就是反了）
#define TOUCH_FLIP_X 1      // v5.5.1 实测左右是反的
#define TOUCH_FLIP_Y 1      // 实机坐标确认：上下也反向；点按和手势使用同一校正
#define DRAG_START  12        // 手指移动超过这么多像素才开始拖
uint16_t *slideOld = nullptr, *slideNew = nullptr;
int16_t chordHalf[LCD_HEIGHT];   // 每一行在圆里的半宽（圆外不用算）
bool chordReady = false;
void drawNavFrame();

// v7.8 音乐页：歌名/歌手图片栏 5/6；#Z,有会话,播放中,按钮位掩码。
bool musicLiked = false;
int musicVolume = 50;
int musicDuration = 0;
int musicPos = 0;
uint32_t musicPosAt = 0;
int musicRepeat = 0;
uint32_t musicCoverTapAt = 0;
uint8_t musicCoverTapCount = 0;
bool musicCoverTapPending = false;

void drawMusicHeart() {
  const int hx = 73, hy = 258, r = 9;
  uint16_t c = musicLiked ? 0xF800 : 0x8410;
  aaDisc(hx - r / 2, hy - r / 3, r / 2, c);
  aaDisc(hx + r / 2, hy - r / 3, r / 2, c);
  gfx->fillTriangle(hx - r, hy, hx + r, hy, hx, hy + r, c);
  printCentered("LIKE", hx, hy + 14, 1, C_GREY);
}

void drawMusicVolume() {
  const int bx = 414, by = 124, bw = 12, bh = 150;
  gfx->fillRoundRect(bx, by, bw, bh, 6, 0x1082);
  int fill = (musicVolume * bh) / 100;
  if (fill > 0) gfx->fillRoundRect(bx, by + bh - fill, bw, fill, 6, C_CYAN);
  gfx->drawRoundRect(bx, by, bw, bh, 6, 0x4208);
  printCentered("VOL", bx + bw / 2, by + bh + 6, 1, C_GREY);
}


// Album cover background: stretch 180x180 to 466x466 and dim to ~28%.
static void drawCoverBackgroundDimmed() {
  if (!coverValid || coverBuf == nullptr) { gfx->fillScreen(C_BLACK); return; }
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) { gfx->fillScreen(C_BLACK); return; }
  const int W = LCD_WIDTH;
  const int H = LCD_HEIGHT;
  for (int y = 0; y < H; y++) {
    int srcY = (y * 180) / H;
    const uint16_t *srcRow = &coverBuf[srcY * 180];
    uint16_t *dstRow = &fb[y * W];
    for (int x = 0; x < W; x++) {
      int srcX = (x * 180) / W;
      uint16_t c = srcRow[srcX];
      uint32_t rb = (c & 0xF81F) * 70;
      uint32_t g  = (c & 0x07E0) * 70;
      dstRow[x] = (uint16_t)(((rb >> 8) & 0xF81F) | ((g >> 8) & 0x07E0));
    }
  }
}

// Draw the 180x180 cover at dstSize.
static void drawScaledCover(int dstX, int dstY, int dstSize) {
  if (!coverBuf) return;
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  const int W = LCD_WIDTH, H = LCD_HEIGHT, srcSize = 180;
  for (int y = 0; y < dstSize; y++) {
    int py = dstY + y;
    if (py < 0 || py >= H) continue;
    int srcY = (y * srcSize) / dstSize;
    const uint16_t *srcRow = &coverBuf[srcY * srcSize];
    uint16_t *dstRow = &fb[py * W];
    for (int x = 0; x < dstSize; x++) {
      int px = dstX + x;
      if (px < 0 || px >= W) continue;
      int srcX = (x * srcSize) / dstSize;
      dstRow[px] = srcRow[srcX];
    }
  }
}

static void drawTextImgAt(int slot, int x0, int y0, uint8_t tint) {
  TextImg &im = imgs[slot];
  if (!im.show || im.showW == 0 || im.showH == 0) return;
  int bpr = (im.showW + 1) / 2;
  for (int y = 0; y < im.showH; y++) {
    for (int x = 0; x < im.showW; x++) {
      uint8_t a = imgNib(im, bpr, x, y);
      if (a) gfx->drawPixel(x0 + x, y0 + y, greyColor(a, tint));
    }
  }
}

void drawMusic() {
  if (coverValid && coverBuf) drawCoverBackgroundDimmed(); else gfx->fillScreen(C_BLACK);
  onWaitScreen = false;
  frameIsMap = false;
  printCentered("MUSIC", CX, 65, 4, C_CYAN);
  if (coverValid && coverBuf) drawScaledCover(55, 144, 82);
  bool linked = bleConnected && bleAuthed;
  if (hidMusicMode && linked && !appleMedia.available) {
    printCentered("PHONE MEDIA KEYS", CX, 158, 2, C_WHITE);
    printCentered("OPEN MUSIC APP ON PHONE", CX, 210, 2, C_GREY);
  } else if (linked) {
    int infoX = 150, infoY = 144;
    if (imgs[5].showW > 0) drawTextImgAt(5, infoX, infoY, 0xFF);
    int infoY2 = infoY + (imgs[5].showH ? imgs[5].showH + 6 : 28);
    if (imgs[6].showW > 0) drawTextImgAt(6, infoX, infoY2, 0xBB);
  } else printCentered("PHONE OFFLINE", CX, 170, 3, C_GREY);
  bool showLyrics = linked && hidMusicMode && musicLyrics && appleMedia.available && imgs[12].showW > 0;
  if (showLyrics) drawTextImg(12, 268, 0xDD);
  const char *feedback[] = {"", "SENT", "PHONE RECEIVED", "DONE", "NO RESPONSE", "NO PLAYER", "HID SENT", "HID NOT READY", "HID SEND FAILED", "AMS SENT", "AMS SEND FAILED", "AMS QUEUED"};
  String state = linked && musicHave ? (musicPlaying ? "PLAYING" : "PAUSED") : "NO PLAYER";
  if (hidMusicMode) {
    state = !hidMediaReady() ? (appleMedia.controlMask() ? "AMS MEDIA READY" : "PAIR HID IN PHONE SETTINGS") : !appleMedia.available ? "HID READY / AMS UNAVAILABLE"
      : !appleMedia.playback.known ? "STATE UNKNOWN" : appleMedia.playback.state == 1 ? "PLAYING" : appleMedia.playback.state == 0 ? "PAUSED" : "SEEKING";
  }
  if (musicAck > 0 && musicAck <= 11) state = feedback[musicAck];
  printCentered(state, CX, showLyrics ? 300 : 290, 2, musicAck == 4 ? C_ORANGE : C_GREY);
  for (int i = 0; i < 3; i++) {
    int x = CX + (i - 1) * 100, y = 355;
    bool enabled = hidMusicMode ? (hidMediaReady() || appleMedia.controlReady(i)) : linked && musicHave;
    uint16_t c = enabled ? C_WHITE : C_GREY;
    aaDisc(x, y, 38, enabled && musicPressed == i ? 0x0470 : enabled ? 0x19A7 : 0x1082);
    if (i == 1) {
      if (hidMusicMode && !appleMedia.playback.known) {
        gfx->fillTriangle(x - 19, y - 15, x - 19, y + 15, x + 1, y, c);
        gfx->fillRect(x + 7, y - 15, 5, 30, c); gfx->fillRect(x + 16, y - 15, 5, 30, c);
      } else if (musicPlaying) {
        gfx->fillRect(x - 13, y - 15, 8, 30, c);
        gfx->fillRect(x + 5, y - 15, 8, 30, c);
      } else gfx->fillTriangle(x - 10, y - 17, x - 10, y + 17, x + 17, y, c);
    } else {
      int sign = i == 0 ? -1 : 1;
      gfx->fillTriangle(x - sign * 12, y - 15, x - sign * 12, y + 15, x + sign * 10, y, c);
      gfx->fillRect(x + sign * 15 - 3, y - 15, 6, 30, c);
    }
  }
  if (hidMusicMode && appleMedia.available) {
    double duration = AppleMediaData::duration(appleMedia.values[3].text.c_str());
    if (duration > 0 && appleMedia.playback.known) {
      double elapsed = appleMedia.playback.elapsed + appleMedia.playback.rate * (millis() - appleMedia.playbackAt) / 1000.0;
      int sec = (int)constrain(elapsed, 0.0, duration), total = (int)duration;
      char time[32]; snprintf(time, sizeof(time), "%d:%02d / %d:%02d", sec/60, sec%60, total/60, total%60);
      printCentered(time, CX, 242, 2, C_GREY);
    }
  } else if (musicDuration > 0) {
    int elapsed = musicPos + (musicPlaying ? (int)((millis() - musicPosAt) / 1000UL) : 0);
    if (elapsed < 0) elapsed = 0;
    if (elapsed > musicDuration) elapsed = musicDuration;
    int x0 = 55, x1 = 385, y = 232;
    gfx->fillRoundRect(x0, y, x1 - x0, 5, 2, 0x1082);
    int fw = (x1 - x0) * elapsed / musicDuration;
    if (fw > 0) gfx->fillRoundRect(x0, y, fw, 5, 2, C_CYAN);
    char time[32]; snprintf(time, sizeof(time), "%d:%02d / %d:%02d", elapsed/60, elapsed%60, musicDuration/60, musicDuration%60);
    printCentered(time, CX, 242, 1, C_GREY);
    const char *rm = musicRepeat == 1 ? "ONE" : musicRepeat == 2 ? "SHUF" : "LOOP";
    printCentered(String("R ") + rm, 398, 258, 1, C_GREY);
    int rx = 398, ry = 240;
    aaDisc(rx, ry, 13, 0x18E3);
    printCentered("R", rx, ry - 7, 2, C_WHITE);
  } else printCentered("SWIPE LEFT TO RETURN", CX, 425, 1, C_GREY);
  drawMusicHeart();
  drawMusicVolume();
  drawPageDots();
  needFlush = true;
}

// 拖动状态
bool dragging = false;
int dragTarget = 0, dragSign = 1;
int dragWeatherDetail = 0;
bool dragHoriz = true;
float dragP = 0, dragV = 0;                 // 新页面已经进来多少像素；速度（像素/毫秒）
unsigned long dragT = 0;

uint16_t *shownFrame() {
  if (fbBuf[1] && flushSrc) return flushSrc;
  return gfx->getFramebuffer();
}

static inline uint16_t dim565(uint16_t c, uint32_t k) {   // k: 0~32，32 = 原样
  uint32_t r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
  return (uint16_t)(((r * k >> 5) << 11) | ((g * k >> 5) << 5) | (b * k >> 5));
}

// 画一个页面到画布（不改当前页）
void renderPage(int p, int detail) {
  int keep = page;
  int keepDetail = wxDetail;
  bool ks = sliding;
  page = p; sliding = false;
  if (p == 2) drawMusic();
  else if (p == 1) { wxDetail = detail; drawWeather(); }
  else if (inNav) drawNavFrame();
  else drawWaiting(bleConnected ? WAIT_READY : WAIT_NOLINK);
  page = keep; wxDetail = keepDetail; sliding = ks;
}

// 准备滑动：存下屏幕上的旧画面，在画布上画好目标页面存起来
bool slidePrepare(int target, int detail) {
  const size_t BYTES = (size_t)LCD_WIDTH * LCD_HEIGHT * 2;
  if (!slideOld) slideOld = (uint16_t *)heap_caps_malloc(BYTES, MALLOC_CAP_SPIRAM);
  if (!slideNew) slideNew = (uint16_t *)heap_caps_malloc(BYTES, MALLOC_CAP_SPIRAM);
  if (!slideOld || !slideNew) return false;
  if (!chordReady) {
    for (int y = 0; y < LCD_HEIGHT; y++) {
      float d = y + 0.5f - LCD_HEIGHT / 2.0f, r = LCD_WIDTH / 2.0f + 1;
      chordHalf[y] = d * d < r * r ? (int16_t)ceilf(sqrtf(r * r - d * d)) : 0;
    }
    chordReady = true;
  }
  if (fbBuf[1]) { flushWait(); xSemaphoreGive(flushDone); }   // 等正在推的那帧推完，旧画面才完整
  memcpy(slideOld, shownFrame(), BYTES);
  renderPage(target, detail);
  memcpy(slideNew, gfx->getFramebuffer(), BYTES);
  sliding = true;
  needFlush = false; frameIsMap = false;
  return true;
}

// 合成一帧：新页面已经进来 p 像素（盖在上面），旧页面往同方向退 30%、变暗，交界处一道阴影
void slideCompose(int p) {
  const int W = LCD_WIDTH, H = LCD_HEIGHT;
  if (p < 0) p = 0;
  if (p > (dragHoriz ? W : H)) p = dragHoriz ? W : H;
  uint16_t *fb = gfx->getFramebuffer();
  int full = dragHoriz ? W : H;
  int q = p * 3 / 10;                                   // 旧页面退后的距离
  uint32_t k0 = 32 - (uint32_t)(p * 13 / full);         // 旧页面整体亮度：100% → 约 60%
  const int SH = 22;                                    // 阴影宽度
  if (dragHoriz) {
    static uint8_t colK[LCD_WIDTH];
    for (int x = 0; x < W; x++) {
      int dist = dragSign > 0 ? x - p : (W - p - 1) - x;   // 离交界多远（旧页面这一侧）
      uint32_t k = k0;
      if (p > 0 && dist < SH) k = k * (14 + dist * 18 / SH) / 32;
      colK[x] = (uint8_t)k;
    }
    for (int y = 0; y < H; y++) {
      int h = chordHalf[y];
      if (!h) continue;
      int xa = max(0, W / 2 - h), xb = min(W, W / 2 + h);
      uint16_t *row = fb + (size_t)y * W;
      const uint16_t *o = slideOld + (size_t)y * W, *n = slideNew + (size_t)y * W;
      if (dragSign > 0) {                               // 新页面从左边进来
        int a = xa, b = min(p, xb);
        if (b > a) memcpy(row + a, n + (W - p) + a, (b - a) * 2);
        for (int x = max(p, xa); x < xb; x++) row[x] = colK[x] >= 32 ? o[x - q] : dim565(o[x - q], colK[x]);
      } else {                                          // 新页面从右边进来
        int a = max(W - p, xa), b = xb;
        if (b > a) memcpy(row + a, n + (a - (W - p)), (b - a) * 2);
        for (int x = xa; x < min(W - p, xb); x++) row[x] = colK[x] >= 32 ? o[x + q] : dim565(o[x + q], colK[x]);
      }
      // 新页面前沿一道细亮边，看得出是一张“卡片”盖过来
      if (p > 1 && p < W - 1) {
        int ex = dragSign > 0 ? p - 1 : W - p;
        if (ex >= xa && ex < xb) row[ex] = 0x4A69;
        int ex2 = ex + (dragSign > 0 ? -1 : 1);
        if (ex2 >= xa && ex2 < xb) row[ex2] = 0x2945;
      }
    }
  } else {
    for (int y = 0; y < H; y++) {
      int h = chordHalf[y];
      if (!h) continue;
      int xa = max(0, W / 2 - h), xb = min(W, W / 2 + h);
      uint16_t *row = fb + (size_t)y * W;
      bool isNew = dragSign > 0 ? y < p : y >= H - p;
      if (isNew) {
        int sy = dragSign > 0 ? y + (H - p) : y - (H - p);
        memcpy(row + xa, slideNew + (size_t)sy * W + xa, (xb - xa) * 2);
        int edge = dragSign > 0 ? p - 1 : H - p;                // 前沿细亮边
        if (p > 1 && p < H - 1 && (y == edge || y == edge + (dragSign > 0 ? -1 : 1))) {
          uint16_t ec = y == edge ? 0x4A69 : 0x2945;
          for (int x = xa; x < xb; x++) row[x] = ec;
        }
      } else {
        int sy = dragSign > 0 ? y - q : y + q;
        int dist = dragSign > 0 ? y - p : (H - p - 1) - y;
        uint32_t k = k0;
        if (p > 0 && dist < SH) k = k * (14 + dist * 18 / SH) / 32;
        const uint16_t *o = slideOld + (size_t)sy * W;
        for (int x = xa; x < xb; x++) row[x] = k >= 32 ? o[x] : dim565(o[x], k);
      }
    }
  }
  flushAsync();                                         // 两块画布轮流：推这一帧的同时画下一帧
  needFlush = false; frameIsMap = false;
}

// 换页完成
void slideCommit() {
  const size_t BYTES = (size_t)LCD_WIDTH * LCD_HEIGHT * 2;
  sliding = false; dragging = false;
  page = dragTarget;
  if (page == 1) wxDetail = dragWeatherDetail;
  pageAt = millis();
  memcpy(gfx->getFramebuffer(), slideNew, BYTES);
  needFlush = true; frameIsMap = false;
  if (page == 2) {
    onWaitScreen = false; musicDirty = true; pendingEvt = "T,M,Q";
  } else if (page == 1) {
    onWaitScreen = false;
    wxDirty = false;
    if (!wxHave || millis() - wxAt > 15UL * 60 * 1000) { pendingEvt = "T,W"; wxAskAt = millis(); }
  } else {
    if (inNav) navDirty = true;
    else { riderPrevF = -1; riderPrevZ = -2; catForce = true; }
  }
}

// 没换成：回到原页面
void slideCancel() {
  sliding = false; dragging = false;
  renderPage(page, wxDetail);
  if (page == 2) musicDirty = true;
  else if (page == 1) wxDirty = false;
  else if (inNav) navDirty = true;
  else { onWaitScreen = true; catForce = true; }
  needFlush = true;
}

// 从 p0 顺势滑到 p1（先快后慢）；ms = 时长
void slideAnimate(float p0, float p1, int ms) {
  unsigned long t0 = millis();
  int frames = 0;
  for (;;) {
    float t = (millis() - t0) / (float)ms;
    if (t >= 1) break;
    float e = 1 - (1 - t) * (1 - t);                   // quadratic ease-out
    slideCompose((int)(p0 + (p1 - p0) * e));
    ++frames;
  }
  slideCompose((int)p1);
  Serial.printf("SLIDE ms=%d frames=%d actual=%lu\n", ms, frames, millis() - t0);
}

// 触摸向量统一换算：先修正芯片坐标的镜像，再按刷屏的逆旋转映射到画布。
// 位移和相对屏幕中心的点都使用这一个函数，避免旋转后点按与手势方向不一致。
void touchVectorToCanvas(float &dx, float &dy) {
  if (TOUCH_FLIP_X) dx = -dx;
  if (TOUCH_FLIP_Y) dy = -dy;
  float ox = dx, oy = dy;
  if (rotFree) { dx = ox * rotCosF + oy * rotSinF; dy = -ox * rotSinF + oy * rotCosF; }
  else if (screenRot == 1) { dx = oy; dy = -ox; }
  else if (screenRot == 2) { dx = -ox; dy = -oy; }
  else if (screenRot == 3) { dx = -oy; dy = ox; }
}

// 手指在拖：(dx, dy) = 从按下到现在的位移
void dragMove(int dx, int dy, unsigned long now) {
  float vx = dx, vy = dy;
  touchVectorToCanvas(vx, vy);
  dx = (int)lroundf(vx); dy = (int)lroundf(vy);
  if (!dragging) {
    int threshold = page == 2 ? 18 : DRAG_START;
    if (abs(dx) < threshold && abs(dy) < threshold) return;
    dragHoriz = abs(dx) >= abs(dy);
    int d = dragHoriz ? dx : dy;
    dragSign = d > 0 ? 1 : -1;
    const auto next = FixedPageLayout::move(page, wxDetail, dragHoriz, dragSign);
    if (!next.allowed) return;
    dragTarget = next.page; dragWeatherDetail = next.detail;
    if (!slidePrepare(dragTarget, dragWeatherDetail)) return;
    dragging = true;
    dragP = 0; dragV = 0; dragT = now;
  }
  float p = (dragHoriz ? dx : dy) * dragSign;
  if (p < 0) p = 0;
  float dt = now - dragT;
  if (dt > 0) dragV = dragV * 0.5f + ((p - dragP) / dt) * 0.5f;
  dragP = p; dragT = now;
  slideCompose((int)p);
}

// 松手：拖过 15% 或明显前甩就完成换页；短回弹也更快。
void dragEnd() {
  if (!dragging) return;
  int full = dragHoriz ? LCD_WIDTH : LCD_HEIGHT;
  bool go = dragP > full * 0.15f || (dragV > 0.20f && dragP > 16);
  if (dragV < -0.20f) go = false;                       // 往回甩：取消
  float target = go ? full : 0;
  float left = fabsf(target - dragP);
  int ms = 220 + (int)(130 * left / full);
  if (go && dragV > 0.45f) ms = max(170, (int)(ms * 0.80f));
  slideAnimate(dragP, target, ms);
  if (go) slideCommit(); else slideCancel();
}

// 来电等情况：直接放弃拖动
void dragAbort() {
  if (!dragging) return;
  sliding = false; dragging = false;
  if (page == 2) musicDirty = true; else if (page == 1) wxDirty = true; else if (inNav) navDirty = true;
}

// 将触摸点换算到画布坐标，按钮使用较大的点按范围。
int musicButtonAt(int x, int y) {
  float dx = x - CX, dy = y - CX;
  touchVectorToCanvas(dx, dy);
  x = lroundf(CX + dx); y = lroundf(CX + dy);
  for (int i = 0; i < 3; i++) {
    int bx = CX + (i - 1) * 100;
    if ((x - bx) * (x - bx) + (y - 355) * (y - 355) <= 44 * 44) return i;
  }
  return -1;
}

void musicTap(int x, int y) {
  if (dragging || page != 2 || callOn || pairOn || !bleConnected || !bleAuthed) return;
  float mdx = x - CX, mdy = y - CX;
  touchVectorToCanvas(mdx, mdy);
  int mx = lroundf(CX + mdx), my = lroundf(CX + mdy);

  if (!hidMusicMode && musicDuration > 0) {
    int rdx = mx - 398, rdy = my - 240;
    if (rdx * rdx + rdy * rdy <= 24 * 24) {
      musicRepeat = (musicRepeat + 1) % 3;
      pendingEvt = String("T,M,R,") + musicRepeat;
      musicAck = 1; musicAckUntil = millis() + 2500;
      musicDirty = true;
      return;
    }
  }
  int hdx = mx - 73, hdy = my - 258;
  if (hdx * hdx + hdy * hdy <= 26 * 26) {
    musicLiked = !musicLiked;
    pendingEvt = String("T,M,L,") + (musicLiked ? 1 : 0);
    musicDirty = true;
    return;
  }
  if (mx >= 404 && mx <= 436 && my >= 118 && my <= 300) {
    int vol = (300 - my) * 100 / 182;
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    musicVolume = vol;
    pendingEvt = String("T,M,V,") + vol;
    musicDirty = true;
    return;
  }

  int button = musicButtonAt(x, y);
  if (hidMusicMode) {
    if (button < 0) return;
    bool hidReady = hidMediaReady();
    if (!hidReady && !appleMedia.controlReady(button)) { musicAck = 7; musicAckUntil = millis() + 2500; musicDirty = true; return; }
    if (hidTapAt && millis() - hidTapAt < 120) return;
    hidTapAt = millis();
    if (!hidReady) {
      musicAck = appleMedia.control(button) ? 11 : 10; musicAckUntil = millis() + 2500; musicDirty = true; return;
    }
    if (!hidPulses.enqueue(button)) { musicAck = 8; musicAckUntil = millis() + 2500; musicDirty = true; }
    return; // no T,M,P/T/N: do not also control Apple Music through the App
  }
  if (button < 0 || !musicHave) {
    pendingEvt = String("T,M,H,") + x + "," + y + "," + button;
    return;
  }
  pendingEvt = String("T,M,") + (button == 0 ? "P" : button == 1 ? "T" : "N") + "," + x + "," + y + "," + button;
  musicAck = 1; musicAckUntil = millis() + 2500; musicDirty = true;
}


void musicCoverAction(int dir) {
  if (page != 2 || callOn || pairOn || !bleConnected || !bleAuthed) return;
  if (hidMusicMode) {
    int button = dir > 0 ? 2 : 0;
    if (!hidMediaReady() && !appleMedia.controlReady(button)) {
      musicAck = 7; musicAckUntil = millis() + 2500; musicDirty = true; return;
    }
    if (!hidMediaReady()) {
      musicAck = appleMedia.control(button) ? 11 : 10;
      musicAckUntil = millis() + 2500; musicDirty = true; return;
    }
    hidPulses.enqueue(button);
    return;
  }
  if (!musicHave) { musicAck = 5; musicAckUntil = millis() + 2500; musicDirty = true; return; }
  pendingEvt = String("T,M,") + (dir > 0 ? "N" : "P") + ",0,0,-1";
  musicAck = 1; musicAckUntil = millis() + 2500; musicDirty = true;
}

void musicCoverTap() {
  if (page != 2 || callOn || pairOn || !bleConnected || !bleAuthed) return;
  if (hidMusicMode) {
    if (!hidMediaReady() && !appleMedia.controlReady(1)) {
      musicAck = 7; musicAckUntil = millis() + 2500; musicDirty = true; return;
    }
    if (!hidMediaReady()) { musicAck = appleMedia.control(1) ? 11 : 10; musicAckUntil = millis() + 2500; musicDirty = true; return; }
    hidPulses.enqueue(1);
    return;
  }
  const uint32_t now = millis();
  if (now - musicCoverTapAt > 360) musicCoverTapCount = 0;
  ++musicCoverTapCount;
  musicCoverTapAt = now;
  if (musicCoverTapCount >= 2) {
    musicCoverTapCount = 0;
    musicCoverTapPending = false;
    musicLiked = !musicLiked;
    pendingEvt = String("T,M,L,") + (musicLiked ? 1 : 0);
    musicAck = 1; musicAckUntil = millis() + 2500; musicDirty = true;
  } else {
    musicCoverTapPending = true;
  }
}

// 固定页面不因导航距离自动跳转；音乐回执照常刷新。
void musicTick() {
  if (screenDark) return;
  if (musicAckUntil && (long)(millis() - musicAckUntil) >= 0) {
    if (musicAck == 1) { musicAck = 4; musicAckUntil = millis() + 2500; }
    else { musicAck = 0; musicAckUntil = 0; }
    musicDirty = true;
  }
  if (musicCoverTapPending && millis() - musicCoverTapAt >= 360) {
    musicCoverTapPending = false;
    if (musicCoverTapCount == 1 && !hidMusicMode && musicHave) {
      pendingEvt = "T,M,T,0,0,-1";
      musicAck = 1; musicAckUntil = millis() + 2500;
    }
    musicCoverTapCount = 0;
    musicDirty = true;
  }
  if (page != 2 || callOn || pairOn || sliding) return;
  static uint32_t progressAt = 0;
  if (musicPlaying && musicDuration > 0 && millis() - progressAt >= 1000) {
    progressAt = millis(); musicDirty = true;
  }
  if (musicDirty) { musicDirty = false; drawMusic(); }
}

// 天气列共享请求和数据；保持用户手动选中的页面。
void weatherTick() {
  if (screenDark) return;
  if (page != 1 || callOn || pairOn || sliding) return;
  unsigned long now = millis();
  // 还没数据或数据旧了：30 秒再要一次
  if (bleConnected && (!wxHave || now - wxAt > 15UL * 60 * 1000) && now - wxAskAt > 30000) { pendingEvt = "T,W"; wxAskAt = now; }
  if (wxDirty) { wxDirty = false; drawWeather(); }
}

void drawStaleOverlay() {
  drawRedRing();
  printCentered("NO DATA", CX, 412, 3, C_RED);
  needFlush = true;
}
