// Schedules random emote reactions when Random mode is enabled.
#pragma once

#include <Arduino.h>

namespace EmoteDirector
{
    void update();                    // called every loop; drives the cadence
    void resetSchedule();             // restart the interval, clear any reaction
    void setNextInterval();           // arm the next reaction window
    void startReaction(uint8_t mood); // begin a timed reaction on the given mood
    void clearReaction();             // end the current reaction immediately
}
