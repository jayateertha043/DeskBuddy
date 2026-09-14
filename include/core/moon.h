// Moon phase calculation. Lightweight algorithm based on days since epoch.
// Inspired by open-source astronomy libraries (e.g., ephem, skyfield).
#pragma once

#include <Arduino.h>

namespace Moon
{
    // Moon phase as a fraction [0, 1], where:
    // 0.0 = new moon
    // 0.25 = first quarter
    // 0.5 = full moon
    // 0.75 = last quarter
    float getPhase(uint32_t unixTime);

    // Moon phase names (8 phases)
    enum Phase : uint8_t
    {
        PHASE_NEW = 0,           // 0.00-0.06
        PHASE_WAXING_CRESCENT,   // 0.06-0.25
        PHASE_FIRST_QUARTER,     // 0.25-0.31
        PHASE_WAXING_GIBBOUS,    // 0.31-0.50
        PHASE_FULL,              // 0.50-0.69
        PHASE_WANING_GIBBOUS,    // 0.69-0.75
        PHASE_LAST_QUARTER,      // 0.75-0.94
        PHASE_WANING_CRESCENT    // 0.94-1.00
    };

    // Get discrete phase from fractional phase
    Phase getDiscretePhase(float phase);

    // Get emoji representation of moon phase
    const char *getEmoji(Phase phase);
    const char *getLabel(Phase phase);
}
