#include "net/wifi_manager.h"

#include "config.h"
#include "core/settings.h"
#include "net/weather_service.h"
#include "net/web_portal.h"

using namespace cfg;

namespace WifiManager
{
    namespace
    {
        DNSServer dnsServer;
        bool portalActive = false;
        bool mdnsStarted = false;
        bool radioIntendedOn = true; // false only when power-saving parks the radio
        bool connectedOnce = false;  // latches true after the first successful STA connect
        uint32_t nextWifiRetry = 0;
        uint32_t offlineSince = 0;

        void beginWifi()
        {
            WiFi.persistent(false);
            if (!portalActive)
                WiFi.mode(WIFI_STA);
            // ESP32-C3 Super Mini has a poor PCB antenna; full TX power causes RF
            // distortion and auth/assoc failures. Reducing TX power is a known fix.
            platformWifiLowPower();
            platformWifiSleep(false);
            WiFi.setAutoReconnect(true);
            platformWifiHostname("deskbuddy");
            WiFi.begin(Settings::ssid().c_str(), Settings::pass().c_str());
            Serial.printf("Connecting to %s...\n", Settings::ssid().c_str());
        }
    }

    void initLed()
    {
        pinMode(STATUS_LED, OUTPUT);
        digitalWrite(STATUS_LED, HIGH); // off (active LOW)
    }

    void begin()
    {
        beginWifi();
        offlineSince = millis();
    }

    void reconnect() { beginWifi(); }

    bool radioEnabled() { return radioIntendedOn; }

    bool everConnected() { return connectedOnce; }

    void setRadioEnabled(bool on)
    {
        if (on == radioIntendedOn)
            return;
        radioIntendedOn = on;
        if (on)
        {
            Serial.println(F("Power: waking WiFi radio"));
            nextWifiRetry = 0;
            offlineSince = millis();
            beginWifi();
        }
        else
        {
            Serial.println(F("Power: parking WiFi radio"));
            stopPortal();
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            digitalWrite(STATUS_LED, HIGH); // off
        }
    }

    bool isPortalActive() { return portalActive; }

    void startPortal()
    {
        if (portalActive)
            return;
        Serial.println(F("Starting recovery AP portal..."));
        WiFi.mode(WIFI_AP_STA);
        platformWifiSleep(false); // modem sleep is unreliable while the SoftAP is up
        platformWifiLowPower();
        WiFi.softAP(AP_SSID, AP_PASSWORD);
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
        WebPortal::start();
        portalActive = true;
        Serial.printf("Portal: connect to \"%s\" (pw %s) -> http://%s\n",
                      AP_SSID, AP_PASSWORD, WiFi.softAPIP().toString().c_str());
    }

    void stopPortal()
    {
        if (!portalActive)
            return;
        Serial.println(F("Closing recovery AP portal."));
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        portalActive = false;
    }

    void handlePortalDns()
    {
        if (portalActive)
            dnsServer.processNextRequest();
    }

    void handleMdns()
    {
        if (mdnsStarted)
            platformMdnsUpdate();
    }

    void loop()
    {
        static wl_status_t previousStatus = WL_IDLE_STATUS;
        const wl_status_t currentStatus = WiFi.status();
        if (currentStatus == WL_CONNECTED && previousStatus != WL_CONNECTED)
        {
            Serial.printf("Connected. IP: %s\n", WiFi.localIP().toString().c_str());
            connectedOnce = true;
            digitalWrite(STATUS_LED, LOW); // on
            WeatherService::requestNow();
            stopPortal();
            // Enable modem sleep now that we're associated, to cut idle power.
            platformWifiSleep(true);
            WebPortal::start();
            if (!mdnsStarted && MDNS.begin("deskbuddy"))
            {
                MDNS.addService("http", "tcp", 80);
                mdnsStarted = true;
            }
            Serial.printf("Dashboard: http://%s  (or http://deskbuddy.local)\n",
                          WiFi.localIP().toString().c_str());
        }
        else if (currentStatus != WL_CONNECTED && previousStatus == WL_CONNECTED)
        {
            digitalWrite(STATUS_LED, HIGH); // off
            offlineSince = millis();
        }
        previousStatus = currentStatus;

        // When the radio is intentionally parked (power saving), skip all
        // reconnect and recovery-portal work until it is woken again.
        if (!radioIntendedOn)
            return;

        if (currentStatus != WL_CONNECTED &&
            static_cast<int32_t>(millis() - nextWifiRetry) >= 0)
        {
            nextWifiRetry = millis() + WIFI_RECONNECT_MS;
            beginWifi();
        }

        // Open the recovery portal after being offline too long.
        if (currentStatus != WL_CONNECTED && !portalActive &&
            millis() - offlineSince >= PORTAL_AFTER_MS)
        {
            startPortal();
        }
    }
}
