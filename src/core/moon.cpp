#include "core/moon.h"

#include <math.h>

namespace Moon
{
    // Known new moon: January 6, 2000, 18:14 UTC (JD 2451550.261)
    // This is a well-established reference in astronomy.
    // Synodic month (lunar cycle): 29.530588861 days
    constexpr double KNOWN_NEW_MOON_JD = 2451550.261;
    constexpr double SYNODIC_MONTH = 29.530588861;

    // Convert Unix timestamp (seconds since 1970-01-01) to Julian Day Number
    double unixToJD(uint32_t unixTime)
    {
        // Julian Day for Unix epoch (1970-01-01 00:00:00 UTC) is 2440587.5
        constexpr double UNIX_EPOCH_JD = 2440587.5;
        // Convert Unix seconds to days and add to epoch
        return UNIX_EPOCH_JD + (static_cast<double>(unixTime) / 86400.0);
    }

    float getPhase(uint32_t unixTime)
    {
        double jd = unixToJD(unixTime);
        double daysSinceNewMoon = fmod(jd - KNOWN_NEW_MOON_JD, SYNODIC_MONTH);
        if (daysSinceNewMoon < 0)
            daysSinceNewMoon += SYNODIC_MONTH;
        return static_cast<float>(daysSinceNewMoon / SYNODIC_MONTH);
    }

    Phase getDiscretePhase(float phase)
    {
        // Normalize to [0, 1]
        phase = fmod(phase, 1.0f);
        if (phase < 0)
            phase += 1.0f;

        // Map to 8 phases
        if (phase < 0.0625f)
            return PHASE_NEW;
        else if (phase < 0.25f)
            return PHASE_WAXING_CRESCENT;
        else if (phase < 0.3125f)
            return PHASE_FIRST_QUARTER;
        else if (phase < 0.50f)
            return PHASE_WAXING_GIBBOUS;
        else if (phase < 0.6875f)
            return PHASE_FULL;
        else if (phase < 0.75f)
            return PHASE_WANING_GIBBOUS;
        else if (phase < 0.9375f)
            return PHASE_LAST_QUARTER;
        else
            return PHASE_WANING_CRESCENT;
    }

    const char *getEmoji(Phase phase)
    {
        // Unicode moon emojis (works on systems with emoji support)
        // Fallback ASCII art if emojis unavailable
        switch (phase)
        {
        case PHASE_NEW:
            return "🌑";  // new moon
        case PHASE_WAXING_CRESCENT:
            return "🌒";  // waxing crescent
        case PHASE_FIRST_QUARTER:
            return "🌓";  // first quarter
        case PHASE_WAXING_GIBBOUS:
            return "🌔";  // waxing gibbous
        case PHASE_FULL:
            return "🌕";  // full moon
        case PHASE_WANING_GIBBOUS:
            return "🌖";  // waning gibbous
        case PHASE_LAST_QUARTER:
            return "🌗";  // last quarter
        case PHASE_WANING_CRESCENT:
            return "🌘";  // waning crescent
        default:
            return "🌑";
        }
    }

    const char *getLabel(Phase phase)
    {
        switch (phase)
        {
        case PHASE_NEW:
            return "New Moon";
        case PHASE_WAXING_CRESCENT:
            return "Waxing Crescent";
        case PHASE_FIRST_QUARTER:
            return "First Quarter";
        case PHASE_WAXING_GIBBOUS:
            return "Waxing Gibbous";
        case PHASE_FULL:
            return "Full Moon";
        case PHASE_WANING_GIBBOUS:
            return "Waning Gibbous";
        case PHASE_LAST_QUARTER:
            return "Last Quarter";
        case PHASE_WANING_CRESCENT:
            return "Waning Crescent";
        default:
            return "Moon";
        }
    }
}
