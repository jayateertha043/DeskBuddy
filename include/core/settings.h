// Persisted user settings + in-RAM mirror. Single owner of NVS (Preferences).
#pragma once

#include <Arduino.h>

namespace Settings
{
    void load();

    const String &ssid();
    const String &pass();
    const String &city();
    const String &country();
    double lat();
    double lon();
    bool resolved();
    const String &status();
    uint8_t mood();
    uint8_t emoteMode();
    bool randomMode(); // deprecated: use emoteMode() == EMOTE_RANDOM
    const String &name();
    uint16_t pomodoroMinutes();
    uint8_t brightnessPercent(); // 5-100 slider value
    uint8_t brightness();        // mapped 0-255 OLED contrast
    uint8_t loopCount();         // number of emotes in the loop playlist
    uint8_t loopAt(uint8_t i);   // mood index at loop position i
    uint16_t loopSeconds();      // seconds each loop emote is shown
    const String &stickyText();  // user text shown by the Sticky Note emote
    uint8_t powerMode();         // 0=none (always on), 1=power saving (on-demand WiFi)
    bool nightSleepEnabled();    // quiet-hours deep sleep on/off
    uint16_t nightSleepStart();  // quiet-hours start, minutes past midnight (0-1439)
    uint16_t nightSleepEnd();    // quiet-hours end / wake time, minutes past midnight

    void saveCreds(const String &ssid, const String &pass);
    void saveMood(uint8_t m);         // persist + apply
    void setMood(uint8_t m);          // apply only (no flash write)
    void saveEmoteMode(uint8_t mode); // 0=static, 1=random, 2=loop
    void saveName(const String &name);
    void saveLocationInput(const String &city, const String &country);
    void applyResolvedLocation(const String &city, const String &country,
                               double lat, double lon);
    void setStatus(const String &status);
    void savePomodoroMinutes(uint16_t minutes);
    void saveBrightness(uint8_t percent);     // clamp 5-100 + persist
    void saveLoopSequence(const String &csv); // ordered CSV of mood indices
    void saveLoopSeconds(uint16_t seconds);   // clamp 3-300 + persist
    void saveStickyText(const String &text);  // persist Sticky Note text (UTF-8)
    void savePowerMode(uint8_t mode);         // 0=none, 1=power saving
    void saveNightSleep(bool enabled, uint16_t startMin, uint16_t endMin);
}
