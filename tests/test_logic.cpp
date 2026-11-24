// Unit tests for zone_logic.h. They run on a PC, no Arduino needed.
//
// Build and run (from the repository root):
//   g++ -std=c++11 -Wall -Wextra -I parking_assistant tests/test_logic.cpp -o test_logic
//   ./test_logic
//
// Each test names the requirement it verifies, so every requirement can be
// traced to a test (traceability, as in automotive projects).

#include <cmath>
#include <cstdio>
#include <cstring>

#include "zone_logic.h"

static int failures = 0;
static int checks = 0;

#define CHECK(condition)                                              \
  do {                                                                \
    checks++;                                                         \
    if (!(condition)) {                                               \
      failures++;                                                     \
      std::printf("  FAILED line %d: %s\n", __LINE__, #condition);    \
    }                                                                 \
  } while (0)

static bool near(float a, float b) { return std::fabs(a - b) < 0.01f; }

static void test_echo_conversion() {
  std::printf("Sensor: echo time -> distance\n");
  CHECK(near(echoToCm(0), 0.0f));
  CHECK(near(echoToCm(1000), 17.15f));    // 1 ms echo = 17.15 cm
  CHECK(near(echoToCm(5831), 100.0f));    // ~5.8 ms echo = ~1 m
}

static void test_valid_range() {
  std::printf("REQ-06: valid measurement range\n");
  CHECK(!isValidDistance(-1.0f));         // timeout marker
  CHECK(!isValidDistance(1.9f));
  CHECK(isValidDistance(2.0f));
  CHECK(isValidDistance(400.0f));
  CHECK(!isValidDistance(400.1f));
}

static void test_raw_zones() {
  std::printf("REQ-03: zone limits\n");
  CHECK(rawZone(10.0f) == ZONE_STOP);
  CHECK(rawZone(29.9f) == ZONE_STOP);
  CHECK(rawZone(30.0f) == ZONE_CLOSE);
  CHECK(rawZone(49.9f) == ZONE_CLOSE);
  CHECK(rawZone(50.0f) == ZONE_NEAR);
  CHECK(rawZone(99.9f) == ZONE_NEAR);
  CHECK(rawZone(100.0f) == ZONE_SAFE);
  CHECK(rawZone(250.0f) == ZONE_SAFE);
}

static void test_more_critical_is_immediate() {
  std::printf("REQ-05: a more critical zone is entered immediately\n");
  CHECK(nextZone(ZONE_SAFE, 99.0f) == ZONE_NEAR);
  CHECK(nextZone(ZONE_NEAR, 49.0f) == ZONE_CLOSE);
  CHECK(nextZone(ZONE_CLOSE, 29.0f) == ZONE_STOP);
  CHECK(nextZone(ZONE_SAFE, 10.0f) == ZONE_STOP);  // skipping zones is allowed
}

static void test_hysteresis() {
  std::printf("REQ-05: leaving a zone needs a 3 cm margin\n");
  CHECK(nextZone(ZONE_STOP, 31.0f) == ZONE_STOP);
  CHECK(nextZone(ZONE_STOP, 32.9f) == ZONE_STOP);
  CHECK(nextZone(ZONE_STOP, 33.0f) == ZONE_CLOSE);
  CHECK(nextZone(ZONE_CLOSE, 52.0f) == ZONE_CLOSE);
  CHECK(nextZone(ZONE_CLOSE, 53.0f) == ZONE_NEAR);
  CHECK(nextZone(ZONE_NEAR, 101.0f) == ZONE_NEAR);
  CHECK(nextZone(ZONE_NEAR, 103.0f) == ZONE_SAFE);
  // A big jump still respects the margin at the last boundary it crosses
  CHECK(nextZone(ZONE_CLOSE, 101.0f) == ZONE_NEAR);
  CHECK(nextZone(ZONE_STOP, 150.0f) == ZONE_SAFE);
}

static void test_no_flicker_on_boundary() {
  std::printf("REQ-05: noise around a boundary does not make the zone flicker\n");
  const float noisy[] = {30.5f, 29.5f, 31.0f, 29.8f, 32.0f, 30.2f, 29.9f, 31.5f};
  Zone zone = ZONE_CLOSE;
  int changes = 0;
  bool enteredStop = false;
  for (float cm : noisy) {
    Zone next = nextZone(zone, cm);
    if (next != zone) changes++;
    if (next == ZONE_STOP) enteredStop = true;
    zone = next;
  }
  CHECK(enteredStop);    // the warning still comes
  CHECK(changes == 1);   // ...but only one change, no flickering
}

static void test_median_filter() {
  std::printf("REQ-04: median filter rejects single wrong readings\n");
  MedianFilter f;
  CHECK(f.empty());
  CHECK(near(f.median(), 0.0f));

  f.add(50.0f);
  CHECK(near(f.median(), 50.0f));
  f.add(52.0f);
  CHECK(near(f.median(), 51.0f));  // even count: mean of the two middle values

  f.add(300.0f);   // outlier (e.g. a missed echo)
  f.add(51.0f);
  f.add(49.0f);
  CHECK(near(f.median(), 51.0f));  // sorted: 49 50 51 52 300 -> 51

  // After 5 new readings the oldest are gone (ring buffer)
  for (int i = 0; i < 5; i++) f.add(20.0f);
  CHECK(near(f.median(), 20.0f));

  f.reset();
  CHECK(f.empty());
}

static void test_zone_names() {
  std::printf("REQ-02: zone names for the LCD\n");
  CHECK(std::strcmp(zoneName(ZONE_SAFE), "SAFE") == 0);
  CHECK(std::strcmp(zoneName(ZONE_NEAR), "NEAR") == 0);
  CHECK(std::strcmp(zoneName(ZONE_CLOSE), "CLOSE") == 0);
  CHECK(std::strcmp(zoneName(ZONE_STOP), "STOP") == 0);
}

static void test_blink_phase() {
  std::printf("REQ-03: blink and beep timing\n");
  CHECK(blinkPhase(0, 100, 1000));
  CHECK(blinkPhase(99, 100, 1000));
  CHECK(!blinkPhase(100, 100, 1000));
  CHECK(!blinkPhase(999, 100, 1000));
  CHECK(blinkPhase(1050, 100, 1000));
}

int main() {
  test_echo_conversion();
  test_valid_range();
  test_raw_zones();
  test_more_critical_is_immediate();
  test_hysteresis();
  test_no_flicker_on_boundary();
  test_median_filter();
  test_zone_names();
  test_blink_phase();

  std::printf("\n%d checks, %d failed\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
