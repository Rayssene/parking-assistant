# Hardware test plan

Run these tests on the assembled circuit. Use a book or a flat box as the obstacle
and a ruler or tape measure for the distance. Fill in the result column.

| ID | Requirement | Procedure | Expected result | Result |
|----|-------------|-----------|-----------------|--------|
| HW-07 | REQ-07 | Press the reset button on the Arduino. | All 3 LEDs and the buzzer are on for about 1 s, LCD shows "Self-test...". | |
| HW-01 | REQ-01 | Open the Serial Monitor (115200 baud) and count the lines over 10 s. | At least 100 lines (≥ 10 measurements per second). | |
| HW-02 | REQ-02 | Place the obstacle at 40 cm, 70 cm and 150 cm. | LCD shows the distance within ±2 cm and the right zone name. | |
| HW-03 | REQ-03 | Move the obstacle slowly from 150 cm to 10 cm. | SAFE → NEAR → CLOSE → STOP with the LED and sound pattern from REQ-03. | |
| HW-04 | REQ-04 | At 60 cm, wave a hand quickly through the beam once. | The zone does not change for a single quick wave. | |
| HW-05 | REQ-05 | Hold the obstacle still at about 30 cm, on the STOP/CLOSE boundary. | The zone does not flicker; it only leaves STOP after moving to about 33 cm. | |
| HW-06 | REQ-06 | Unplug the ECHO wire while the system runs, then plug it back in. | After about 0.5 s: "SENSOR ERROR", red LED blinks. After reconnecting: normal operation. | |
