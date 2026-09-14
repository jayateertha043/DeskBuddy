// Over-The-Air (OTA) firmware update service.
#pragma once

#include <Arduino.h>

namespace OtaService
{
    // Initialize OTA service (called once during setup)
    void init();

    // Process OTA operations in the main loop
    void update();

    // Get current OTA status message
    const char *status();
}
