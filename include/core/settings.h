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
  bool randomMode();
  const String &name();

  void saveCreds(const String &ssid, const String &pass);
  void saveMood(uint8_t m);   // persist + apply
  void setMood(uint8_t m);    // apply only (no flash write)
  void saveEmoteMode(bool useRandom);
  void saveName(const String &name);
  void saveLocationInput(const String &city, const String &country);
  void applyResolvedLocation(const String &city, const String &country,
                             double lat, double lon);
  void setStatus(const String &status);
}
