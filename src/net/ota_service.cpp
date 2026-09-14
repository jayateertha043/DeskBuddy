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

        ArduinoOTA.onStart([]()
                           {
            String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
            Serial.println("OTA: Starting " + type + " update..."); });

        ArduinoOTA.onEnd([]()
                         {
            Serial.println("\nOTA: Update complete!");
            status_msg = "Update successful!"; });

        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                              {
            static unsigned long lastPrint = 0;
            unsigned long now = millis();
            if (now - lastPrint > 500) {
                lastPrint = now;
                unsigned int percent = (progress / (total / 100));
                Serial.print("OTA Progress: ");
                Serial.print(percent);
                Serial.println("%");
            } });

        ArduinoOTA.onError([](ota_error_t error)
                           {
            Serial.print("OTA Error: ");
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
            else Serial.println("Unknown Error");
            status_msg = "OTA failed"; });

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

        ArduinoOTA.onStart([]()
                           { Serial.println("OTA: Starting update..."); });

        ArduinoOTA.onEnd([]()
                         {
            Serial.println("\nOTA: Update complete!");
            status_msg = "Update successful!"; });

        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                              {
            static unsigned long lastPrint = 0;
            unsigned long now = millis();
            if (now - lastPrint > 500) {
                lastPrint = now;
                unsigned int percent = (progress / (total / 100));
                Serial.print("OTA Progress: ");
                Serial.print(percent);
                Serial.println("%");
            } });

        ArduinoOTA.onError([](ota_error_t error)
                           {
            Serial.print("OTA Error: ");
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
            else Serial.println("Unknown Error");
            status_msg = "OTA failed"; });

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
}
#endif
