#include "net/power_manager.h"

#include "config.h"
#include "app_state.h"
#include "core/clock.h"
#include "core/settings.h"
#include "display/canvas.h"
#include "net/weather_service.h"
#include "net/wifi_manager.h"

using namespace cfg;

namespace PowerManager
{
    namespace
    {
        uint32_t bootMs = 0;
        uint32_t nextFetchDue = 0; // when we next want fresh internet data
        uint32_t awakeSince = 0;   // millis() when the radio last woke for a task
        bool awakeForTask = false; // radio is up specifically to run a scheduled task

        bool bootWindowActive(uint32_t now)
        {
            return static_cast<int32_t>(now - (bootMs + BOOT_WIFI_WINDOW_MS)) < 0;
        }

        // Deep sleep until the quiet-hours window ends, then reset and boot fresh.
        void enterQuietSleep(uint32_t seconds, uint16_t endMin)
        {
            Serial.printf("[Sleep] Quiet hours: deep sleep %lu s (wake at %02u:%02u)\n",
                          static_cast<unsigned long>(seconds), endMin / 60, endMin % 60);
            if (Canvas::ready())
            {
                char wake[6];
                snprintf(wake, sizeof(wake), "%02u:%02u", endMin / 60, endMin % 60);
                Canvas::display.clearDisplay();
                Canvas::display.setTextColor(SSD1306_WHITE);
                Canvas::display.setTextSize(1);
                Canvas::display.setCursor(28, 20);
                Canvas::display.print(F("Quiet hours"));
                Canvas::display.setCursor(22, 36);
                Canvas::display.print(F("Waking at "));
                Canvas::display.print(wake);
                Canvas::display.display();
                delay(1500);
                Canvas::displayOff();
            }
            Clock::prepareForDeepSleep(); // keep the clock correct across the sleep
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            platformDeepSleep(static_cast<uint64_t>(seconds) * 1000000ULL);
        }

        // Indefinite deep sleep (wake only on manual reset) when WiFi never came up.
        void enterUnconfiguredSleep()
        {
            Serial.println(F("[Sleep] No WiFi within recovery window; deep sleep until reset."));
            if (Canvas::ready())
            {
                Canvas::display.clearDisplay();
                Canvas::display.setTextColor(SSD1306_WHITE);
                Canvas::display.setTextSize(1);
                Canvas::display.setCursor(20, 14);
                Canvas::display.print(F("WiFi not set up"));
                Canvas::display.setCursor(6, 30);
                Canvas::display.print(F("Sleeping to save power"));
                Canvas::display.setCursor(16, 46);
                Canvas::display.print(F("Reset to retry"));
                Canvas::display.display();
                delay(2000);
                Canvas::displayOff();
            }
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            platformDeepSleepForever();
        }

        // After the recovery window, if WiFi has never connected (bad settings /
        // no AP), stop draining power on the recovery AP and sleep until reset.
        // Applies in both normal and power-saving modes.
        void checkUnconfiguredTimeout()
        {
            if (WifiManager::everConnected())
                return;
            if (static_cast<int32_t>(millis() - (bootMs + BOOT_WIFI_WINDOW_MS)) < 0)
                return; // still inside the 30-min recovery window
            enterUnconfiguredSleep();
        }

        // If we're inside the configured quiet-hours window, power down now.
        void checkQuietHours()
        {
            if (!Settings::nightSleepEnabled())
                return;
            // Give the user a short grace after boot to reach the dashboard,
            // and never sleep while the setup portal is open.
            if (millis() < QUIET_SLEEP_BOOT_GRACE_MS || WifiManager::isPortalActive())
                return;
            // Only trust real (NTP) time so we never sleep at the wrong hour.
            struct tm t;
            if (!Clock::isSynced() || !Clock::localTime(t))
                return;

            const uint16_t start = Settings::nightSleepStart();
            const uint16_t end = Settings::nightSleepEnd();
            if (start == end)
                return; // zero-length window = disabled

            const uint16_t nowMin = t.tm_hour * 60 + t.tm_min;
            const bool inWindow = start < end
                                      ? (nowMin >= start && nowMin < end)
                                      : (nowMin >= start || nowMin < end); // crosses midnight
            if (!inWindow)
                return;

            uint16_t minsToEnd = (end + 1440 - nowMin) % 1440;
            if (minsToEnd == 0)
                minsToEnd = 1440;
            uint32_t secsToEnd = static_cast<uint32_t>(minsToEnd) * 60 - t.tm_sec;
            if (secsToEnd < 1)
                secsToEnd = 1;
            enterQuietSleep(secsToEnd, end);
        }
    }

