#pragma once
#include <Arduino.h>

// =========================================================================
// MotoNav Modern UI Framework (Glassmorphism & Anti-Aliased Vector Primitives)
// 为 M5Stopwatch 打造的现代毛玻璃与抗锯齿矢量 UI 渲染引擎
// =========================================================================

// 1. 半透明毛玻璃圆角矩形 (带有磨砂玻璃质感与边缘微光)
static inline void drawTranslucentRoundRect(int rx, int ry, int rw, int rh, int radius, uint16_t tintColor = 0x0000, uint8_t alpha = 185) {
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  const int W = LCD_WIDTH;
  const int H = LCD_HEIGHT;
  int invA = 256 - alpha;
  uint32_t tintRB = (tintColor & 0xF81F) * alpha;
  uint32_t tintG  = (tintColor & 0x07E0) * alpha;

  int y0 = max(0, ry);
  int y1 = min(H - 1, ry + rh - 1);
  int r2 = radius * radius;

  for (int y = y0; y <= y1; y++) {
    int dy = 0;
    if (y < ry + radius) dy = (ry + radius) - y;
    else if (y > ry + rh - 1 - radius) dy = y - (ry + rh - 1 - radius);

    int cornerInset = 0;
    if (dy > 0 && radius > 0) {
      int span = (int)sqrtf((float)max(0, r2 - dy * dy));
      cornerInset = radius - span;
    }

    int x0 = max(0, rx + cornerInset);
    int x1 = min(W - 1, rx + rw - 1 - cornerInset);
    if (x0 > x1) continue;

    uint16_t *row = &fb[y * W];
    for (int x = x0; x <= x1; x++) {
      uint16_t c = row[x];
      uint32_t rb = (c & 0xF81F) * invA + tintRB;
      uint32_t g  = (c & 0x07E0) * invA + tintG;
      row[x] = (uint16_t)(((rb >> 8) & 0xF81F) | ((g >> 8) & 0x07E0));
    }
  }
}

// 2. 半透明毛玻璃圆形底座
static inline void drawTranslucentCircle(int cx, int cy, int r, uint16_t tintColor = 0x0000, uint8_t alpha = 140) {
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  const int W = LCD_WIDTH;
  const int H = LCD_HEIGHT;
  int invA = 256 - alpha;
  uint32_t tintRB = (tintColor & 0xF81F) * alpha;
  uint32_t tintG  = (tintColor & 0x07E0) * alpha;

  int y0 = max(0, cy - r);
  int y1 = min(H - 1, cy + r);
  for (int y = y0; y <= y1; y++) {
    int dy = y - cy;
    int maxDx = (int)sqrtf((float)(r * r - dy * dy));
    int x0 = max(0, cx - maxDx);
    int x1 = min(W - 1, cx + maxDx);
    uint16_t *row = &fb[y * W];
    for (int x = x0; x <= x1; x++) {
      uint16_t c = row[x];
      uint32_t rb = (c & 0xF81F) * invA + tintRB;
      uint32_t g  = (c & 0x07E0) * invA + tintG;
      row[x] = (uint16_t)(((rb >> 8) & 0xF81F) | ((g >> 8) & 0x07E0));
    }
  }
}

// =========================================================================
// 现代 5x7 矢量抗锯齿字模库 (完全告别锯齿方块点阵)
// 支持 0-9, A-Z, :, %, -, +, ., 空格
// =========================================================================

