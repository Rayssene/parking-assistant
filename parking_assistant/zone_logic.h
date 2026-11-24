// zone_logic.h
// Hardware-independent logic of the parking assistant.
//
// Everything in this file is plain C++ with no Arduino functions, so it runs
// both on the Arduino and on a PC. That is what makes it possible to test the
// logic automatically (see tests/test_logic.cpp) without any hardware.

#ifndef ZONE_LOGIC_H
#define ZONE_LOGIC_H

#include <stdint.h>

// ---------------------------------------------------------------------------
// Warning zones (REQ-03). The order matters: a higher value = more critical.
// ---------------------------------------------------------------------------
enum Zone : uint8_t {
  ZONE_SAFE = 0,
  ZONE_NEAR = 1,
  ZONE_CLOSE = 2,
  ZONE_STOP = 3
};

// Zone limits in cm (REQ-03)
const float STOP_LIMIT_CM = 30.0f;   // below 30 cm          -> STOP
const float CLOSE_LIMIT_CM = 50.0f;  // 30 cm to under 50 cm -> CLOSE
const float NEAR_LIMIT_CM = 100.0f;  // 50 cm to under 100 cm -> NEAR, else SAFE

// Hysteresis margin in cm (REQ-05)
const float HYSTERESIS_CM = 3.0f;

// Range the HC-SR04 can measure reliably (datasheet: about 2 cm to 400 cm)
const float MIN_VALID_CM = 2.0f;
const float MAX_VALID_CM = 400.0f;

// ---------------------------------------------------------------------------
// Sensor conversion
// ---------------------------------------------------------------------------

// Converts the echo time of the HC-SR04 into a distance.
// Sound travels at about 343 m/s = 0.0343 cm/us at 20 degC.
// The echo time covers the way to the obstacle AND back, so we divide by 2.
inline float echoToCm(unsigned long echoUs) {
  return echoUs * 0.0343f / 2.0f;
}

// A reading is valid only inside the range the sensor can measure (REQ-06).
inline bool isValidDistance(float cm) {
  return cm >= MIN_VALID_CM && cm <= MAX_VALID_CM;
}

// ---------------------------------------------------------------------------
// Median filter (REQ-04)
// Keeps the last 5 valid readings and returns their median. A single wrong
// reading (for example 300 cm in a series of 50 cm) cannot move the median.
// ---------------------------------------------------------------------------
const uint8_t FILTER_SIZE = 5;

class MedianFilter {
 public:
  MedianFilter() : count_(0), next_(0) {}

  void reset() {
    count_ = 0;
    next_ = 0;
  }

  bool empty() const { return count_ == 0; }

  // Adds a reading. When the buffer is full, the oldest reading is replaced.
  void add(float value) {
    values_[next_] = value;
    next_ = (next_ + 1) % FILTER_SIZE;
    if (count_ < FILTER_SIZE) {
      count_++;
    }
  }

  // Returns the median of the stored readings (0 if there are none).
  float median() const {
    if (count_ == 0) {
      return 0.0f;
    }
    // Copy the readings and sort the copy (insertion sort, fine for 5 values)
    float sorted[FILTER_SIZE];
    for (uint8_t i = 0; i < count_; i++) {
      sorted[i] = values_[i];
    }
    for (uint8_t i = 1; i < count_; i++) {
      float key = sorted[i];
      int8_t j = i - 1;
      while (j >= 0 && sorted[j] > key) {
        sorted[j + 1] = sorted[j];
        j--;
      }
      sorted[j + 1] = key;
    }
    if (count_ % 2 == 1) {
      return sorted[count_ / 2];
    }
    return (sorted[count_ / 2 - 1] + sorted[count_ / 2]) / 2.0f;
  }

 private:
  float values_[FILTER_SIZE];
  uint8_t count_;
  uint8_t next_;
};

// ---------------------------------------------------------------------------
// Zone logic (REQ-03, REQ-05)
// ---------------------------------------------------------------------------

// Zone for a distance, without hysteresis.
inline Zone rawZone(float cm) {
  if (cm < STOP_LIMIT_CM) return ZONE_STOP;
  if (cm < CLOSE_LIMIT_CM) return ZONE_CLOSE;
  if (cm < NEAR_LIMIT_CM) return ZONE_NEAR;
  return ZONE_SAFE;
}

// Zone with hysteresis, based on the current zone and a new distance.
//
// Safety rule: a MORE critical zone is entered immediately, because a warning
// must never be delayed. A LESS critical zone is only entered when the
// distance is at least HYSTERESIS_CM past the boundary. This stops the
// LEDs and the buzzer from flickering when the obstacle sits on a boundary.
inline Zone nextZone(Zone current, float cm) {
  Zone candidate = rawZone(cm);
  if (candidate >= current) {
    return candidate;  // same or more critical: react immediately
  }
  // Less critical: evaluate as if every boundary were HYSTERESIS_CM further away
  Zone withMargin = rawZone(cm - HYSTERESIS_CM);
  return (withMargin < current) ? withMargin : current;
}

inline const char* zoneName(Zone zone) {
  switch (zone) {
    case ZONE_SAFE: return "SAFE";
    case ZONE_NEAR: return "NEAR";
    case ZONE_CLOSE: return "CLOSE";
    case ZONE_STOP: return "STOP";
  }
  return "?";
}

// ---------------------------------------------------------------------------
// Timing helper for blinking LEDs and beeping without delay()
// Returns true during the first onMs of every periodMs.
// ---------------------------------------------------------------------------
inline bool blinkPhase(unsigned long nowMs, unsigned long onMs, unsigned long periodMs) {
  return (nowMs % periodMs) < onMs;
}

#endif  // ZONE_LOGIC_H
