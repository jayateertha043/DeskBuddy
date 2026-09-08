// Weather + geocoding via Open-Meteo. Owns the current Weather snapshot.
#pragma once

#include "app_state.h"

namespace WeatherService
{
  const Weather &data();
  void requestNow();             // fetch weather at the next loop tick
  void requestLocationRefresh(); // re-resolve city/country immediately
  bool resolveLocation();
  void updateWeather();
  void loop();
}
