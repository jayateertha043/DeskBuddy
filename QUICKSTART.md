# DeskBuddy — Quick Start

Get your DeskBuddy running in about 5 minutes. Works on both the **ESP32-C3 Super
Mini** and the **ESP8266** (NodeMCU / Wemos D1) from the same code.

## 1. Wire the OLED (2 min)

Power the OLED from **3.3 V, never 5 V.**

| OLED | ESP32-C3 | ESP8266 (NodeMCU) |
|------|----------|-------------------|
| SDA  | GPIO 5   | D2 (GPIO 4)       |
| SCL  | GPIO 4   | D1 (GPIO 5)       |
| VDD  | 3V3      | 3V3               |
| GND  | GND      | GND               |

## 2. Build & upload (2 min)

In VS Code with the PlatformIO extension, pick your board's environment and click
**Upload**, or use the terminal:

```bash
# ESP32-C3 Super Mini (default)
pio run -e esp32-c3-devkitm-1 -t upload

# ESP8266
pio run -e nodemcuv2 -t upload
```

Open the serial monitor to watch it boot: `pio device monitor`.

## 3. Configure (1 min)

The device first tries the built-in default Wi-Fi. To use your own network, wait for
the recovery portal (it appears after ~40 s offline), then:

1. Connect to the Wi-Fi **`DeskBuddy-Setup`** (password **`12345678`**).
2. Open **http://192.168.4.1**.
3. Enter your **Wi-Fi**, **City**, **Country**, **name**, and **mood**.
4. **Save** — it reconnects and the setup AP closes.

## Done!

Your DeskBuddy now shows an animated face reacting to live weather, the temperature,
a clock, and a "Hi &lt;name&gt;!" greeting.

- **Dashboard any time:** `http://deskbuddy.local` (or the device IP shown on the OLED).
- **Change only location/name/mood:** leave the Wi-Fi password blank when saving.

## Tips

| Problem | Fix |
|---------|-----|
| OLED blank | Check SDA/SCL for your board, 3.3 V power, address `0x3C`. |
| Can't find `DeskBuddy-Setup` | Reset the board and wait ~40 s. |
| Won't connect | Wi-Fi must be **2.4 GHz**, not 5 GHz. |
| Weather won't load | Check internet and City/Country spelling. |

## Want to change the defaults in code?

Edit the top of [src/main.cpp](src/main.cpp): `WIFI_SSID`, `WIFI_PASSWORD`,
`LOCATION_CITY`, `LOCATION_COUNTRY`. Everything else is set from the dashboard.

See [README.md](README.md) for full details and [HARDWARE.md](HARDWARE.md) for wiring.
