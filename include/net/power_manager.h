// On-demand WiFi power scheduler.
// In POWER_SAVING mode the radio is parked and only woken shortly before a
// scheduled internet task (weather / NTP). WiFi + dashboard stay on for a
// fixed window after every fresh boot. In POWER_NONE the radio is left on.
#pragma once

#include <Arduino.h>

namespace PowerManager
{
    void begin();           // call once in setup(), after Settings::load()
    void loop();            // call each iteration, before WifiManager::loop()
    void resetBootWindow(); // restart the always-on window (e.g. when enabling saving)

    bool inBootWindow();        // true while the post-boot always-on window is active
    bool saving();              // true when power saving is the active mode
    bool radioParked();         // true when the radio is currently parked to save power
    uint32_t msUntilNextWake(); // ms until the radio next wakes (0 if already awake)
}
