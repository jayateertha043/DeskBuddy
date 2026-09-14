#include "display/screen_clock.h"

#include <Arduino.h>
#include <time.h>

#include "app_state.h"
#include "core/clock.h"
#include "display/canvas.h"
#include "display/screen_weather.h"
#include "net/weather_service.h"

using Canvas::display;

namespace ScreenClock
{
    namespace
    {
        const char *const kWeekday[7] = {"Sun", "Mon", "Tue", "Wed",
                                         "Thu", "Fri", "Sat"};
        const char *const kMonth[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    }

    // Layout for a 128x64 OLED: big HH:MM up top, date strip, weather forecast row.
    void draw()
    {
        struct tm t{};
        const bool haveTime = Clock::localTime(t);

        // --- Big digital time HH:MM (size 3 = 18px wide, 24px tall cells) ---
        char hh[3], mm[3];
        if (haveTime)
        {
            snprintf(hh, sizeof(hh), "%02d", t.tm_hour);
            snprintf(mm, sizeof(mm), "%02d", t.tm_min);
        }
        else
        {
            strcpy(hh, "--");
            strcpy(mm, "--");
        }

        const int startX = 19; // (128 - 90) / 2 for "HH:MM"
        const int topY = 2;
        display.setTextSize(3);
        display.setCursor(startX, topY);
        display.print(hh);
        // Colon blinks once per second for a live-clock feel.
        const bool showColon = !haveTime || (t.tm_sec % 2 == 0);
        display.setCursor(startX + 36, topY);
        display.print(showColon ? ":" : " ");
        display.setCursor(startX + 54, topY);
        display.print(mm);

        // Seconds shown small in the top-right corner.
        display.setTextSize(1);
        if (haveTime)
        {
            char ss[3];
            snprintf(ss, sizeof(ss), "%02d", t.tm_sec);
            display.setCursor(115, 2);
            display.print(ss);
        }

        // --- Date strip ---
        display.drawLine(0, 27, 127, 27, SSD1306_WHITE);
        if (haveTime)
        {
            char dateBuf[20];
            snprintf(dateBuf, sizeof(dateBuf), "%s %02d %s %04d",
                     kWeekday[t.tm_wday % 7], t.tm_mday,
                     kMonth[t.tm_mon % 12], t.tm_year + 1900);
            const int w = static_cast<int>(strlen(dateBuf)) * 6;
            display.setCursor((128 - w) / 2, 31);
            display.print(dateBuf);
        }
        else
        {
            display.setCursor(22, 31);
            display.print(F("Syncing time..."));
        }

        // --- Weather forecast row ---
        display.drawLine(0, 42, 127, 42, SSD1306_WHITE);
        display.setTextSize(1);
        const Weather &weather = WeatherService::data();
        if (weather.valid)
        {
            // Line 1: icon + temperature (left), day/night (right).
            ScreenWeather::drawIcon(2, 45, weather.code);
            display.setCursor(22, 47);
            display.printf("%.1fC", weather.temperature);
            const __FlashStringHelper *phase =
                weather.isDay ? F("Day") : F("Night");
            const int phaseW = (weather.isDay ? 3 : 5) * 6;
            display.setCursor(126 - phaseW, 47);
            display.print(phase);

            // Line 2: condition summary, kept clear of the icon on the left.
            String summary = weather.summary;
            if (summary.length() > 17)
                summary = summary.substring(0, 17);
            display.setCursor(22, 56);
            display.print(summary);
        }
        else
        {
            display.setCursor(10, 50);
            display.print(F("Weather unavailable"));
        }
    }
}
