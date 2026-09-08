# DeskBuddy — Animated Weather Buddy for ESP32-C3 & ESP8266

DeskBuddy is a tiny desk companion: an OLED shows an **animated face** that reacts
to your local weather, the current temperature, a live clock, and a friendly
greeting. Everything is configured from a built-in **web dashboard** — no app,
no cloud account, and no API key required.

The same application runs on two boards from **one shared codebase**:

- **ESP32-C3 Super Mini** (reference / production board)
- **ESP8266** (NodeMCU 1.0 / Wemos D1)

All chip-specific differences live in [include/platform.h](include/platform.h),
so features and behaviour are identical on both.

## Features

- **Animated buddy face** — liquid eyes that blink, drift, and react to the weather
  (moon + Zzz at night, raindrops, snowflakes, lightning, fog, drifting clouds, sun rays).
- **Wake-up animation** — Teevee opens its eyes, looks left and right, double-blinks,
  gets excited, gives a local-time greeting, and introduces itself.
- **Manual moods** — override the weather face with any of: happy, sad, excited,
  angry, stretching, sneezing, sleeping, confused, curious, do-not-disturb, yawn,
  look left, look right, wink, laugh, surprised, and nervous.
- **Live weather + clock** — temperature, condition, and an NTP-synced clock in the
  top strip, with the time zone resolved automatically for your location.
- **Personal greeting** — periodically shows "Hi &lt;your name&gt;!".
- **Web dashboard** — reachable any time on your network at `http://deskbuddy.local`
  (or the device IP). Pick emotes from visual cards, hold one in Static mode, or
  let Random mode play a reaction for 10 seconds every 10 minutes before returning
  to the weather face.
- **Recovery Wi-Fi portal** — if it can't get online, it opens a `DeskBuddy-Setup`
  access point with a captive portal so you can fix the credentials.
- **City/country geocoding** — enter a city and country; coordinates are resolved
  on-device via the free Open-Meteo geocoding API.
- **Persistent storage** — settings survive power loss (NVS on ESP32, LittleFS on ESP8266).
- **No API key** — uses the free Open-Meteo weather API.

## Hardware

| Part | Notes |
|------|-------|
| ESP32-C3 Super Mini **or** ESP8266 (NodeMCU/Wemos D1) | Main microcontroller |
| 128×64 I2C OLED (SSD1306) | Address `0x3C` (some are `0x3D`) |
| USB cable | Power + programming |
| 2.4 GHz Wi-Fi | Both chips are 2.4 GHz only |

### Wiring

**Power the OLED from 3.3 V, never 5 V.**

| OLED | ESP32-C3 Super Mini | ESP8266 (NodeMCU) |
|------|---------------------|-------------------|
| SDA  | GPIO 5              | D2 (GPIO 4)       |
| SCL  | GPIO 4              | D1 (GPIO 5)       |
| VDD  | 3V3                 | 3V3               |
| GND  | GND                 | GND               |

The onboard status LED lights when connected (GPIO 8 on the C3, `LED_BUILTIN`/GPIO 2
on the ESP8266). See [HARDWARE.md](HARDWARE.md) for full wiring diagrams.

## Build & Upload

