// Local time + greeting derived from NTP and the weather-provided UTC offset.
#pragma once

#include <Arduino.h>
#include <time.h>

namespace Clock
{
    void beginNtp();
    void initBuildTime();  // Initialize compile-time fallback (call once in setup)
    void prepareForDeepSleep(); // stash time state in RTC memory before deep sleep
    void restoreAfterWake();    // restore RTC-retained time state after a timer wake
    bool isSynced();       // true once real NTP time is available (not the build fallback)
    void setUtcOffset(int32_t seconds);
    int32_t utcOffset();
    String localTimeString();
    int localHour();
    bool localTime(struct tm &out); // full local time parts; false if not yet available
    const __FlashStringHelper *greeting(int hour);
}
