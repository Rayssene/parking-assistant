/*
  Parking Assistant (Arduino Uno)

  Measures the distance to an obstacle with an HC-SR04 ultrasonic sensor and
  warns with LEDs, a buzzer and an LCD, like the parking sensors in a car.
  The requirements (REQ-01 ... REQ-07) are in docs/requirements.md.

  Structure:
    zone_logic.h        -> logic without hardware (filter, zones, hysteresis)
    parking_assistant   -> this file: reads the sensor, drives LEDs, buzzer, LCD
*/

#include "zone_logic.h"

// ===========================================================================
// Hardware configuration: change these three lines to match your parts
// ===========================================================================
#define LCD_USE_I2C 1          // 1 = LCD with 4 pins (I2C), 0 = LCD with 16 pins
#define LCD_I2C_ADDRESS 0x27   // if the I2C display stays empty, try 0x3F
#define BUZZER_PASSIVE 0       // 1 = passive buzzer, 0 = active buzzer

#if LCD_USE_I2C
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, 16, 2);
#else
#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);  // RS, E, D4, D5, D6, D7
#endif

// ===========================================================================
// Pins
// ===========================================================================
const uint8_t PIN_TRIG = 9;
const uint8_t PIN_ECHO = 10;
const uint8_t PIN_LED_GREEN = 6;
const uint8_t PIN_LED_YELLOW = 7;
const uint8_t PIN_LED_RED = 8;
const uint8_t PIN_BUZZER = A0;

// ===========================================================================
// Timing
// ===========================================================================
const unsigned long MEASURE_PERIOD_MS = 60;   // ~16 measurements/s (REQ-01)
const unsigned long DISPLAY_PERIOD_MS = 200;  // LCD refresh, avoids flicker
const unsigned long SENSOR_TIMEOUT_MS = 500;  // no valid reading -> error (REQ-06)
const unsigned long ECHO_TIMEOUT_US = 25000;  // max. wait for the echo (~4 m)
const unsigned long SELF_TEST_MS = 1000;      // REQ-07
const unsigned int BUZZER_FREQ_HZ = 2000;     // only used for a passive buzzer

// ===========================================================================
// State
// ===========================================================================
MedianFilter filter;
Zone zone = ZONE_STOP;     // start in the most critical zone until we know better
bool sensorError = false;
bool hasReading = false;
float filteredCm = 0.0f;
bool buzzerIsOn = false;

unsigned long lastMeasureMs = 0;
unsigned long lastDisplayMs = 0;
unsigned long lastValidMs = 0;

// ===========================================================================
// Outputs
// ===========================================================================
void setLeds(bool green, bool yellow, bool red) {
  digitalWrite(PIN_LED_GREEN, green ? HIGH : LOW);
  digitalWrite(PIN_LED_YELLOW, yellow ? HIGH : LOW);
  digitalWrite(PIN_LED_RED, red ? HIGH : LOW);
}

void setBuzzer(bool on) {
  if (on == buzzerIsOn) {
    return;  // nothing changes, so don't restart the tone
  }
  buzzerIsOn = on;
#if BUZZER_PASSIVE
  if (on) {
    tone(PIN_BUZZER, BUZZER_FREQ_HZ);
  } else {
    noTone(PIN_BUZZER);
  }
#else
  digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
#endif
}

// REQ-03 and REQ-06: what the LEDs and the buzzer do in each state.
void updateOutputs(unsigned long now) {
  if (sensorError) {
    setLeds(false, false, blinkPhase(now, 250, 500));  // red blinks
    setBuzzer(false);
    return;
  }

  switch (zone) {
    case ZONE_SAFE:
      setLeds(true, false, false);
      setBuzzer(false);
      break;
    case ZONE_NEAR:
      setLeds(false, true, false);
      setBuzzer(blinkPhase(now, 100, 1000));  // slow beep: 100 ms every second
      break;
    case ZONE_CLOSE: {
      bool on = blinkPhase(now, 150, 300);    // fast: 150 ms on, 150 ms off
      setLeds(false, on, false);              // yellow blinks with the beep
      setBuzzer(on);
      break;
    }
    case ZONE_STOP:
      setLeds(false, false, true);
      setBuzzer(true);                        // continuous tone
      break;
  }
}