This is a [PlatformIO](https://platformio.org/) project. The default environment
is the ESP32-C3, so plain `pio run` targets that board.

```bash
# ESP32-C3 Super Mini (default)
pio run -e esp32-c3-devkitm-1 -t upload

# ESP8266 (NodeMCU / Wemos D1)
pio run -e nodemcuv2 -t upload

# Serial monitor
pio device monitor
```

The correct serial port is auto-selected by USB vendor ID, so a fixed `COMx` is not
required. To force one: `pio run -t upload --upload-port COMx`.

> **Build location:** artifacts are written to `C:/pio_build/DeskBuddy-SuperMini`
> (outside OneDrive) to avoid file-sync locks stalling the compiler.

## First-run configuration

1. On first boot the device tries to connect using the compile-time defaults
   (`WIFI_SSID`, `WIFI_PASSWORD`, `LOCATION_CITY`, `LOCATION_COUNTRY` at the top of
   [src/main.cpp](src/main.cpp)). Edit those to your own values, **or** use the portal below.
2. If it can't get online within ~40 s, it opens a recovery access point:
   - **SSID:** `DeskBuddy-Setup`  ·  **Password:** `12345678`  ·  **URL:** `http://192.168.4.1`
3. Connect to that AP, open the page, and enter your **Wi-Fi**, **City**, **Country**,
   **name**, and **mood**. Save — the device reconnects and the AP closes.
4. Once online, the dashboard is always available at `http://deskbuddy.local` or the
   device's IP. Leave the Wi-Fi password blank to keep the current one while changing
   only location/name/mood.

## What the display shows

- **Top strip:** weather icon + temperature + clock, or a rotating "Hi &lt;name&gt;!"
  greeting, or a connection status message while offline.
- **Below the divider:** the animated buddy face — driven by the weather in `Auto`
  mode, or by your chosen mood otherwise.

## Weather & location

- **Weather:** Open-Meteo forecast API, refreshed every 15 minutes (retried every
  60 s on failure). Provides temperature, condition code, day/night, and UTC offset.
- **Geocoding:** Open-Meteo geocoding API resolves your City + Country to
  coordinates on-device. Country accepts a full name or an ISO code.
- No API key, no account. HTTPS is used for both endpoints.

## Project structure

```
DeskBuddy-SuperMini/
├── platformio.ini        # Envs for both boards + dependencies
├── src/
│   └── main.cpp          # Shared application (compiled for both chips)
├── include/
│   └── platform.h        # Chip abstraction (WiFi, web, mDNS, HTTP, TLS, storage, pins)
├── README.md             # This file
├── QUICKSTART.md         # 5-minute setup
└── HARDWARE.md           # Wiring details & troubleshooting
```

## Architecture notes

- **One codebase, two chips.** `main.cpp` is platform-independent; `platform.h`
  supplies typedefs and small inline helpers for each target:
  - `WebServerClass` — `WebServer` (ESP32) vs `ESP8266WebServer`.
  - `SecureHttp` — stack `WiFiClientSecure` on ESP32, heap-allocated
    `BearSSL::WiFiClientSecure` on ESP8266 (the BearSSL client is too large for the stack).
  - Pins — `DESKBUDDY_OLED_SDA/SCL/STATUS_LED`.
  - `platformWifiLowPower/Hostname/Sleep`, `platformMdnsUpdate`, `platformHttpTimeouts`.
- **Storage** uses the ESP32 `Preferences` API on both boards; on ESP8266 the
  `vshymanskyy/Preferences` library provides the same API backed by LittleFS.
- **TX power** is reduced only on the ESP32-C3 (a fix for its weak PCB antenna that
  otherwise causes auth/assoc failures). The ESP8266 keeps full power for better range.

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| OLED blank | Verify SDA/SCL pins for your board, 3.3 V power, and address `0x3C`. |
| Never connects / opens `DeskBuddy-Setup` | Wi-Fi must be **2.4 GHz**; re-enter credentials in the portal. |
| `deskbuddy.local` won't resolve | Use the device IP (shown on the OLED / serial), or check your network supports mDNS. |
| Weather stuck on "Fetching…" | Check internet access and that City/Country spelling resolves. |
| ESP32-C3 auth failures | Already mitigated via reduced TX power; if it persists try a hotspot to isolate the AP. |
| Upload fails | Try another USB port/cable; on ESP8266 the CH340/CP2102 driver may be needed. |

## Credits

- Weather & geocoding: [Open-Meteo](https://open-meteo.com/)
- Display: [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) + [SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
- Boot-motion inspiration: [FluxGarage RoboEyes](https://github.com/FluxGarage/RoboEyes)
- ESP8266 storage: [vshymanskyy/Preferences](https://github.com/vshymanskyy/Preferences)

## License

MIT — modify and share freely.
