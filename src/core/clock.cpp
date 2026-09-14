#include "core/clock.h"

#include <time.h>
#include <cstring>

namespace Clock
{
    namespace
    {
        int32_t utcOffsetSeconds = 0;
        time_t buildTimestamp = 0;   // Unix timestamp of build time
        uint32_t buildMillis = 0;    // millis() when buildTimestamp was recorded
        bool buildTimeInitialized = false;

        // Parse __DATE__ ("Sep 14 2026") and __TIME__ ("14:30:45") into Unix timestamp
        time_t parseCompileTime()
        {
            struct tm t = {};
            const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

            int day, year, hour, minute, second;
            char monthStr[4];

            // Parse __DATE__ = "Sep 14 2026"
            sscanf(__DATE__, "%3s %d %d", monthStr, &day, &year);
            // Parse __TIME__ = "14:30:45"
            sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

            // Find month index (0-11)
            int month = 0;
            for (int i = 0; i < 12; i++)
            {
                if (strncmp(monthStr, months[i], 3) == 0)
                {
                    month = i;
                    break;
                }
            }

            t.tm_year = year - 1900;  // Years since 1900
            t.tm_mon = month;          // 0-11
            t.tm_mday = day;           // 1-31
            t.tm_hour = hour;          // 0-23
            t.tm_min = minute;         // 0-59
            t.tm_sec = second;         // 0-59
            t.tm_isdst = -1;           // Auto-detect DST

            return mktime(&t);
        }
    }

    void beginNtp()
    {
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    }

    void initBuildTime()
    {
        if (!buildTimeInitialized)
        {
            buildTimestamp = parseCompileTime();
            buildMillis = millis();
            buildTimeInitialized = true;
            Serial.printf("[Clock] Build timestamp: %lld, init at millis: %lu\n",
                          (long long)buildTimestamp, buildMillis);
        }
    }

    void setUtcOffset(int32_t seconds) { utcOffsetSeconds = seconds; }
    int32_t utcOffset() { return utcOffsetSeconds; }

    String localTimeString()
    {
        time_t timeToUse;
        const time_t utcNow = time(nullptr);

        if (utcNow >= 100000)
        {
            // NTP has synced, use real time
            timeToUse = utcNow + utcOffsetSeconds;
        }
        else if (buildTimeInitialized)
        {
            // Fallback: use build time + elapsed milliseconds
            uint32_t elapsedMs = millis() - buildMillis;
            timeToUse = buildTimestamp + (elapsedMs / 1000) + utcOffsetSeconds;
        }
        else
        {
            // Build time not initialized yet
            return String();
        }

        struct tm parts{};
        gmtime_r(&timeToUse, &parts);
        char clockText[6];
        snprintf(clockText, sizeof(clockText), "%02d:%02d", parts.tm_hour, parts.tm_min);
        return String(clockText);
    }

    int localHour()
    {
        struct tm parts{};
        return localTime(parts) ? parts.tm_hour : -1;
    }

    bool localTime(struct tm &out)
    {
        time_t timeToUse;
        const time_t utcNow = time(nullptr);

        if (utcNow >= 100000)
        {
            // NTP has synced, use real time
            timeToUse = utcNow + utcOffsetSeconds;
        }
        else if (buildTimeInitialized)
        {
            // Fallback: use build time + elapsed milliseconds
            uint32_t elapsedMs = millis() - buildMillis;
            timeToUse = buildTimestamp + (elapsedMs / 1000) + utcOffsetSeconds;
        }
        else
        {
            return false;
        }

        gmtime_r(&timeToUse, &out);
        return true;
    }

    const __FlashStringHelper *greeting(int hour)
    {
        if (hour < 0) // time not synced yet (no WiFi/NTP)
            return F("Welcome!");
        if (hour >= 5 && hour < 12)
            return F("Good morning!");
        if (hour >= 12 && hour < 17)
            return F("Good afternoon!");
        if (hour >= 17 && hour < 21)
            return F("Good evening!");
        return F("Good night!");
    }
}
