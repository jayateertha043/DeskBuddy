#include "display/screen_router.h"

#include <Arduino.h>

#include "config.h" // WiFi via platform.h
#include "app_state.h"
#include "core/clock.h"
#include "core/settings.h"
#include "display/canvas.h"
#include "display/screen_boot.h"
#include "display/screen_clock.h"
#include "display/screen_header.h"
#include "display/screen_mood.h"
#include "display/screen_pomodoro.h"
#include "display/screen_sticky.h"
#include "display/screen_weather.h"
#include "net/weather_service.h"
#include "pomodoro.h"

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
        // Moods and the focus hourglass redraw faster (~22 fps) for fluid motion.
        const uint32_t frameInterval =
            (Pomodoro::active() || Settings::mood() != MOOD_AUTO) ? 45 : 90;
        if (!Canvas::ready() || millis() - lastAnimationFrame < frameInterval)
            return;
        lastAnimationFrame = millis();
        ++animationFrame;

        if (ScreenBoot::render())
            return;

        const Weather &weather = WeatherService::data();
        const bool night = weather.valid && !weather.isDay;
        // Base contrast follows the user's brightness slider; night dims it further
        // (but never to black). Re-applied whenever either value changes.
        const uint8_t base = Settings::brightness();
        uint8_t desiredContrast = night ? static_cast<uint8_t>(base / 3) : base;
        if (desiredContrast < 8)
            desiredContrast = 8;
        static int16_t lastContrast = -1;
        if (lastContrast != static_cast<int16_t>(desiredContrast))
        {
            Canvas::setContrast(desiredContrast);
            lastContrast = desiredContrast;
        }
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);

        // A running focus session takes over the whole screen.
        if (Pomodoro::active())
        {
            ScreenPomodoro::draw();
            display.display();
            return;
        }

        // Digital clock emote owns the whole screen (no weather/clock header).
        if (Settings::mood() == MOOD_CLOCK)
        {
            ScreenClock::draw();
            display.display();
            return;
        }

        // Sticky note emote owns the whole screen (centered auto-sized text).
        if (Settings::mood() == MOOD_STICKY)
        {
            ScreenSticky::draw();
            display.display();
            return;
        }

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
