#include "net/ota_service.h"

#ifdef ESP32

#include <ArduinoOTA.h>

namespace OtaService
{
    namespace
    {
        String status_msg = "Ready";
    }

    void init()
    {
        // Configure ArduinoOTA
        ArduinoOTA.setHostname("deskbuddy");
        
        ArduinoOTA.onStart([]() {
            String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
            Serial.println("OTA: Starting " + type + " update...");
        });

        ArduinoOTA.onEnd([]() {
            Serial.println("\nOTA: Update complete!");
            status_msg = "Update successful!";
        });

        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            static unsigned long lastPrint = 0;
            unsigned long now = millis();
            if (now - lastPrint > 500) {
                lastPrint = now;
                unsigned int percent = (progress / (total / 100));
                Serial.print("OTA Progress: ");
                Serial.print(percent);
                Serial.println("%");
            }
        });

        ArduinoOTA.onError([](ota_error_t error) {
            Serial.print("OTA Error: ");
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
            else Serial.println("Unknown Error");
            status_msg = "OTA failed";
        });

        ArduinoOTA.begin();
        status_msg = "Ready";
    }

    void update()
    {
        ArduinoOTA.handle();
    }

    const char *status()
    {
        return status_msg.c_str();
    }

    bool isUpdating()
    {
        return false; // ArduinoOTA handles its own state internally
    }

    bool updateAvailable()
    {
        return false; // TODO: Implement version checking
    }

    // Unused functions (kept for API compatibility)
    void beginUpdate(size_t size) {}
    size_t writeUpdateData(const uint8_t *data, size_t len) { return 0; }
    void endUpdate() {}
    void cancelUpdate() {}
}

#else
// ESP8266 fallback with ArduinoOTA support
#include <ArduinoOTA.h>

namespace OtaService
{
    namespace
    {
        String status_msg = "Ready";
    }

    void init()
    {
        ArduinoOTA.setHostname("deskbuddy");
        
        ArduinoOTA.onStart([]() {
            Serial.println("OTA: Starting update...");
        });

        ArduinoOTA.onEnd([]() {
            Serial.println("\nOTA: Update complete!");
            status_msg = "Update successful!";
        });

        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            static unsigned long lastPrint = 0;
            unsigned long now = millis();
            if (now - lastPrint > 500) {
                lastPrint = now;
                unsigned int percent = (progress / (total / 100));
                Serial.print("OTA Progress: ");
                Serial.print(percent);
                Serial.println("%");
            }
        });

        ArduinoOTA.onError([](ota_error_t error) {
            Serial.print("OTA Error: ");
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
            else Serial.println("Unknown Error");
            status_msg = "OTA failed";
        });

        ArduinoOTA.begin();
        status_msg = "Ready";
    }

    void update()
    {
        ArduinoOTA.handle();
    }

    const char *status()
    {
        return status_msg.c_str();
    }

    bool isUpdating()
    {
        return false;
    }

    bool updateAvailable()
    {
        return false;
    }

    void beginUpdate(size_t size) {}
    size_t writeUpdateData(const uint8_t *data, size_t len) { return 0; }
    void endUpdate() {}
    void cancelUpdate() {}
}
#endif
