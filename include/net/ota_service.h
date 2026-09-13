// Over-The-Air (OTA) firmware update service.
#pragma once

#include <Arduino.h>

namespace OtaService
{
    // Initialize OTA service (called once during setup)
    void init();

    // Process OTA operations in the main loop
    void update();

    // Get current OTA status message (e.g., "Ready", "Uploading...", "Success!")
    const char *status();

    // Check if OTA is currently in progress
    bool isUpdating();

    // Check if an update is available (for future use with remote version checking)
    bool updateAvailable();

    // Begin firmware update with expected size
    void beginUpdate(size_t size);

    // Write firmware data chunk
    size_t writeUpdateData(const uint8_t *data, size_t len);

    // Finalize firmware update
    void endUpdate();

    // Cancel ongoing firmware update
    void cancelUpdate();
}
