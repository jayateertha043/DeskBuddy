// Small text/geo helpers shared across modules.
#pragma once

#include <Arduino.h>

namespace Util
{
  String urlEncode(const String &value);
  String htmlEscape(const String &in);
  bool coordinatesValid(double lat, double lon);
}
