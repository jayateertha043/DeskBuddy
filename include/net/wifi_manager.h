// STA connection, reconnect, recovery AP portal, mDNS, status LED.
#pragma once

#include <Arduino.h>

namespace WifiManager
{
    void initLed();
    void begin();     // start STA connection (settings must be loaded first)
    void reconnect(); // re-apply credentials without resetting offline timer
    void startPortal();
    void stopPortal();
    bool isPortalActive();
    void handlePortalDns();
    void handleMdns();
    void loop();
}
