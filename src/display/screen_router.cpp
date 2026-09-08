#include "display/screen_router.h"

#include <Arduino.h>

#include "config.h" // WiFi via platform.h
#include "app_state.h"
#include "core/clock.h"
#include "core/settings.h"
#include "display/canvas.h"
#include "display/screen_boot.h"
#include "display/screen_header.h"
#include "display/screen_mood.h"
#include "display/screen_weather.h"
#include "net/weather_service.h"

using Canvas::animationFrame;
using Canvas::display;

namespace ScreenRouter
{
  namespace
  {
    uint32_t lastAnimationFrame = 0;
  }

  void render()
  {
    // Moods redraw faster (~22 fps) for fluid motion; weather stays ~11 fps.
    const uint32_t frameInterval = (Settings::mood() == MOOD_AUTO) ? 90 : 45;
    if (!Canvas::ready() || millis() - lastAnimationFrame < frameInterval)
      return;
    lastAnimationFrame = millis();
    ++animationFrame;

    if (ScreenBoot::render())
      return;

    const Weather &weather = WeatherService::data();
    const bool night = weather.valid && !weather.isDay;
    static int8_t previousNightMode = -1;
    if (previousNightMode != static_cast<int8_t>(night))
    {
      // Reduce contrast at night but keep it clearly visible (dim(true) would
      // drop contrast to 0 and blank the panel).
      display.ssd1306_command(SSD1306_SETCONTRAST);
      display.ssd1306_command(night ? 0x40 : 0xCF);
      previousNightMode = night;
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    const String clockText = Clock::localTimeString();

    // Animated loop: periodically greet the user in the top strip, replacing
    // the weather/clock while the face keeps animating below.
    const bool greet = Settings::name().length() && (millis() % 14000UL) >= 10000UL;
    if (greet)
    {
      ScreenHeader::draw();
    }
    else if (weather.valid)
    {
      ScreenWeather::drawIcon(1, 1, weather.code);
      display.setCursor(20, 2);
      display.printf("%.1fC", weather.temperature);
      if (clockText.length())
      {
        display.setCursor(94, 2);
        display.print(clockText);
      }
    }
    else if (WiFi.status() == WL_CONNECTED)
    {
      display.setCursor(5, 3);
      display.print(F("Fetching weather..."));
    }
    else
    {
      display.setCursor(4, 3);
      display.print(F("Connecting WiFi..."));
    }
    display.drawLine(0, 17, 127, 17, SSD1306_WHITE);
    if (Settings::mood() == MOOD_AUTO)
      ScreenWeather::drawFace();
    else
      ScreenMood::draw();
    display.display();
  }
}
