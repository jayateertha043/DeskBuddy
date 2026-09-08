#include "core/settings.h"

#include "config.h"
#include "app_state.h"
#include "core/util.h"

namespace Settings
{
  namespace
  {
    Preferences prefs;
    String staSsid;
    String staPass;
    String locCity = LOCATION_CITY;
    String locCountry = LOCATION_COUNTRY;
    double locLat = LOCATION_LATITUDE;
    double locLon = LOCATION_LONGITUDE;
    bool locResolved = true; // defaults ship pre-resolved
    String locStatus = LOCATION_CITY ", " LOCATION_COUNTRY;
    uint8_t moodSel = MOOD_AUTO;
    bool randomEmotes = false;
    String userName;
  }

  void load()
  {
    prefs.begin("deskbuddy", true);
    staSsid = prefs.getString("ssid", WIFI_SSID);
    staPass = prefs.getString("pass", WIFI_PASSWORD);
    locCity = prefs.getString("city", LOCATION_CITY);
    locCountry = prefs.getString("country", LOCATION_COUNTRY);
    locLat = prefs.getDouble("lat", LOCATION_LATITUDE);
    locLon = prefs.getDouble("lon", LOCATION_LONGITUDE);
    locResolved = prefs.getBool("locok", true) && Util::coordinatesValid(locLat, locLon);
    moodSel = prefs.getUChar("mood", MOOD_AUTO);
    if (moodSel >= MOOD_COUNT)
      moodSel = MOOD_AUTO;
    randomEmotes = prefs.getBool("random", false);
    if (randomEmotes)
      moodSel = MOOD_AUTO; // Random mode rests on the weather face between reactions.
    userName = prefs.getString("name", "");
    prefs.end();
    locStatus = locResolved ? locCity + ", " + locCountry
                            : String(F("Waiting to locate ")) + locCity;
  }

  const String &ssid() { return staSsid; }
  const String &pass() { return staPass; }
  const String &city() { return locCity; }
  const String &country() { return locCountry; }
  double lat() { return locLat; }
  double lon() { return locLon; }
  bool resolved() { return locResolved; }
  const String &status() { return locStatus; }
  uint8_t mood() { return moodSel; }
  bool randomMode() { return randomEmotes; }
  const String &name() { return userName; }

  void saveCreds(const String &ssid, const String &pass)
  {
    prefs.begin("deskbuddy", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
    staSsid = ssid;
    staPass = pass;
  }

  void saveMood(uint8_t m)
  {
    if (m >= MOOD_COUNT)
      m = MOOD_AUTO;
    moodSel = m;
    prefs.begin("deskbuddy", false);
    prefs.putUChar("mood", m);
    prefs.end();
  }

  void setMood(uint8_t m)
  {
    if (m >= MOOD_COUNT)
      m = MOOD_AUTO;
    moodSel = m;
  }

  void saveEmoteMode(bool useRandom)
  {
    randomEmotes = useRandom;
    prefs.begin("deskbuddy", false);
    prefs.putBool("random", useRandom);
    prefs.end();
  }

  void saveName(const String &name)
  {
    userName = name;
    prefs.begin("deskbuddy", false);
    prefs.putString("name", name);
    prefs.end();
  }

  void saveLocationInput(const String &city, const String &country)
  {
    prefs.begin("deskbuddy", false);
    prefs.putString("city", city);
    prefs.putString("country", country);
    prefs.putBool("locok", false);
    prefs.end();
    locCity = city;
    locCountry = country;
    locResolved = false;
    locStatus = String(F("Waiting to locate ")) + city;
  }

  void applyResolvedLocation(const String &city, const String &country,
                             double lat, double lon)
  {
    locLat = lat;
    locLon = lon;
    if (city.length())
      locCity = city;
    if (country.length())
      locCountry = country;
    locResolved = true;
    locStatus = locCity + ", " + locCountry;
    prefs.begin("deskbuddy", false);
    prefs.putString("city", locCity);
    prefs.putString("country", locCountry);
    prefs.putDouble("lat", locLat);
    prefs.putDouble("lon", locLon);
    prefs.putBool("locok", true);
    prefs.end();
  }

  void setStatus(const String &status) { locStatus = status; }
}
