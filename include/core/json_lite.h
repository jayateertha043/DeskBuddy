// Minimal JSON field extraction for Open-Meteo responses.
#pragma once

#include <Arduino.h>

namespace Json
{
  // Reads a number after the first "current": object (weather forecast).
  float number(const String &body, const char *key, float fallback);
  // Reads a number for the first key found at/after `from`.
  float numberFrom(const String &body, const char *key, int from, float fallback);
  // Reads a string value for the first key found within [from, until).
  String stringFrom(const String &body, const char *key, int from, int until);
}
