# Parking Assistant (Arduino)

![CI](https://github.com/Rayssene/parking-assistant/actions/workflows/ci.yml/badge.svg)

A small version of the **park distance control** found in cars: an ultrasonic
sensor measures the distance to an obstacle, and the system warns with LEDs, a
buzzer and an LCD as the obstacle gets closer.

The project is built the way automotive software is developed:
**requirements → design → implementation → verification**, with every
requirement traced to a test.

> **Status:** software complete and unit-tested; hardware build and hardware
> tests in progress.

## Features

| Zone | Distance | LED | Sound |
|------|----------|-----|-------|
| SAFE | ≥ 100 cm | green | none |
| NEAR | 50 – 100 cm | yellow | slow beep |
| CLOSE | 30 – 50 cm | yellow, blinking | fast beep |
| STOP | < 30 cm | red | continuous tone |

- **Median filter** rejects single wrong readings from the ultrasonic sensor
- **Hysteresis** stops the warnings from flickering at a zone boundary, without ever delaying a warning
- **Sensor fault detection** with a fail-safe restart
- **Self-test** of all outputs at startup
- **Non-blocking** main loop (`millis()` instead of `delay()`)
- Measurements streamed to the Serial Monitor as CSV for logging

## Repository structure

```
parking_assistant/
  parking_assistant.ino   hardware: sensor, LEDs, buzzer, LCD, main loop
  zone_logic.h            pure logic: filter, zones, hysteresis (no Arduino code)
tests/
  test_logic.cpp          unit tests for the logic, run on a PC
docs/
  requirements.md         requirements REQ-01 ... REQ-07
  wiring.md               parts list and wiring tables
  test-plan.md            hardware test procedures HW-01 ... HW-07
.github/workflows/ci.yml  runs the unit tests and compiles the sketch on every push
```

Separating the logic (`zone_logic.h`) from the hardware code is what makes it
possible to test the behaviour automatically on a PC, without an Arduino.

## Hardware

Arduino Uno, HC-SR04, 16x2 LCD (I2C or parallel), three LEDs with 220 Ω
resistors, and a buzzer. Full wiring: [docs/wiring.md](docs/wiring.md).

Three settings at the top of the sketch adapt it to your parts:

```cpp
#define LCD_USE_I2C 1          // 1 = LCD with 4 pins (I2C), 0 = LCD with 16 pins
#define LCD_I2C_ADDRESS 0x27   // if the I2C display stays empty, try 0x3F
#define BUZZER_PASSIVE 0       // 1 = passive buzzer, 0 = active buzzer
```

## Run the unit tests

```
g++ -std=c++11 -Wall -Wextra -I parking_assistant tests/test_logic.cpp -o test_logic
./test_logic
```

## Upload to the Arduino

1. Open `parking_assistant/parking_assistant.ino` in the Arduino IDE.
2. For an I2C display, install **LiquidCrystal I2C** (Frank de Brabander) in the Library Manager.
3. Select **Arduino Uno** and the right COM port, then upload.
4. Open the Serial Monitor at **115200 baud** to see `raw_cm,filtered_cm,zone`.

## Possible extensions

- Temperature compensation of the speed of sound with a DHT11 sensor
- Logging the CSV data in Python and plotting raw vs. filtered distance
- Sending the distance over CAN bus to a second "dashboard" node

## License

MIT
