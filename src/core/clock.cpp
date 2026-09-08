#include "core/clock.h"

#include <time.h>

namespace Clock
{
    namespace
    {
        int32_t utcOffsetSeconds = 0;
    }

    void beginNtp()
    {
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    }

    void setUtcOffset(int32_t seconds) { utcOffsetSeconds = seconds; }
    int32_t utcOffset() { return utcOffsetSeconds; }

    String localTimeString()
    {
        const time_t utcNow = time(nullptr);
        if (utcNow < 100000)
            return String();
        const time_t localNow = utcNow + utcOffsetSeconds;
        struct tm parts{};
        gmtime_r(&localNow, &parts);
        char clockText[6];
        snprintf(clockText, sizeof(clockText), "%02d:%02d", parts.tm_hour, parts.tm_min);
        return String(clockText);
    }

    int localHour()
    {
        const time_t utcNow = time(nullptr);
        if (utcNow < 100000)
            return -1;
        const time_t localNow = utcNow + utcOffsetSeconds;
        struct tm parts{};
        gmtime_r(&localNow, &parts);
        return parts.tm_hour;
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