// ===========================================================================
// LCD
// ===========================================================================
void lcdBegin() {
#if LCD_USE_I2C
  lcd.init();
  lcd.backlight();
#else
  lcd.begin(16, 2);
#endif
}

// REQ-02 and REQ-06: show distance and zone, or the error message.
// Each line is always written with exactly 16 characters, so old characters
// are overwritten and we never need lcd.clear() (which makes the LCD flicker).
void updateDisplay() {
  char line[17];  // 16 characters + end of string

  if (sensorError) {
    lcd.setCursor(0, 0);
    lcd.print("SENSOR ERROR    ");
    lcd.setCursor(0, 1);
    lcd.print("Check HC-SR04   ");
    return;
  }

  lcd.setCursor(0, 0);
  if (hasReading) {
    snprintf(line, sizeof(line), "Dist: %3d cm    ", (int)(filteredCm + 0.5f));
  } else {
    snprintf(line, sizeof(line), "Dist: --- cm    ");
  }
  lcd.print(line);

  lcd.setCursor(0, 1);
  snprintf(line, sizeof(line), "Zone: %-10s", zoneName(zone));
  lcd.print(line);
}

// ===========================================================================
// Sensor
// ===========================================================================

// Triggers one HC-SR04 measurement. Returns the distance in cm,
// or -1 if no echo came back in time.
float measureDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);  // a 10 us pulse starts the measurement
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long echoUs = pulseIn(PIN_ECHO, HIGH, ECHO_TIMEOUT_US);
  if (echoUs == 0) {
    return -1.0f;  // timeout: no echo
  }
  return echoToCm(echoUs);
}

// Prints one line per measurement to the Serial Monitor (CSV format):
// raw_cm, filtered_cm, zone. Useful for debugging and for logging data.
void logMeasurement(float rawCm) {
  Serial.print(rawCm, 1);
  Serial.print(',');
  Serial.print(filteredCm, 1);
  Serial.print(',');
  Serial.println(sensorError ? "ERROR" : zoneName(zone));
}

// ===========================================================================
// Self-test (REQ-07)
// ===========================================================================
void selfTest() {
  lcd.setCursor(0, 0);
  lcd.print("Parking assist  ");
  lcd.setCursor(0, 1);
  lcd.print("Self-test...    ");
  setLeds(true, true, true);
  setBuzzer(true);
  delay(SELF_TEST_MS);  // the only delay(): before the system starts working
  setBuzzer(false);
  setLeds(false, false, false);
}

// ===========================================================================
// Arduino entry points
// ===========================================================================
void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  lcdBegin();
  selfTest();

  Serial.println("raw_cm,filtered_cm,zone");
  lastValidMs = millis();
}

void loop() {
  unsigned long now = millis();

  // 1) Measure (REQ-01)
  if (now - lastMeasureMs >= MEASURE_PERIOD_MS) {
    lastMeasureMs = now;
    float rawCm = measureDistanceCm();

    if (isValidDistance(rawCm)) {
      filter.add(rawCm);                    // REQ-04
      filteredCm = filter.median();
      lastValidMs = now;
      hasReading = true;
      sensorError = false;
      zone = nextZone(zone, filteredCm);    // REQ-03, REQ-05
    } else if (!sensorError && now - lastValidMs > SENSOR_TIMEOUT_MS) {
      // REQ-06: no valid reading for too long. Fail safe: forget old readings
      // and assume the worst zone, so recovery starts from STOP.
      sensorError = true;
      hasReading = false;
      filter.reset();
      zone = ZONE_STOP;
    }

    logMeasurement(rawCm);
  }

  // 2) LEDs and buzzer, every loop so blinking stays smooth
  updateOutputs(now);

  // 3) LCD, a few times per second
  if (now - lastDisplayMs >= DISPLAY_PERIOD_MS) {
    lastDisplayMs = now;
    updateDisplay();
  }
}
