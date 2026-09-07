# Hardware Connection Guide

DeskBuddy needs just two things: a supported board and a 128×64 I2C OLED. The same
firmware runs on the **ESP32-C3 Super Mini** and the **ESP8266** (NodeMCU / Wemos D1);
only the wiring pins differ.

> ⚠️ **CRITICAL: power the OLED from 3.3 V, never 5 V.**

## Pin assignments

| Signal | ESP32-C3 Super Mini | ESP8266 (NodeMCU) | OLED pin |
|--------|---------------------|-------------------|----------|
| SDA    | GPIO 5              | D2 (GPIO 4)       | SDA      |
| SCL    | GPIO 4              | D1 (GPIO 5)       | SCL      |
| VDD    | 3V3                 | 3V3               | VDD      |
| GND    | GND                 | GND               | GND      |
| Status LED (onboard) | GPIO 8 (active LOW) | GPIO 2 / `LED_BUILTIN` (active LOW) | — |

The status LED turns on when the device is connected to Wi-Fi.

## Wiring — ESP32-C3 Super Mini

```
┌──────────────┐           ┌──────────────┐
│  ESP32-C3    │           │  OLED 128x64 │
│              │           │   (SSD1306)  │
│   3V3  ──────┼───────────► VDD          │
│   GND  ──────┼───────────► GND          │
│  GPIO4 (SCL) ┼───────────► SCL          │
│  GPIO5 (SDA) ┼───────────► SDA          │
└──────────────┘           └──────────────┘
```

## Wiring — ESP8266 (NodeMCU / Wemos D1)

```
┌──────────────┐           ┌──────────────┐
│   ESP8266    │           │  OLED 128x64 │
│              │           │   (SSD1306)  │
│   3V3  ──────┼───────────► VDD          │
│   GND  ──────┼───────────► GND          │
│  D1 (GPIO5)  ┼───────────► SCL          │
│  D2 (GPIO4)  ┼───────────► SDA          │
└──────────────┘           └──────────────┘
```

> Note the mapping differs by board: on the C3, SDA=GPIO5/SCL=GPIO4; on the ESP8266,
> SDA=D2(GPIO4)/SCL=D1(GPIO5). The firmware sets these automatically per board via
> [include/platform.h](include/platform.h).

## OLED specifications

| Property | Value |
|----------|-------|
| Resolution | 128 × 64 pixels |
| Controller | SSD1306 |
| Interface | I2C |
| Address | `0x3C` (some modules use `0x3D`) |
| Supply | **3.3 V** (not 5 V) |
| Current | ~20 mA typical |

## Power

- Board is powered over USB (5 V in, on-board 3.3 V regulator).
- Typical current: ~50 mA idle, ~150 mA with Wi-Fi active, ~300 mA peak.
- The OLED's VDD must come from the board's **3V3** pin.

## Connection checklist

- [ ] OLED VDD on **3.3 V** (never 5 V)
- [ ] GND connected solidly
- [ ] SDA/SCL match the table for **your** board
- [ ] No shorts or loose wires
- [ ] USB cable supplies power (and data for programming)

## Troubleshooting

### OLED shows nothing
1. Confirm VDD is 3.3 V and GND is solid.
2. Double-check SDA/SCL against the pin table for your specific board.
3. Verify the I2C address is `0x3C`; try `0x3D` if your module uses it.
4. Some modules need external 4.7 kΩ pull-ups on SDA/SCL to 3.3 V (most have them built in).

### Board won't boot / upload fails
1. Try a different USB cable/port (some cables are power-only).
2. On ESP8266, install the USB-serial driver (CH340 or CP2102).
3. Watch the serial monitor during boot for I2C or Wi-Fi messages.

### Wi-Fi won't connect
- Both chips are **2.4 GHz only** — 5 GHz networks won't appear.
- Use the `DeskBuddy-Setup` recovery portal to re-enter credentials (see
  [QUICKSTART.md](QUICKSTART.md)).

## Quick reference

| Item | ESP32-C3 | ESP8266 |
|------|----------|---------|
| SDA  | GPIO 5   | D2 (GPIO 4) |
| SCL  | GPIO 4   | D1 (GPIO 5) |
| Status LED | GPIO 8 | GPIO 2 |
| OLED power | 3.3 V | 3.3 V |
| OLED address | `0x3C` | `0x3C` |
| Wi-Fi band | 2.4 GHz | 2.4 GHz |
