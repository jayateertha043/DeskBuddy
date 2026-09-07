// Platform abstraction for DeskBuddy.
//
// The DeskBuddy application (src/main.cpp) is a single shared codebase compiled
// for two chips: the ESP32-C3 Super Mini (reference/production) and the ESP8266
// (NodeMCU / Wemos D1). Everything the two chips do differently — WiFi, web
// server, mDNS, HTTP, TLS, storage keys, and pins — is isolated here so the
// application code stays identical for both targets.
#pragma once

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP8266)
// ---------------------------- ESP8266 ----------------------------
#include <memory>

#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Preferences.h> // vshymanskyy/Preferences: ESP32-compatible API on LittleFS
#include <WiFiClientSecureBearSSL.h>

using WebServerClass = ESP8266WebServer;

#ifndef DESKBUDDY_OLED_SDA
#define DESKBUDDY_OLED_SDA D2 // NodeMCU SDA (GPIO4)
#endif
#ifndef DESKBUDDY_OLED_SCL
#define DESKBUDDY_OLED_SCL D1 // NodeMCU SCL (GPIO5)
#endif
#ifndef DESKBUDDY_STATUS_LED
#define DESKBUDDY_STATUS_LED LED_BUILTIN // GPIO2, active LOW
#endif
#ifndef WIFI_POWER_8_5dBm
#define WIFI_POWER_8_5dBm 0 // unused on ESP8266; keeps app code symbol-clean
#endif

// The ESP32-C3 Super Mini needs reduced TX power to work around its poor PCB
// antenna. The ESP8266 has no such issue, so keep full power (better range).
inline void platformWifiLowPower() {}
inline void platformWifiHostname(const char *name) { WiFi.hostname(name); }
inline void platformWifiSleep(bool enable)
{
    WiFi.setSleepMode(enable ? WIFI_MODEM_SLEEP : WIFI_NONE_SLEEP);
}
inline void platformMdnsUpdate() { MDNS.update(); }
inline void platformHttpTimeouts(HTTPClient &http) { http.setTimeout(9000); }

// BearSSL's secure client is large; heap-allocate it so it can't blow the stack.
struct SecureHttp
{
    std::unique_ptr<BearSSL::WiFiClientSecure> client{new BearSSL::WiFiClientSecure};
    SecureHttp() { client->setInsecure(); }
    WiFiClient &ref() { return *client; }
};

#else
// ------------------------- ESP32 / ESP32-C3 -------------------------
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

using WebServerClass = WebServer;

#ifndef DESKBUDDY_OLED_SDA
#define DESKBUDDY_OLED_SDA 5 // ESP32-C3 Super Mini I2C SDA
#endif
#ifndef DESKBUDDY_OLED_SCL
#define DESKBUDDY_OLED_SCL 4 // ESP32-C3 Super Mini I2C SCL
#endif
#ifndef DESKBUDDY_STATUS_LED
#define DESKBUDDY_STATUS_LED 8 // onboard LED, active LOW
#endif

// Reduce TX power: the ESP32-C3 Super Mini's antenna causes auth/assoc failures
// at full power. This is the key fix for AUTH_EXPIRE on that board.
inline void platformWifiLowPower() { WiFi.setTxPower(WIFI_POWER_8_5dBm); }
inline void platformWifiHostname(const char *name) { WiFi.setHostname(name); }
inline void platformWifiSleep(bool enable) { WiFi.setSleep(enable); }
inline void platformMdnsUpdate() {}
inline void platformHttpTimeouts(HTTPClient &http)
{
    http.setConnectTimeout(10000); // ms: bound TCP connect
    http.setTimeout(9000);
}

struct SecureHttp
{
    WiFiClientSecure client;
    SecureHttp()
    {
        client.setInsecure();
        client.setHandshakeTimeout(10); // seconds: bound TLS handshake
        client.setTimeout(10);          // seconds: socket read timeout
    }
    WiFiClient &ref() { return client; }
};

#endif
