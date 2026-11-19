#pragma once
// Wrist raise/lower detection for the 1.75C's QMI8658.
//
// The algorithm is the plain-C++ state machine from muki01/Smartwatch-ESP32
// (MIT), fed in the screen frame:
//   x = right, y = toward 12 o'clock, z = out of the screen.
//
// The 1.75C panel is mounted 90 degrees from the sensor: sensor Y is the
// screen's horizontal axis, sensor X is vertical. LeanMeter::leanRead()
// already computes the raw axes; it calls wristFeed() so this file does not
// open a second I2C transaction.

#include <math.h>
#include <stdint.h>

class WristRaise {
 public:
  enum Event : uint8_t { NONE = 0, RAISE = 1, LOWER = 2 };

  static constexpr float VIEW_Z = 0.45f;
  static constexpr float VIEW_Y_MIN = -0.30f;
  static constexpr float VIEW_X_MAX = 0.75f;
  static constexpr float AWAY_Z = 0.20f;
  static constexpr float AWAY_X = 0.85f;
  static constexpr float AWAY_Y = -0.55f;
  static constexpr float STEADY_G = 0.12f;
  static constexpr float RAISE_Y_MIN = 0.05f;
  static constexpr float TILT_Y_RISE = 0.25f;
  static constexpr float TILT_Y_MIN = 0.25f;
  static constexpr uint32_t HOLD_MS = 160;
  static constexpr uint32_t RAISE_WINDOW_MS = 1500;
  static constexpr uint32_t TILT_WINDOW_MS = 800;
  static constexpr uint32_t LOWER_MS = 600;
  static constexpr uint32_t COOLDOWN_MS = 1000;
  static constexpr int HISTORY = 40;

  void reset() {
    started_ = false;
    away_ms_ = view_since_ = away_since_ = cooldown_until_ = 0;
    lower_sent_ = false;
    for (int i = 0; i < HISTORY; i++) { y_hist_[i] = 0.0f; t_hist_[i] = 0; }
    pos_ = 0;
  }

  Event feed(float x, float y, float z, uint32_t now_ms, bool screen_on) {
    bool steady = started_ && fabsf(x - px_) + fabsf(y - py_) + fabsf(z - pz_) < STEADY_G;
    px_ = x; py_ = y; pz_ = z; started_ = true;
    y_hist_[pos_] = y; t_hist_[pos_] = now_ms;
    pos_ = (pos_ + 1) % HISTORY;

    const bool view = z >= VIEW_Z && y >= VIEW_Y_MIN && fabsf(x) <= VIEW_X_MAX;
    const bool away = z < AWAY_Z || fabsf(x) > AWAY_X || y < AWAY_Y;
    if (away) {
      away_ms_ = now_ms ? now_ms : 1;
      if (!away_since_) away_since_ = away_ms_;
    } else {
      away_since_ = 0;
      lower_sent_ = false;
    }
    if (view && steady) {
      if (!view_since_) view_since_ = now_ms ? now_ms : 1;
    } else {
      view_since_ = 0;
    }

    if (screen_on) {
      if (away_since_ && !lower_sent_ && now_ms - away_since_ >= LOWER_MS) {
        lower_sent_ = true;
        return LOWER;
      }
      return NONE;
    }

    if (!view_since_ || now_ms - view_since_ < HOLD_MS ||
        (int32_t)(now_ms - cooldown_until_) < 0) return NONE;
    const bool from_down = away_ms_ && view_since_ - away_ms_ <= RAISE_WINDOW_MS && y >= RAISE_Y_MIN;
    const bool tilted = y >= TILT_Y_MIN && y - min_recent_y(now_ms) >= TILT_Y_RISE;
    if (!from_down && !tilted) return NONE;
    away_ms_ = view_since_ = 0;
    cooldown_until_ = now_ms + COOLDOWN_MS;
    return RAISE;
  }

 private:
  float min_recent_y(uint32_t now_ms) const {
    float m = 1e9f;
    for (int i = 0; i < HISTORY; i++) {
      if (t_hist_[i] && now_ms - t_hist_[i] <= TILT_WINDOW_MS && y_hist_[i] < m) m = y_hist_[i];
    }
    return m;
  }

  bool started_ = false;
  float px_ = 0, py_ = 0, pz_ = 0;
  float y_hist_[HISTORY] = {};
  uint32_t t_hist_[HISTORY] = {};
  int pos_ = 0;
  uint32_t away_ms_ = 0, view_since_ = 0, away_since_ = 0;
  bool lower_sent_ = false;
  uint32_t cooldown_until_ = 0;
};

static WristRaise wristRaise;
static volatile uint8_t wristEvent = WristRaise::NONE;

static inline void wristFeed(float x, float y, float z, uint32_t now_ms, bool screen_on) {
  const uint8_t e = wristRaise.feed(x, y, z, now_ms, screen_on);
  if (e) wristEvent = e;
}

static inline uint8_t wristTakeEvent() {
  const uint8_t e = wristEvent;
  wristEvent = WristRaise::NONE;
  return e;
}
