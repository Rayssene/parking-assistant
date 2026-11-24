# Requirements: Parking Assistant v1

Each requirement has an ID so it can be traced to the code and to a test.
Automated tests are in `tests/test_logic.cpp`; hardware tests are in `docs/test-plan.md`.

| ID | Requirement | Verified by |
|----|-------------|-------------|
| REQ-01 | The system measures the distance to an obstacle with the HC-SR04 at least 10 times per second. | HW-01 |
| REQ-02 | The LCD shows the current distance in cm and the zone name (SAFE / NEAR / CLOSE / STOP). | Unit test, HW-02 |
| REQ-03 | Zones and warnings: **100 cm or more → SAFE** (green LED, no sound); **50 to under 100 cm → NEAR** (yellow LED, slow beep); **30 to under 50 cm → CLOSE** (yellow LED blinking, fast beep); **under 30 cm → STOP** (red LED, continuous tone). | Unit test, HW-03 |
| REQ-04 | The raw measurement is filtered so that a single wrong reading does not trigger a warning. | Unit test, HW-04 |
| REQ-05 | A more critical zone is entered immediately. A less critical zone is only entered when the distance is at least 3 cm past the boundary (hysteresis), so the outputs do not flicker at a boundary. | Unit test, HW-05 |
| REQ-06 | If the sensor gives no valid reading (2–400 cm) for more than 0.5 s, the LCD shows SENSOR ERROR and the red LED blinks. After an error, the system restarts from the STOP zone (fail-safe). | Unit test, HW-06 |
| REQ-07 | At startup, all LEDs and the buzzer are switched on for 1 s as a self-test. | HW-07 |

## Design decisions

- **Hysteresis only when leaving a zone (REQ-05):** a warning must never be delayed, so getting closer reacts immediately. Only moving away needs the 3 cm margin.
- **Median instead of average (REQ-04):** one wrong reading (for example 300 cm instead of 50 cm) shifts an average a lot but does not move a median.
- **Fail-safe after an error (REQ-06):** when the sensor recovers, the system starts from the most critical zone and moves down from there, instead of trusting old data.