    void begin()
    {
        bootMs = millis();
        // First on-demand fetch is scheduled for the moment the boot window ends.
        nextFetchDue = bootMs + BOOT_WIFI_WINDOW_MS;
        awakeForTask = false;
    }

    void resetBootWindow()
    {
        bootMs = millis();
        nextFetchDue = bootMs + BOOT_WIFI_WINDOW_MS;
        awakeForTask = false;
    }

    bool saving() { return Settings::powerMode() == POWER_SAVING; }
    bool inBootWindow() { return saving() && bootWindowActive(millis()); }
    bool radioParked() { return saving() && !WifiManager::radioEnabled(); }

    uint32_t msUntilNextWake()
    {
        if (!saving() || WifiManager::radioEnabled())
            return 0;
        const uint32_t wakeAt = nextFetchDue - WIFI_PREFETCH_LEAD_MS;
        const int32_t delta = static_cast<int32_t>(wakeAt - millis());
        return delta > 0 ? static_cast<uint32_t>(delta) : 0;
    }

    void loop()
    {
        // Scheduled quiet-hours deep sleep runs regardless of the power mode.
        checkQuietHours();
        // Give up (deep sleep until reset) if WiFi never connected; both modes.
        checkUnconfiguredTimeout();

        // Power saving off: make sure the radio is on and do nothing else.
        if (!saving())
        {
            if (!WifiManager::radioEnabled())
                WifiManager::setRadioEnabled(true);
            return;
        }

        const uint32_t now = millis();

        // Always-on window right after boot: keep WiFi + dashboard available.
        if (bootWindowActive(now))
        {
            if (!WifiManager::radioEnabled())
                WifiManager::setRadioEnabled(true);
            return;
        }

        // Never park until we've had at least one good STA connection. Bad
        // credentials / an unreachable router must keep the radio on so the
        // recovery AP portal and dashboard stay reachable for reconfiguration.
        if (!WifiManager::everConnected())
        {
            if (!WifiManager::radioEnabled())
                WifiManager::setRadioEnabled(true);
            return;
        }

        // Leave the radio alone while the recovery portal is up (user setup).
        if (WifiManager::isPortalActive())
            return;

        if (!awakeForTask)
        {
            const uint32_t wakeAt = nextFetchDue - WIFI_PREFETCH_LEAD_MS;
            if (static_cast<int32_t>(now - wakeAt) >= 0)
            {
                // Time to wake ahead of the scheduled internet task.
                awakeForTask = true;
                awakeSince = now;
                WifiManager::setRadioEnabled(true);
                Clock::beginNtp();            // opportunistic time resync
                WeatherService::requestNow(); // fetch as soon as we associate
            }
            else if (WifiManager::radioEnabled())
            {
                WifiManager::setRadioEnabled(false);
            }
        }
        else
        {
            // Awake for a task: sleep again once the data lands, or on timeout
            // ONLY if WiFi is actually connected (a transient fetch/API failure).
            // If we can't associate, stay awake so the recovery AP stays up.
            const bool connected = WiFi.status() == WL_CONNECTED;
            const uint32_t updatedAt = WeatherService::data().updatedAt;
            const bool fetched = updatedAt != 0 &&
                                 static_cast<int32_t>(updatedAt - awakeSince) >= 0;
            const bool timedOut =
                static_cast<int32_t>(now - (awakeSince + POWER_SAVE_MAX_AWAKE_MS)) >= 0;
            if (fetched || (timedOut && connected))
            {
                awakeForTask = false;
                nextFetchDue = now + WEATHER_INTERVAL_MS;
                WifiManager::setRadioEnabled(false);
            }
        }
    }
}
