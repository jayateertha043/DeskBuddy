// STA connection, reconnect, recovery AP portal, mDNS, status LED.
#pragma once

#include <Arduino.h>

namespace WifiManager
{
    void initLed();
    void begin();                  // start STA connection (settings must be loaded first)
    void reconnect();              // re-apply credentials without resetting offline timer
    void setRadioEnabled(bool on); // power manager control: false cuts the radio
    bool radioEnabled();           // whether the radio is intended to be on
    bool everConnected();          // true once an STA connection has succeeded (creds are good)
    void startPortal();
    void stopPortal();
    bool isPortalActive();
    void handlePortalDns();
    void handleMdns();
    void loop();
}
