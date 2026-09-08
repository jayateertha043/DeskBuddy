// Local time + greeting derived from NTP and the weather-provided UTC offset.
#pragma once

#include <Arduino.h>

namespace Clock
{
  void beginNtp();
  void setUtcOffset(int32_t seconds);
  int32_t utcOffset();
  String localTimeString();
  int localHour();
  const __FlashStringHelper *greeting(int hour);
}
