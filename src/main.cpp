// DeskBuddy: shared codebase for ESP32-C3 Super Mini (reference) and ESP8266.
// Hardcoded WiFi + location. STA-first; recovery AP portal if STA can't connect.
// Chip-specific differences live in include/platform.h; features live in modules.

#include <Arduino.h>

#include "platform.h"
#include "core/clock.h"
#include "core/settings.h"
#include "display/canvas.h"
#include "display/emote_director.h"
#include "display/screen_boot.h"
#include "display/screen_router.h"
#include "net/weather_service.h"
#include "net/web_portal.h"
#include "net/wifi_manager.h"
#include "net/power_manager.h"
#include "net/ota_service.h"
#include "pomodoro.h"

void setup()
{
  Serial.begin(115200);
  delay(200);
  platformLowerCpu(); // run at 80 MHz to reduce active current
  WifiManager::initLed();

  if (Canvas::begin())
  {
    ScreenBoot::begin();
    ScreenBoot::render();
  }
  else
  {
    Serial.println(F("SSD1306 display not found at 0x3C"));
  }

  Settings::load();
  WifiManager::begin();
  PowerManager::begin();
  Clock::beginNtp();
  Clock::initBuildTime(); // Initialize compile-time fallback for offline time  Clock::restoreAfterWake(); // recover timezone offset if we woke from quiet-hours deep sleep  OtaService::init();
}

void loop()
{
  Pomodoro::update();
  EmoteDirector::update();
  ScreenRouter::render();
  WebPortal::handle();
  WifiManager::handlePortalDns();
  WifiManager::handleMdns();
  PowerManager::loop();
  WifiManager::loop();
  WeatherService::loop();
  OtaService::update();
  delay(5);
}
