# Wiring

## Parts

| Part | Quantity |
|------|----------|
| Arduino Uno R3 (or compatible) | 1 |
| HC-SR04 ultrasonic sensor | 1 |
| LCD 16x2 (with I2C backpack = 4 pins, or without = 16 pins) | 1 |
| LEDs: green, yellow, red | 1 each |
| Resistors 220 Ω (for the LEDs) | 3 |
| Buzzer (active or passive) | 1 |
| 10 kΩ potentiometer (only for the 16-pin LCD, contrast) | 1 |
| Breadboard, jumper wires, USB cable | |

## Connections (same for both LCD types)

| From | To Arduino |
|------|------------|
| HC-SR04 VCC | 5V |
| HC-SR04 GND | GND |
| HC-SR04 TRIG | D9 |
| HC-SR04 ECHO | D10 |
| Green LED (long leg) → 220 Ω → | D6 |
| Yellow LED (long leg) → 220 Ω → | D7 |
| Red LED (long leg) → 220 Ω → | D8 |
| All LED short legs | GND |
| Buzzer + (long leg / marked "+") | A0 |
| Buzzer − | GND |

## LCD with 4 pins (I2C): set `LCD_USE_I2C 1`

| LCD pin | Arduino |
|---------|---------|
| GND | GND |
| VCC | 5V |
| SDA | A4 |
| SCL | A5 |

Library: **LiquidCrystal I2C** by Frank de Brabander (Arduino Library Manager).
If the screen lights up but shows nothing, turn the small blue potentiometer on the
back with a screwdriver (contrast). If it still shows nothing, change
`LCD_I2C_ADDRESS` from `0x27` to `0x3F`.

## LCD with 16 pins (parallel): set `LCD_USE_I2C 0`

| LCD pin | Name | Connect to |
|---------|------|-----------|
| 1 | VSS | GND |
| 2 | VDD | 5V |
| 3 | V0 | Middle pin of the 10 kΩ potentiometer (outer pins to 5V and GND) |
| 4 | RS | D12 |
| 5 | RW | GND |
| 6 | E | D11 |
| 7–10 | D0–D3 | not connected |
| 11 | D4 | D5 |
| 12 | D5 | D4 |
| 13 | D6 | D3 |
| 14 | D7 | D2 |
| 15 | A (backlight +) | 5V through a 220 Ω resistor |
| 16 | K (backlight −) | GND |

Library: **LiquidCrystal** (already included in the Arduino IDE).

## Buzzer type

- **Active buzzer** (beeps when connected to 5V directly, often has a sticker on top, sealed underneath): `BUZZER_PASSIVE 0`
- **Passive buzzer** (only clicks on 5V, green circuit board visible underneath): `BUZZER_PASSIVE 1`
