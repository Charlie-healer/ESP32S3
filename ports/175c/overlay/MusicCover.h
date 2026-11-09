#pragma once
// Album cover receiver for the 1.75C port.
// The phone already sends TYPE_COVER = 6 using the StopWatch cover packet:
// 180x180, 16-color RGB565 palette, 4-bit RLE + Deflate.

#include <Arduino.h>
#include <miniz.h>

#define COVER_SIZE 180
#define COVER_BUF_BYTES (COVER_SIZE * COVER_SIZE * 2)
#define COVER_MAX_BYTES (48 * 1024)

extern String pendingAck;

uint16_t *coverBuf = nullptr;
bool coverValid = false;
uint16_t coverPalette[16];

static tinfl_decompressor *coverInflater = nullptr;

bool parseCover(const uint8_t *d, uint32_t total) {
  if (!d || total < 42) return false;
  int w = d[0] | (d[1] << 8);
  int h = d[2] | (d[3] << 8);
  if (w <= 0 || w > COVER_SIZE || h <= 0 || h > COVER_SIZE) return false;

  for (int i = 0; i < 16; i++) {
    coverPalette[i] = d[6 + i * 2] | (d[7 + i * 2] << 8);
  }

  uint32_t rawRleLen = (uint32_t)d[38] | ((uint32_t)d[39] << 8) |
                       ((uint32_t)d[40] << 16) | ((uint32_t)d[41] << 24);
  if (rawRleLen == 0 || rawRleLen > (uint32_t)w * h * 2) return false;

  size_t packed = total - 42;
  uint8_t *rleBuf = (uint8_t *)ps_malloc(rawRleLen);
  if (!rleBuf) return false;

  if (!coverInflater) coverInflater = (tinfl_decompressor *)malloc(sizeof(tinfl_decompressor));
  if (!coverInflater) { free(rleBuf); return false; }
  tinfl_init(coverInflater);

  size_t inBytes = packed, outBytes = rawRleLen;
  tinfl_status res = tinfl_decompress(coverInflater, d + 42, &inBytes,
      rleBuf, rleBuf, &outBytes,
      TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
  if (res != TINFL_STATUS_DONE && outBytes != rawRleLen) {
    tinfl_init(coverInflater);
    inBytes = packed; outBytes = rawRleLen;
    res = tinfl_decompress(coverInflater, d + 42, &inBytes,
        rleBuf, rleBuf, &outBytes,
        TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
  }
  if (outBytes != rawRleLen) { free(rleBuf); return false; }

  if (!coverBuf) coverBuf = (uint16_t *)ps_calloc(1, COVER_BUF_BYTES);
  if (!coverBuf) { free(rleBuf); return false; }

  int totalPix = w * h;
  int outIdx = 0;
  size_t p = 0;
  while (p < outBytes && outIdx < totalPix) {
    uint8_t tag = rleBuf[p++];
    if (tag < 16) {
      coverBuf[outIdx++] = coverPalette[tag];
    } else {
      int cIdx = tag & 0x0F;
      int len = ((tag >> 4) & 0x0F) + 1;
      if (len == 16 && p < outBytes) len += rleBuf[p++];
      uint16_t c = coverPalette[cIdx];
      for (int k = 0; k < len && outIdx < totalPix; k++) coverBuf[outIdx++] = c;
    }
  }

  free(rleBuf);
  if (outIdx >= totalPix) {
    coverValid = true;
    musicDirty = true;
    return true;
  }
  return false;
}

static uint32_t coverRd32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

uint8_t *coverRecv = nullptr;
int coverRecvId = -1;
uint32_t coverRecvTotal = 0;
uint16_t coverRecvParts = 0, coverRecvGot = 0;
uint8_t coverRecvMask[512];
bool coverRecvDone = false;
bool coverRecvOk = false;

void handleCoverBulk(const uint8_t *d, int n) {
  if (n < 15) return;
  int id = d[2];
  uint16_t parts = d[3] | (d[4] << 8);
  uint16_t seq = d[5] | (d[6] << 8);
  uint32_t total = coverRd32(d + 7);
  uint32_t off = coverRd32(d + 11);
  if (total == 0 || total > COVER_MAX_BYTES || parts == 0 || parts > 4096 || seq >= parts) return;

  if (!coverRecv) coverRecv = (uint8_t *)ps_malloc(COVER_MAX_BYTES);
  if (!coverRecv) return;

  if (id != coverRecvId || total != coverRecvTotal || parts != coverRecvParts) {
    coverRecvId = id;
    coverRecvTotal = total;
    coverRecvParts = parts;
    coverRecvGot = 0;
    coverRecvDone = false;
    coverRecvOk = false;
    memset(coverRecvMask, 0, sizeof(coverRecvMask));
  }
  if (coverRecvDone) {
    pendingAck = String(coverRecvOk ? "K,6," : "N,6,") + String(id);
    return;
  }

  uint32_t len = n - 15;
  if (off >= total) return;
  if (off + len > total) len = total - off;
  if (!(coverRecvMask[seq >> 3] & (1 << (seq & 7)))) {
    memcpy(coverRecv + off, d + 15, len);
    coverRecvMask[seq >> 3] |= (1 << (seq & 7));
    coverRecvGot++;
  }

  if (coverRecvGot >= coverRecvParts) {
    coverRecvDone = true;
    coverRecvOk = parseCover(coverRecv, total);
    pendingAck = String(coverRecvOk ? "K,6," : "N,6,") + String(id);
    if (coverRecvOk) musicDirty = true;
  }
}
