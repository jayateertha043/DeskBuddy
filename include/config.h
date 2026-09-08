// Compile-time configuration: user defaults, hardware constants, timings.
#pragma once

#include <Arduino.h>

#include "platform.h"

// ---------------- User configuration ----------------
// Default credentials, used until different ones are saved via the portal.
#ifndef WIFI_SSID
#define WIFI_SSID "RandomWifi"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "12345678"
#endif

// Default location (Hosur, India), used until changed via the portal.
// City/country are resolved to coordinates on-device via Open-Meteo geocoding.
#ifndef LOCATION_CITY
#define LOCATION_CITY "Hosur"
#endif
#ifndef LOCATION_COUNTRY
#define LOCATION_COUNTRY "India"
#endif
#ifndef LOCATION_LATITUDE
#define LOCATION_LATITUDE 12.7409
#endif
#ifndef LOCATION_LONGITUDE
#define LOCATION_LONGITUDE 77.8253
#endif
// ----------------------------------------------------

namespace cfg
{
  constexpr uint8_t OLED_WIDTH = 128;
  constexpr uint8_t OLED_HEIGHT = 64;
  constexpr uint8_t OLED_ADDRESS = 0x3C;
  constexpr uint8_t OLED_SDA = DESKBUDDY_OLED_SDA;
  constexpr uint8_t OLED_SCL = DESKBUDDY_OLED_SCL;
  constexpr uint8_t STATUS_LED = DESKBUDDY_STATUS_LED; // active LOW

  constexpr uint32_t WEATHER_INTERVAL_MS = 15UL * 60UL * 1000UL;
  constexpr uint32_t WEATHER_RETRY_MS = 60UL * 1000UL;
  constexpr uint32_t WIFI_RECONNECT_MS = 15UL * 1000UL;
  constexpr uint32_t PORTAL_AFTER_MS = 40UL * 1000UL; // open AP after this long offline
  constexpr uint32_t LOCATION_RETRY_MS = 60UL * 1000UL;
  constexpr uint32_t RANDOM_EMOTE_INTERVAL_MS = 10UL * 60UL * 1000UL;
  constexpr uint32_t RANDOM_EMOTE_DURATION_MS = 10UL * 1000UL;
  constexpr uint16_t DNS_PORT = 53;

  constexpr const char *AP_SSID = "DeskBuddy-Setup";
  constexpr const char *AP_PASSWORD = "12345678"; // >= 8 chars required by SoftAP
}