static const uint8_t aaGlyphDigits[10][5] = {
  {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
  {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
  {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
  {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
  {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
  {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
};

static const uint8_t aaGlyphLetters[26][5] = {
  {0x7C, 0x12, 0x11, 0x12, 0x7C}, // A
  {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
  {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
  {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
  {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
  {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
  {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
  {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
  {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
  {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
  {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
  {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
  {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
  {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
  {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
  {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
  {0x46, 0x49, 0x49, 0x49, 0x31}, // S
  {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
  {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
  {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
  {0x63, 0x14, 0x08, 0x14, 0x63}, // X
  {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
  {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
};

static const uint8_t aaGlyphColon[5]   = {0x00, 0x36, 0x36, 0x00, 0x00}; // :
static const uint8_t aaGlyphPercent[5] = {0x63, 0x33, 0x18, 0x0C, 0x66}; // %
static const uint8_t aaGlyphMinus[5]   = {0x08, 0x08, 0x08, 0x08, 0x08}; // -
static const uint8_t aaGlyphPlus[5]    = {0x08, 0x08, 0x3E, 0x08, 0x08}; // +
static const uint8_t aaGlyphDot[5]     = {0x00, 0x60, 0x60, 0x00, 0x00}; // .
static const uint8_t aaGlyphDegree[5]  = {0x00, 0x06, 0x09, 0x06, 0x00}; // * or °

static inline const uint8_t* getAaGlyph(char c) {
  if (c >= '0' && c <= '9') return aaGlyphDigits[c - '0'];
  if (c >= 'A' && c <= 'Z') return aaGlyphLetters[c - 'A'];
  if (c >= 'a' && c <= 'z') return aaGlyphLetters[c - 'a'];
  if (c == ':') return aaGlyphColon;
  if (c == '%') return aaGlyphPercent;
  if (c == '-') return aaGlyphMinus;
  if (c == '+') return aaGlyphPlus;
  if (c == '.') return aaGlyphDot;
  if (c == '*' || c == '^') return aaGlyphDegree;
  return nullptr;
}

// 绘制单个抗锯齿矢量字符 (以 cx, cy 为中心)
static inline void drawAaChar(float cx, float cy, const uint8_t bmp[5], float scale, float dotR, uint16_t color) {
  if (!bmp) return;
  for (int col = 0; col < 5; col++) {
    uint8_t b = bmp[col];
    for (int row = 0; row < 7; row++) {
      if ((b >> row) & 1) {
        float px = cx + (col - 2.0f) * scale;
        float py = cy + (row - 3.0f) * scale;
        aaDisc(px, py, dotR, color);
      }
    }
  }
}

// 绘制抗锯齿字符串 (支持居中或左对齐，完全告别锯齿方块点阵)
static inline void drawAaString(float cx, float cy, const char *str, float scale, float dotR, float spacing, uint16_t color, bool center = true) {
  if (!str) return;
  int len = strlen(str);
  if (len == 0) return;
  float totalW = (len - 1) * spacing;
  float startX = center ? (cx - totalW / 2.0f) : cx;

  for (int i = 0; i < len; i++) {
    char ch = str[i];
    if (ch == ' ') continue;
    const uint8_t *bmp = getAaGlyph(ch);
    if (bmp) {
      drawAaChar(startX + i * spacing, cy, bmp, scale, dotR, color);
    }
  }
}

// 3. 经典 Rockstar (R星) 风格五角星绘制函数
// cx, cy: 中心坐标, outerR: 外顶点半径, innerR: 内凹点半径
static inline void drawRockstarStar(int cx, int cy, float outerR, float innerR, uint16_t fillColor, uint16_t outlineColor) {
  float ptsX[10];
  float ptsY[10];
  for (int i = 0; i < 10; i++) {
    float angle = (-90.0f + i * 36.0f) * (float)M_PI / 180.0f;
    float r = (i % 2 == 0) ? outerR : innerR;
    ptsX[i] = cx + r * cosf(angle);
    ptsY[i] = cy + r * sinf(angle);
  }

  // 扇形三角形拼合填充五角星实心内部
  for (int i = 0; i < 10; i++) {
    int next = (i + 1) % 10;
    gfx->fillTriangle((int)lroundf(ptsX[i]), (int)lroundf(ptsY[i]),
                      (int)lroundf(ptsX[next]), (int)lroundf(ptsY[next]),
                      cx, cy, fillColor);
  }

  // 轮廓线条平滑描边
  for (int i = 0; i < 10; i++) {
    int next = (i + 1) % 10;
    gfx->drawLine((int)lroundf(ptsX[i]), (int)lroundf(ptsY[i]),
                  (int)lroundf(ptsX[next]), (int)lroundf(ptsY[next]), outlineColor);
  }
  // 5 个外顶点抗锯齿平滑小圆点加固
  for (int i = 0; i < 10; i += 2) {
    aaDisc(ptsX[i], ptsY[i], 1.2f, outlineColor);
  }
}

// 4. 动态主题地图背景生成 (根据用户选择的地图主题 themeIdx 和 nightOn，动态呈现对应风格的精美地图底图)
