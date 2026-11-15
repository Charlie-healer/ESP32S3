#pragma once
// Short-term heading fusion for the 1.75C handlebar display.
//
// Ported from moto-gps-waveshare-platforms/esp32/main/motion_heading_fusion.hpp
// and motion_heading_axis.hpp (PolyForm Noncommercial 1.0.0, same board).
//
// The phone's GPS course is the absolute, drift-free reference. The onboard
// QMI8658 gyro fills the ~200 ms between phone fixes and keeps turns
// responsive at walking pace. There is no magnetometer here, so this never
// claims to be a north reference on its own: it starts from a phone heading
// and is corrected back towards it whenever the phone reports a moving course.
//
// The rate is projected onto the measured gravity vector. That makes both the
// axis and the sign correct no matter whether the board is mounted flat or
// upright, and independent of the software screen rotation.

#include <math.h>
#include <stdint.h>

class HeadingFusion {
 public:
  void reset() {
    initialized_ = false;
    hasPhone_ = false;
    hasMovingCourse_ = false;
    stationary_ = false;
    heading_ = 0.0f;
    filteredRate_ = 0.0f;
    lastMs_ = 0;
    lastUpdateMs_ = 0;
  }

  // phoneDeg: 0..360, 0 = north, clockwise. speedMps: from the phone fix.
  void anchor(float phoneDeg, float speedMps, bool usable, uint32_t nowMs) {
    if (!usable || !isfinite(phoneDeg)) return;
    if (isfinite(speedMps)) {
      if (speedMps >= kAnchorSpeedMps) stationary_ = false;
      else if (speedMps < kStationaryMps) stationary_ = true;
    }
    const float phone = normalize(phoneDeg);
    const bool moving = isfinite(speedMps) && speedMps >= kAnchorSpeedMps;
    lastUpdateMs_ = nowMs;
    if (!hasPhone_ || (moving && !hasMovingCourse_)) {
      heading_ = phone;
      initialized_ = true;
      hasPhone_ = true;
      hasMovingCourse_ = moving;
      lastMs_ = nowMs;
      return;
    }
    // Only a course from a moving receiver is trustworthy. Correct drift
    // progressively so a bend does not produce a visible snap.
    if (moving) {
      const float err = shortestDelta(phone, heading_);
      const float k = fabsf(err) > 80.0f ? 0.65f : 0.28f;
      heading_ = normalize(heading_ + err * k);
    }
  }

  // rateDps: clockwise-positive yaw rate from the gyro. Returns true when the
  // heading actually advanced.
  bool integrate(float rateDps, uint32_t nowMs) {
    if (!isfinite(rateDps)) return false;
    lastUpdateMs_ = nowMs;
    if (stationary_) return false;
    if (!initialized_) {
      initialized_ = true;
      heading_ = 0.0f;
    }
    if (lastMs_ == 0) {
      lastMs_ = nowMs;
      return false;
    }
    if (nowMs <= lastMs_) return false;
    const uint32_t dtMs = nowMs - lastMs_;
    lastMs_ = nowMs;
    if (dtMs > kMaxGapMs) {
      filteredRate_ = 0.0f;
      return false;
    }
    const float bounded = constrainf(rateDps, -500.0f, 500.0f);
    // ~30 ms of smoothing at the sensor rate: vibration is suppressed without
    // the sluggishness of waiting for the next phone fix.
    filteredRate_ += (bounded - filteredRate_) * 0.24f;
    const float active =
        fabsf(filteredRate_) < kDeadbandDps ? 0.0f : filteredRate_;
    if (active == 0.0f) return false;
    heading_ = normalize(heading_ + active * (float)dtMs / 1000.0f);
    return true;
  }

  bool initialized() const { return initialized_; }

  bool fresh(uint32_t nowMs, uint32_t maxAgeMs = 700) const {
    return initialized_ && lastUpdateMs_ != 0 &&
           (uint32_t)(nowMs - lastUpdateMs_) <= maxAgeMs;
  }

  float heading() const { return heading_; }

 private:
  static constexpr float kAnchorSpeedMps = 1.5f;
  static constexpr float kStationaryMps = 0.6f;
  static constexpr float kDeadbandDps = 0.65f;
  static constexpr uint32_t kMaxGapMs = 160;

  static float normalize(float value) {
    float r = fmodf(value, 360.0f);
    if (r < 0.0f) r += 360.0f;
    return r;
  }

  static float shortestDelta(float target, float current) {
    float d = normalize(target) - normalize(current);
    if (d > 180.0f) d -= 360.0f;
    if (d < -180.0f) d += 360.0f;
    return d;
  }

  static float constrainf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
  }

  bool initialized_ = false;
  bool hasPhone_ = false;
  bool hasMovingCourse_ = false;
  bool stationary_ = false;
  float heading_ = 0.0f;
  float filteredRate_ = 0.0f;
  uint32_t lastMs_ = 0;
  uint32_t lastUpdateMs_ = 0;
};

// Single translation unit: these headers are included once from MotoNav.ino.
static HeadingFusion headingFusion;

// The signed projection of angular velocity onto gravity is the clockwise
// heading rate. See moto-gps motion_heading_axis.hpp for the derivation notes.
static inline float clockwiseHeadingRate(float ax, float ay, float az,
                                         float gx, float gy, float gz) {
  const float len = sqrtf(ax * ax + ay * ay + az * az);
  if (!isfinite(len) || len < 0.01f) return 0.0f;
  return -(gx * ax + gy * ay + gz * az) / len;
}

// Read accel+gyro and integrate. The QMI8658 gyro runs at 112 Hz in this
// firmware, so polling faster than that only re-reads the same sample.
static inline void headingFusionPoll(uint32_t nowMs) {
  if (!imuOK) return;
  static uint32_t lastPoll = 0;
  if (lastPoll != 0 && (uint32_t)(nowMs - lastPoll) < 12) return;
  lastPoll = nowMs;
  AccelerometerData accel;
  GyroscopeData gyro;
  if (!imu.readAccel(accel)) return;
  if (!imu.readGyro(gyro)) return;
  headingFusion.integrate(
      clockwiseHeadingRate(accel.mps2.x, accel.mps2.y, accel.mps2.z,
                           gyro.dps.x, gyro.dps.y, gyro.dps.z),
      nowMs);
}
