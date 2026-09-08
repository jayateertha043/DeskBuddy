// DeskBuddy: shared codebase for ESP32-C3 Super Mini (reference) and ESP8266.
// Hardcoded WiFi + location. STA-first; recovery AP portal if STA can't connect.
// All chip-specific differences live in include/platform.h.

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <math.h>
#include <time.h>

#include "platform.h"

// ---------------- User configuration ----------------
// Default credentials, used until different ones are saved via the portal.
#ifndef WIFI_SSID
#define WIFI_SSID "RandomWifi"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "12345678"
#endif

// Default location (Hosur, India), used until changed via the portal.
// City/country are resolved to coordinates on-device via Open-Meteo geocoding.
#ifndef LOCATION_CITY
#define LOCATION_CITY "Hosur"
#endif
#ifndef LOCATION_COUNTRY
#define LOCATION_COUNTRY "India"
#endif
#ifndef LOCATION_LATITUDE
#define LOCATION_LATITUDE 12.7409
#endif
#ifndef LOCATION_LONGITUDE
#define LOCATION_LONGITUDE 77.8253
#endif
// ----------------------------------------------------

namespace
{
  constexpr uint8_t OLED_WIDTH = 128;
  constexpr uint8_t OLED_HEIGHT = 64;
  constexpr uint8_t OLED_ADDRESS = 0x3C;
  constexpr uint8_t OLED_SDA = DESKBUDDY_OLED_SDA;
  constexpr uint8_t OLED_SCL = DESKBUDDY_OLED_SCL;
  constexpr uint8_t STATUS_LED = DESKBUDDY_STATUS_LED; // active LOW

  constexpr uint32_t WEATHER_INTERVAL_MS = 15UL * 60UL * 1000UL;
  constexpr uint32_t WEATHER_RETRY_MS = 60UL * 1000UL;
  constexpr uint32_t WIFI_RECONNECT_MS = 15UL * 1000UL;
  constexpr uint32_t PORTAL_AFTER_MS = 40UL * 1000UL; // open AP after this long offline
  constexpr uint32_t LOCATION_RETRY_MS = 60UL * 1000UL;
  constexpr uint32_t RANDOM_EMOTE_INTERVAL_MS = 10UL * 60UL * 1000UL;
  constexpr uint32_t RANDOM_EMOTE_DURATION_MS = 10UL * 1000UL;
  constexpr uint16_t DNS_PORT = 53;

  const char *const AP_SSID = "DeskBuddy-Setup";
  const char *const AP_PASSWORD = "12345678"; // >= 8 chars required by SoftAP

  enum Mood : uint8_t
  {
    MOOD_AUTO = 0,
    MOOD_HAPPY,
    MOOD_SAD,
    MOOD_EXCITED,
    MOOD_ANGRY,
    MOOD_STRETCH,
    MOOD_SNEEZE,
    MOOD_SLEEP,
    MOOD_CONFUSED,
    MOOD_CURIOUS,
    MOOD_DND,
    MOOD_YAWN,
    MOOD_GLANCE_LEFT,
    MOOD_GLANCE_RIGHT,
    MOOD_WINK,
    MOOD_LAUGH,
    MOOD_SURPRISED,
    MOOD_NERVOUS,
    MOOD_COUNT
  };
  const char *const MOOD_SLUG[MOOD_COUNT] = {
      "auto", "happy", "sad", "excited", "angry", "stretching",
      "sneezing", "sleeping", "confused", "curious", "dnd",
      "yawn", "glance-left", "glance-right", "wink", "laugh",
      "surprised", "nervous"};
  const char *const MOOD_LABEL[MOOD_COUNT] = {
      "Auto (weather)", "Happy", "Sad", "Excited", "Angry", "Stretching",
      "Sneezing", "Sleeping", "Confused", "Curious", "Do not disturb",
      "Yawn", "Look left", "Look right", "Wink", "Laugh", "Surprised",
      "Nervous"};
  const char *const MOOD_ICON[MOOD_COUNT] = {
      "~", "^_^", "T_T", "*o*", ">_<", "-o-", "achoo", "zZz",
      "?_-", "o_O", "...", "-O-", "<.<", ">.>", ";-)", "^o^",
      "O_O", "o~o"};

  struct Weather
  {
    bool valid = false;
    float temperature = 0;
    int code = -1;
    bool isDay = true;
    String summary = "Waiting";
    uint32_t updatedAt = 0;
  };

  Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
  WebServerClass server(80);
  DNSServer dnsServer;
  Preferences prefs;
  Weather weather;
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
  uint32_t nextRandomEmoteAt = 0;
  uint32_t randomEmoteEndsAt = 0;
  uint8_t lastRandomMood = MOOD_AUTO;
  String userName;
  uint32_t nextLocationAttempt = 0;
  bool oledReady = false;
  bool ntpStarted = false;
  bool portalActive = false;
  bool webStarted = false;
  bool mdnsStarted = false;
  int32_t utcOffsetSeconds = 0;
  uint32_t nextWeatherAttempt = 0;
  uint32_t nextWifiRetry = 0;
  uint32_t offlineSince = 0;
  uint32_t lastAnimationFrame = 0;
  uint16_t animationFrame = 0;

  enum BootStage : uint8_t
  {
    BOOT_WAITING = 0,
    BOOT_OPENING,
    BOOT_LOOK_LEFT,
    BOOT_LOOK_RIGHT,
    BOOT_DOUBLE_BLINK,
    BOOT_EXCITED,
    BOOT_GREETING,
    BOOT_INTRO,
    BOOT_COMPLETE
  };
  BootStage bootStage = BOOT_WAITING;
  uint32_t bootStageStartedAt = 0;

  float jsonNumber(const String &body, const char *key, float fallback)
  {
    const int currentStart = body.indexOf("\"current\":");
    if (currentStart < 0)
      return fallback;
    String needle = String('"') + key + "\":";
    int start = body.indexOf(needle, currentStart);
    if (start < 0)
      return fallback;
    start += needle.length();
    return body.substring(start).toFloat();
  }

  float jsonNumberFrom(const String &body, const char *key, int from, float fallback)
  {
    String needle = String('"') + key + "\":";
    int start = body.indexOf(needle, from);
    if (start < 0)
      return fallback;
    start += needle.length();
    return body.substring(start).toFloat();
  }

  String jsonStringFrom(const String &body, const char *key, int from, int until)
  {
    String needle = String('"') + key + "\":\"";
    int start = body.indexOf(needle, from);
    if (start < 0 || start >= until)
      return String();
    start += needle.length();
    String value;
    bool escaped = false;
    for (int i = start; i < until; ++i)
    {
      const char c = body[i];
      if (!escaped && c == '"')
        break;
      if (!escaped && c == '\\')
      {
        escaped = true;
        continue;
      }
      value += c;
      escaped = false;
    }
    return value;
  }

  bool coordinatesValid(double lat, double lon)
  {
    return isfinite(lat) && isfinite(lon) && lat >= -90.0 && lat <= 90.0 &&
           lon >= -180.0 && lon <= 180.0;
  }

  String urlEncode(const String &value)
  {
    static const char hex[] = "0123456789ABCDEF";
    String out;
    out.reserve(value.length() * 2);
    for (size_t i = 0; i < value.length(); ++i)
    {
      const uint8_t c = static_cast<uint8_t>(value[i]);
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')
        out += static_cast<char>(c);
      else if (c == ' ')
        out += "%20";
      else
      {
        out += '%';
        out += hex[c >> 4];
        out += hex[c & 0x0F];
      }
    }
    return out;
  }

  String weatherSummary(int code)
  {
    if (code == 0)
      return F("Clear");
    if (code <= 3)
      return F("Cloudy");
    if (code == 45 || code == 48)
      return F("Foggy");
    if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82))
      return F("Rain");
    if ((code >= 71 && code <= 77) || code == 85 || code == 86)
      return F("Snow");
    if (code >= 95)
      return F("Storm");
    return F("Weather");
  }

  String localTimeString()
  {
    const time_t utcNow = time(nullptr);
    if (utcNow < 100000)
      return String();
    const time_t localNow = utcNow + utcOffsetSeconds;
    struct tm parts{};
    gmtime_r(&localNow, &parts);
    char clockText[6];
    snprintf(clockText, sizeof(clockText), "%02d:%02d", parts.tm_hour, parts.tm_min);
    return String(clockText);
  }

  int localHour()
  {
    const time_t utcNow = time(nullptr);
    if (utcNow < 100000)
      return -1;
    const time_t localNow = utcNow + utcOffsetSeconds;
    struct tm parts{};
    gmtime_r(&localNow, &parts);
    return parts.tm_hour;
  }

  const __FlashStringHelper *timeGreeting(int hour)
  {
    if (hour >= 5 && hour < 12)
      return F("Good morning!");
    if (hour >= 12 && hour < 17)
      return F("Good afternoon!");
    if (hour >= 17 && hour < 21)
      return F("Good evening!");
    return F("Good night!");
  }

  void loadCreds()
  {
    prefs.begin("deskbuddy", true);
    staSsid = prefs.getString("ssid", WIFI_SSID);
    staPass = prefs.getString("pass", WIFI_PASSWORD);
    locCity = prefs.getString("city", LOCATION_CITY);
    locCountry = prefs.getString("country", LOCATION_COUNTRY);
    locLat = prefs.getDouble("lat", LOCATION_LATITUDE);
    locLon = prefs.getDouble("lon", LOCATION_LONGITUDE);
    locResolved = prefs.getBool("locok", true) && coordinatesValid(locLat, locLon);
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

  void saveEmoteMode(bool useRandom)
  {
    randomEmotes = useRandom;
    nextRandomEmoteAt = millis() + RANDOM_EMOTE_INTERVAL_MS;
    randomEmoteEndsAt = 0;
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
    nextLocationAttempt = 0;
  }

  void saveResolvedLocation()
  {
    prefs.begin("deskbuddy", false);
    prefs.putString("city", locCity);
    prefs.putString("country", locCountry);
    prefs.putDouble("lat", locLat);
    prefs.putDouble("lon", locLon);
    prefs.putBool("locok", true);
    prefs.end();
  }

  // Resolve city/country -> coordinates via Open-Meteo geocoding.
  bool resolveLocation()
  {
    if (!locCity.length() || !locCountry.length() || WiFi.status() != WL_CONNECTED)
      return false;
    locStatus = String(F("Locating ")) + locCity + F("...");
    Serial.println(locStatus);

    SecureHttp secure;
    HTTPClient http;
    String url = F("https://geocoding-api.open-meteo.com/v1/search?name=");
    url += urlEncode(locCity);
    url += F("&count=10&language=en&format=json");
    if (!http.begin(secure.ref(), url))
    {
      locStatus = F("Location service unavailable; retrying");
      return false;
    }
    platformHttpTimeouts(http);
    const int status = http.GET();
    if (status != HTTP_CODE_OK)
    {
      locStatus = String(F("Location request failed (")) + status + F("); retrying");
      http.end();
      return false;
    }
    const String body = http.getString();
    http.end();

    int position = body.indexOf(F("\"results\":"));
    while (position >= 0)
    {
      const int objectStart = body.indexOf('{', position);
      const int objectEnd = body.indexOf('}', objectStart);
      if (objectStart < 0 || objectEnd < 0)
        break;
      const String resultCountry = jsonStringFrom(body, "country", objectStart, objectEnd);
      const String countryCode = jsonStringFrom(body, "country_code", objectStart, objectEnd);
      if (resultCountry.equalsIgnoreCase(locCountry) || countryCode.equalsIgnoreCase(locCountry))
      {
        const double lat = jsonNumberFrom(body, "latitude", objectStart, 999);
        const double lon = jsonNumberFrom(body, "longitude", objectStart, 999);
        if (coordinatesValid(lat, lon))
        {
          const String resultCity = jsonStringFrom(body, "name", objectStart, objectEnd);
          locLat = lat;
          locLon = lon;
          if (resultCity.length())
            locCity = resultCity;
          if (resultCountry.length())
            locCountry = resultCountry;
          locResolved = true;
          locStatus = locCity + ", " + locCountry;
          saveResolvedLocation();
          Serial.printf("Location: %s (%.4f, %.4f)\n", locStatus.c_str(), lat, lon);
          nextWeatherAttempt = 0;
          return true;
        }
      }
      position = objectEnd + 1;
    }
    locStatus = F("City/country not found; check spelling");
    return false;
  }

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
    WiFi.begin(staSsid.c_str(), staPass.c_str());
    Serial.printf("Connecting to %s...\n", staSsid.c_str());
  }

  String htmlEscape(const String &in)
  {
    String out;
    out.reserve(in.length());
    for (char c : in)
    {
      if (c == '&')
        out += "&amp;";
      else if (c == '<')
        out += "&lt;";
      else if (c == '>')
        out += "&gt;";
      else if (c == '"')
        out += "&quot;";
      else if (c == '\'')
        out += "&#39;"; // values sit in single-quoted attributes; unescaped ' breaks the form
      else
        out += c;
    }
    return out;
  }

  void handleRoot()
  {
    const bool online = WiFi.status() == WL_CONNECTED;
    String page = F("<!doctype html><html><head><meta charset=utf-8>"
                    "<meta name=viewport content='width=device-width,initial-scale=1'>"
                    "<title>DeskBuddy</title><style>"
                    "body{font:15px system-ui;margin:0;background:#10131c;color:#f7f8fc}"
                    "main{max-width:460px;margin:auto;padding:24px 18px}"
                    "h1{font-size:22px;margin:0 0 4px}.s{color:#a9b0c3;margin:0 0 18px}"
                    "label{display:block;color:#a9b0c3;font-size:13px;margin:14px 0 6px}"
                    "input{width:100%;box-sizing:border-box;padding:11px;border-radius:10px;"
                    "border:1px solid #424b67;background:#111522;color:#f7f8fc;font:inherit}"
                    "button{font:inherit;cursor:pointer}.save{width:100%;box-sizing:border-box;padding:11px;"
                    "border-radius:10px;margin-top:18px;background:#67e8c2;color:#08120f;font-weight:700;border:0}"
                    ".card{background:#1b2030;border:1px solid #343b53;border-radius:16px;padding:20px}"
                    ".emotions{margin-top:14px}.emotions h2{font-size:17px;margin:0 0 4px}"
                    ".mode{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin:14px 0}"
                    ".mode label{margin:0;padding:10px;border:1px solid #424b67;border-radius:10px;"
                    "background:#111522;color:#f7f8fc;text-align:center;cursor:pointer}"
                    ".mode input{width:auto;margin-right:6px}.emotes{display:grid;"
                    "grid-template-columns:repeat(2,minmax(0,1fr));gap:9px}"
                    ".emote{min-height:76px;border:1px solid #424b67;border-radius:12px;"
                    "background:#111522;color:#f7f8fc;padding:9px 5px}"
                    ".emote.sel{border-color:#67e8c2;box-shadow:0 0 0 1px #67e8c2 inset}"
                    ".face{display:block;color:#67e8c2;font:700 18px monospace;margin-bottom:5px}"
                    ".hint{color:#a9b0c3;font-size:12px;margin:0 0 12px}"
                    ".st{padding:10px 12px;background:#121724;border-radius:10px;margin-bottom:14px;color:#a9b0c3}"
                    "</style></head><body><main>"
                    "<h1>DeskBuddy</h1><p class=s>Status &amp; settings</p><div class=card>");
    page += F("<div class=st>Status: ");
    if (online)
    {
      page += F("Online \xC2\xB7 ");
      page += WiFi.localIP().toString();
    }
    else
    {
      page += F("Offline, trying to connect\xE2\x80\xA6");
    }
    if (weather.valid)
    {
      page += F(" \xC2\xB7 ");
      page += String(weather.temperature, 1);
      page += F("C ");
      page += weather.summary;
    }
    page += F(" \xC2\xB7 ");
    page += htmlEscape(locStatus);
    page += F("</div><form method=POST action=/save>"
              "<label>Wi-Fi network</label>"
              "<input name=ssid maxlength=32 required value='");
    page += htmlEscape(staSsid);
    page += F("'><label>Wi-Fi password</label>"
              "<input name=pass type=password maxlength=63 placeholder='Leave blank to keep current'>"
              "<label>City</label>"
              "<input name=city maxlength=32 required value='");
    page += htmlEscape(locCity);
    page += F("'><label>Country (name or code)</label>"
              "<input name=country maxlength=32 required value='");
    page += htmlEscape(locCountry);
    page += F("'><label>Your name</label>"
              "<input name=name maxlength=20 placeholder='Shown as \"Hi name!\"' value='");
    page += htmlEscape(userName);
    page += F("'><button class=save type=submit>Save &amp; connect</button></form></div>"
              "<div class='card emotions'><h2>Emotes</h2>"
              "<p class=hint>Static holds the selected face. Random plays an emote for 10 seconds every 10 minutes.</p>"
              "<form method=POST action=/emote><div class=mode>"
              "<label><input type=radio name=mode value=static");
    if (!randomEmotes)
      page += F(" checked");
    page += F(">Static</label><label><input type=radio name=mode value=random");
    if (randomEmotes)
      page += F(" checked");
    page += F(">Random</label></div><div class=emotes>");
    for (uint8_t i = 0; i < MOOD_COUNT; ++i)
    {
      page += F("<button class='emote");
      if (i == moodSel)
        page += F(" sel");
      page += F("' type=submit name=mood value='");
      page += MOOD_SLUG[i];
      page += F("'><span class=face>");
      page += htmlEscape(String(MOOD_ICON[i]));
      page += F("</span>");
      page += MOOD_LABEL[i];
      page += F("</button>");
    }
    page += F("</div><button class=save type=submit>Save behavior</button>"
              "</form></div></main></body></html>");
    server.send(200, F("text/html"), page);
  }

  uint8_t moodFromSlug(const String &slug)
  {
    for (uint8_t i = 0; i < MOOD_COUNT; ++i)
      if (slug.equalsIgnoreCase(MOOD_SLUG[i]))
        return i;
    return MOOD_COUNT;
  }

  void redirectHome()
  {
    server.sendHeader(F("Location"), F("/"), true);
    server.send(303, F("text/plain"), F("Updated"));
  }

  void handleEmote()
  {
    const String mode = server.arg("mode");
    if (mode != F("static") && mode != F("random"))
    {
      server.send(400, F("text/plain"), F("Choose Static or Random"));
      return;
    }

    const bool requestedRandom = mode == F("random");
    const bool modeChanged = requestedRandom != randomEmotes;
    const bool moodChosen = server.hasArg("mood");
    uint8_t requestedMood = moodSel;
    if (moodChosen)
    {
      requestedMood = moodFromSlug(server.arg("mood"));
      if (requestedMood >= MOOD_COUNT)
      {
        server.send(400, F("text/plain"), F("Unknown emote"));
        return;
      }
      saveMood(requestedMood);
    }
    else if (modeChanged && !requestedRandom)
    {
      saveMood(moodSel);
    }

    if (modeChanged)
      saveEmoteMode(requestedRandom);

    if (requestedRandom)
    {
      nextRandomEmoteAt = millis() + RANDOM_EMOTE_INTERVAL_MS;
      if (moodChosen && requestedMood != MOOD_AUTO)
      {
        randomEmoteEndsAt = millis() + RANDOM_EMOTE_DURATION_MS;
        lastRandomMood = requestedMood;
      }
      else
      {
        moodSel = MOOD_AUTO;
        randomEmoteEndsAt = 0;
      }
    }
    redirectHome();
  }

  void handleSave()
  {
    String ssid = server.arg("ssid");
    String rawPass = server.arg("pass");
    String city = server.arg("city");
    String country = server.arg("country");
    String uname = server.arg("name");
    ssid.trim();
    city.trim();
    country.trim();
    uname.trim();
    if (!ssid.length() || ssid.length() > 32 || rawPass.length() > 63 ||
        !city.length() || city.length() > 32 || !country.length() || country.length() > 32 ||
        uname.length() > 20)
    {
      server.send(400, F("text/plain"), F("Invalid input"));
      return;
    }

    // Apply only the fields that actually changed.
    const bool ssidChanged = ssid != staSsid;
    const bool passChanged = rawPass.length() && rawPass != staPass;
    const bool wifiChanged = ssidChanged || passChanged;
    const bool locChanged = !city.equalsIgnoreCase(locCity) ||
                            !country.equalsIgnoreCase(locCountry);
    const bool nameChanged = uname != userName;

    if (wifiChanged)
      saveCreds(ssid, passChanged ? rawPass : staPass);
    if (nameChanged)
      saveName(uname);
    if (locChanged)
      saveLocationInput(city, country);

    String msg = F("Saved");
    if (wifiChanged)
      msg += F(" · reconnecting Wi-Fi");
    else if (locChanged)
      msg += F(" · updating location");
    server.send(200, F("text/html"),
                String(F("<!doctype html><meta charset=utf-8><meta name=viewport "
                         "content='width=device-width,initial-scale=1'>"
                         "<body style='font:15px system-ui;background:#10131c;color:#f7f8fc;padding:24px'>")) +
                    msg + F(". <a style='color:#67e8c2' href='/'>Back</a>"));

    if (wifiChanged)
    {
      delay(150);
      beginWifi(); // only reconnect when Wi-Fi credentials changed
    }
  }

  void startWebServer()
  {
    if (webStarted)
      return;
    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/emote", HTTP_POST, handleEmote);
    server.onNotFound(handleRoot); // serves dashboard + captive-portal catch-all
    server.begin();
    webStarted = true;
  }

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
    startWebServer();
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

  void updateWeather()
  {
    if (WiFi.status() != WL_CONNECTED || !locResolved)
    {
      nextWeatherAttempt = millis() + WEATHER_RETRY_MS;
      return;
    }

    SecureHttp secure;
    HTTPClient http;
    String url = F("https://api.open-meteo.com/v1/forecast?latitude=");
    url += String(locLat, 6);
    url += F("&longitude=");
    url += String(locLon, 6);
    url += F("&current=temperature_2m,weather_code,is_day&timezone=auto&forecast_days=1");

    Serial.printf("Updating weather... (heap=%u)\n", ESP.getFreeHeap());
    if (!http.begin(secure.ref(), url))
    {
      nextWeatherAttempt = millis() + WEATHER_RETRY_MS;
      return;
    }
    platformHttpTimeouts(http);
    const int status = http.GET();
    if (status == HTTP_CODE_OK)
    {
      const String body = http.getString();
      utcOffsetSeconds = static_cast<int32_t>(jsonNumberFrom(body, "utc_offset_seconds", 0, utcOffsetSeconds));
      weather.temperature = jsonNumber(body, "temperature_2m", 0);
      weather.code = static_cast<int>(jsonNumber(body, "weather_code", -1));
      weather.isDay = jsonNumber(body, "is_day", 1) > 0.5f;
      weather.summary = weatherSummary(weather.code);
      weather.valid = weather.code >= 0;
      weather.updatedAt = millis();
      nextWeatherAttempt = millis() + WEATHER_INTERVAL_MS;
      Serial.printf("Weather: %.1f C, %s\n", weather.temperature, weather.summary.c_str());
    }
    else
    {
      Serial.printf("Weather request failed: %d\n", status);
      nextWeatherAttempt = millis() + WEATHER_RETRY_MS;
    }
    http.end();
  }

  bool isRainCode(int code) { return (code >= 51 && code <= 67) || (code >= 80 && code <= 82); }
  bool isSnowCode(int code) { return (code >= 71 && code <= 77) || code == 85 || code == 86; }
  bool isStormCode(int code) { return code >= 95; }
  bool isFogCode(int code) { return code == 45 || code == 48; }

  void drawWeatherIcon(int x, int y, int code)
  {
    if (code == 0)
    {
      display.drawCircle(x + 6, y + 6, 4, SSD1306_WHITE);
      for (int a = 0; a < 8; ++a)
      {
        float angle = a * PI / 4.0f;
        display.drawLine(x + 6 + cos(angle) * 6, y + 6 + sin(angle) * 6,
                         x + 6 + cos(angle) * 8, y + 6 + sin(angle) * 8, SSD1306_WHITE);
      }
    }
    else if (isRainCode(code) || isStormCode(code))
    {
      display.fillCircle(x + 5, y + 5, 4, SSD1306_WHITE);
      display.fillCircle(x + 10, y + 6, 5, SSD1306_WHITE);
      display.fillRect(x + 3, y + 6, 12, 4, SSD1306_WHITE);
      display.drawLine(x + 5, y + 12, x + 4, y + 15, SSD1306_WHITE);
      display.drawLine(x + 11, y + 12, x + 10, y + 15, SSD1306_WHITE);
    }
    else
    {
      display.fillCircle(x + 5, y + 7, 4, SSD1306_WHITE);
      display.fillCircle(x + 10, y + 6, 5, SSD1306_WHITE);
      display.fillRect(x + 3, y + 7, 13, 4, SSD1306_WHITE);
    }
  }

  void drawWeatherEffects(bool night)
  {
    const int fall = (animationFrame * 2) % 30;
    if (night)
    {
      display.fillCircle(112, 28, 7, SSD1306_WHITE);
      display.fillCircle(115, 25, 7, SSD1306_BLACK);
      display.drawPixel(16, 25, SSD1306_WHITE);
      display.drawPixel(109, 49, SSD1306_WHITE);
      if ((animationFrame / 5) % 2)
        display.drawPixel(18, 27, SSD1306_WHITE);
      const int z = (animationFrame / 8) % 3;
      display.drawLine(105 + z, 44 - z, 110 + z, 44 - z, SSD1306_WHITE);
      display.drawLine(110 + z, 44 - z, 105 + z, 49 - z, SSD1306_WHITE);
      display.drawLine(105 + z, 49 - z, 110 + z, 49 - z, SSD1306_WHITE);
    }
    if (!weather.valid)
      return;
    if (isRainCode(weather.code))
    {
      for (int i = 0; i < 4; ++i)
      {
        const int x = i < 2 ? 8 + i * 10 : 106 + (i - 2) * 10;
        const int y = 22 + (fall + i * 9) % 35;
        display.drawLine(x, y, x - 2, y + 4, SSD1306_WHITE);
      }
    }
    else if (isSnowCode(weather.code))
    {
      for (int i = 0; i < 4; ++i)
      {
        const int x = i < 2 ? 10 + i * 10 : 107 + (i - 2) * 10;
        const int y = 23 + (fall / 2 + i * 11) % 34;
        display.drawPixel(x, y, SSD1306_WHITE);
        display.drawPixel(x - 1, y, SSD1306_WHITE);
        display.drawPixel(x + 1, y, SSD1306_WHITE);
        display.drawPixel(x, y - 1, SSD1306_WHITE);
        display.drawPixel(x, y + 1, SSD1306_WHITE);
      }
    }
    else if (isStormCode(weather.code))
    {
      const int flash = animationFrame % 24 < 5 ? 2 : 0;
      display.drawLine(11, 23, 6 + flash, 36, SSD1306_WHITE);
      display.drawLine(6 + flash, 36, 12, 35, SSD1306_WHITE);
      display.drawLine(12, 35, 7 + flash, 50, SSD1306_WHITE);
      display.drawLine(115, 24, 109, 37, SSD1306_WHITE);
      display.drawLine(109, 37, 116, 35, SSD1306_WHITE);
      display.drawLine(116, 35, 111, 49, SSD1306_WHITE);
    }
    else if (isFogCode(weather.code))
    {
      const int drift = (animationFrame / 3) % 6;
      display.drawLine(3 + drift, 27, 24 + drift, 27, SSD1306_WHITE);
      display.drawLine(103 - drift, 35, 124 - drift, 35, SSD1306_WHITE);
      display.drawLine(5, 49, 25, 49, SSD1306_WHITE);
    }
    else if (weather.code >= 1 && weather.code <= 3)
    {
      const int drift = (animationFrame / 8) % 7;
      display.fillCircle(8 + drift, 29, 4, SSD1306_WHITE);
      display.fillCircle(14 + drift, 27, 6, SSD1306_WHITE);
      display.fillRect(5 + drift, 29, 15, 4, SSD1306_WHITE);
    }
    else if (!night)
    {
      const int pulse = (animationFrame / 5) % 2;
      display.drawLine(10, 26 - pulse, 10, 21 - pulse, SSD1306_WHITE);
      display.drawLine(6, 23 - pulse, 3, 20 - pulse, SSD1306_WHITE);
      display.drawLine(14, 23 - pulse, 17, 20 - pulse, SSD1306_WHITE);
    }
  }

  float easeLiquid(float value)
  {
    if (value <= 0.0f)
      return 0.0f;
    if (value >= 1.0f)
      return 1.0f;
    return value * value * (3.0f - 2.0f * value);
  }

  float liquidEnvelope(uint32_t beat, uint32_t riseEnd, uint32_t holdEnd,
                       uint32_t fallEnd)
  {
    if (beat < riseEnd)
      return easeLiquid(static_cast<float>(beat) / riseEnd);
    if (beat < holdEnd)
      return 1.0f;
    if (beat < fallEnd)
    {
      return 1.0f - easeLiquid(static_cast<float>(beat - holdEnd) /
                               static_cast<float>(fallEnd - holdEnd));
    }
    return 0.0f;
  }

  void drawLiquidEye(int centerX, int centerY, int width, int height)
  {
    if (width < 4)
      width = 4;
    if (height < 2)
      height = 2;
    const int radius = min(width, height) / 2;
    display.fillRoundRect(centerX - width / 2, centerY - height / 2,
                          width, height, radius, SSD1306_WHITE);
  }

  void drawHappyEyes(int y, int xOffset = 0)
  {
    display.drawLine(35 + xOffset, y + 3, 44 + xOffset, y - 3, SSD1306_WHITE);
    display.drawLine(44 + xOffset, y - 3, 53 + xOffset, y + 3, SSD1306_WHITE);
    display.drawLine(75 + xOffset, y + 3, 84 + xOffset, y - 3, SSD1306_WHITE);
    display.drawLine(84 + xOffset, y - 3, 93 + xOffset, y + 3, SSD1306_WHITE);
  }

  void drawCenteredText(const __FlashStringHelper *text, int y)
  {
    const String line(text);
    const int textW = static_cast<int>(line.length()) * 6;
    display.setCursor(max(0, (static_cast<int>(OLED_WIDTH) - textW) / 2), y);
    display.print(line);
  }

  void drawBootSmile(int xOffset = 0, int yOffset = 0)
  {
    display.drawLine(53 + xOffset, 47 + yOffset, 59 + xOffset, 53 + yOffset, SSD1306_WHITE);
    display.drawLine(59 + xOffset, 53 + yOffset, 69 + xOffset, 53 + yOffset, SSD1306_WHITE);
    display.drawLine(69 + xOffset, 53 + yOffset, 75 + xOffset, 47 + yOffset, SSD1306_WHITE);
  }

  int doubleBlinkHeight(uint32_t elapsed)
  {
    uint32_t phase = elapsed;
    if (phase >= 500)
      phase -= 500; // second blink after a short, expressive pause
    if (phase < 170)
      return 16 - static_cast<int>(14.0f * easeLiquid(phase / 170.0f));
    if (phase < 350)
      return 2 + static_cast<int>(14.0f * easeLiquid((phase - 170) / 180.0f));
    return 16;
  }

  // Face-only wake-up choreography inspired by the smooth transitions and
  // directional gaze principles of the open-source FluxGarage RoboEyes project.
  // This original state machine stays non-blocking so networking keeps running.
  bool renderBootAnimation()
  {
    if (bootStage == BOOT_COMPLETE)
      return false;

    const int hour = localHour();
    if (bootStage == BOOT_WAITING && weather.valid && hour >= 0)
    {
      bootStage = BOOT_OPENING;
      bootStageStartedAt = millis();
    }

    uint32_t elapsed = millis() - bootStageStartedAt;
    const uint32_t duration = bootStage == BOOT_OPENING      ? 1250UL
                              : bootStage == BOOT_LOOK_LEFT  ? 750UL
                              : bootStage == BOOT_LOOK_RIGHT ? 1050UL
                              : bootStage == BOOT_DOUBLE_BLINK ? 1050UL
                              : bootStage == BOOT_EXCITED    ? 1650UL
                              : bootStage == BOOT_GREETING   ? 2200UL
                              : bootStage == BOOT_INTRO      ? 2200UL
                                                             : 0UL;
    if (duration && elapsed >= duration)
    {
      bootStage = static_cast<BootStage>(static_cast<uint8_t>(bootStage) + 1);
      bootStageStartedAt = millis();
      elapsed = 0;
      if (bootStage == BOOT_COMPLETE)
        return false;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    if (bootStage == BOOT_WAITING)
    {
      // Calm closed eyes while local time is being synchronized.
      const int breathe = static_cast<int>(sinf(millis() * 0.004f));
      display.drawLine(31, 32 + breathe, 43, 36 + breathe, SSD1306_WHITE);
      display.drawLine(43, 36 + breathe, 55, 32 + breathe, SSD1306_WHITE);
      display.drawLine(73, 32 + breathe, 85, 36 + breathe, SSD1306_WHITE);
      display.drawLine(85, 36 + breathe, 97, 32 + breathe, SSD1306_WHITE);
      display.drawLine(58, 50 + breathe, 70, 50 + breathe, SSD1306_WHITE);
    }
    else if (bootStage == BOOT_OPENING)
    {
      const float progress = easeLiquid(min(1.0f, elapsed / 1100.0f));
      const int eyeH = 2 + static_cast<int>(14.0f * progress);
      const int eyeY = 34 - static_cast<int>(2.0f * progress);
      drawLiquidEye(43, eyeY, 23, eyeH);
      drawLiquidEye(85, eyeY, 23, eyeH);
      display.drawLine(58, 50, 70, 50, SSD1306_WHITE);
    }
    else if (bootStage == BOOT_LOOK_LEFT)
    {
      const float progress = easeLiquid(min(1.0f, elapsed / 600.0f));
      const int gaze = -static_cast<int>(9.0f * progress);
      drawLiquidEye(43 + gaze, 32, 23, 16 + static_cast<int>(3.0f * progress));
      drawLiquidEye(85 + gaze, 32, 23, 16);
      display.drawRoundRect(61, 48, 7, 7, 3, SSD1306_WHITE);
    }
    else if (bootStage == BOOT_LOOK_RIGHT)
    {
      const float progress = easeLiquid(min(1.0f, elapsed / 850.0f));
      const int gaze = -9 + static_cast<int>(18.0f * progress);
      drawLiquidEye(43 + gaze, 32, 23, 16);
      drawLiquidEye(85 + gaze, 32, 23, 16 + static_cast<int>(3.0f * progress));
      display.drawRoundRect(61, 48, 7, 7, 3, SSD1306_WHITE);
    }
    else if (bootStage == BOOT_DOUBLE_BLINK)
    {
      const int eyeH = doubleBlinkHeight(elapsed);
      drawLiquidEye(43, 32, 23, eyeH);
      drawLiquidEye(85, 32, 23, eyeH);
      drawBootSmile();
    }
    else if (bootStage == BOOT_EXCITED)
    {
      const float progress = min(1.0f, elapsed / 1450.0f);
      const float pop = fabsf(sinf(progress * 2.0f * PI));
      const int bounce = static_cast<int>(5.0f * pop);
      const int eyeW = 23 + static_cast<int>(5.0f * pop);
      const int eyeH = 16 + static_cast<int>(4.0f * pop);
      drawLiquidEye(43, 33 - bounce, eyeW, eyeH);
      drawLiquidEye(85, 33 - bounce, eyeW, eyeH);
      drawBootSmile(0, -bounce);
      const int sparkle = 2 + static_cast<int>(2.0f * pop);
      display.drawLine(15, 21 - sparkle, 15, 21 + sparkle, SSD1306_WHITE);
      display.drawLine(15 - sparkle, 21, 15 + sparkle, 21, SSD1306_WHITE);
      display.drawLine(113, 24 - sparkle, 113, 24 + sparkle, SSD1306_WHITE);
      display.drawLine(113 - sparkle, 24, 113 + sparkle, 24, SSD1306_WHITE);
    }
    else if (bootStage == BOOT_GREETING)
    {
      const int bounce = static_cast<int>(2.0f * sinf(elapsed * 0.009f));
      drawHappyEyes(33 + bounce);
      drawBootSmile(0, bounce);
      drawCenteredText(timeGreeting(hour), 5);
    }
    else if (bootStage == BOOT_INTRO)
    {
      const int sway = static_cast<int>(2.0f * sinf(elapsed * 0.008f));
      drawLiquidEye(43 + sway, 32, 23, 16);
      drawLiquidEye(85 + sway, 32, 23, 16);
      drawBootSmile(sway);
      drawCenteredText(F("Hi! I am Teevee"), 5);
    }

    display.display();
    return true;
  }

  void drawFace()
  {
    const bool night = weather.valid && !weather.isDay;
    const bool rain = weather.valid && isRainCode(weather.code);
    const bool snow = weather.valid && isSnowCode(weather.code);
    const bool storm = weather.valid && isStormCode(weather.code);
    const bool fog = weather.valid && isFogCode(weather.code);
    const uint16_t blinkPhase = animationFrame % 82;
    int eyeHeight = 12;
    if (blinkPhase == 76 || blinkPhase == 79)
      eyeHeight = 7;
    else if (blinkPhase >= 77 && blinkPhase <= 78)
      eyeHeight = 2;
    const int drift = static_cast<int>(2.0f * sinf(animationFrame * 0.035f));

    drawWeatherEffects(night);
    if ((night && !storm) || fog)
    {
      display.drawLine(35 + drift, 35, 44 + drift, 38, SSD1306_WHITE);
      display.drawLine(44 + drift, 38, 53 + drift, 35, SSD1306_WHITE);
      display.drawLine(75 + drift, 35, 84 + drift, 38, SSD1306_WHITE);
      display.drawLine(84 + drift, 38, 93 + drift, 35, SSD1306_WHITE);
    }
    else if (snow)
    {
      drawHappyEyes(35, drift);
    }
    else
    {
      drawLiquidEye(44 + drift, 35, 20, storm ? 15 : eyeHeight);
      drawLiquidEye(84 + drift, 35, 20, storm ? 15 : eyeHeight);
    }

    if (storm)
    {
      display.drawRoundRect(60 + drift, 47, 8, 7, 3, SSD1306_WHITE);
    }
    else if (rain)
    {
      display.drawLine(56 + drift, 54, 60 + drift, 50, SSD1306_WHITE);
      display.drawLine(60 + drift, 50, 68 + drift, 50, SSD1306_WHITE);
      display.drawLine(68 + drift, 50, 72 + drift, 54, SSD1306_WHITE);
    }
    else if (night || fog)
    {
      display.drawLine(57 + drift, 51, 71 + drift, 51, SSD1306_WHITE);
    }
    else
    {
      display.drawLine(54 + drift, 48, 59 + drift, 53, SSD1306_WHITE);
      display.drawLine(59 + drift, 53, 69 + drift, 53, SSD1306_WHITE);
      display.drawLine(69 + drift, 53, 74 + drift, 48, SSD1306_WHITE);
    }
  }

  void drawMoodFace()
  {
    // Liquid-eye personality style (matches the 8266). The selected mood stays
    // fixed; only its own animation loops via millis().
    const uint32_t elapsed = millis();
    int leftX = 44, rightX = 84, leftY = 28, rightY = 28;
    int leftW = 22, rightW = 22, leftH = 13, rightH = 13;
    int mouthX = 64, mouthY = 45;
    float action = 0.0f;
    bool happyEyes = false;
    bool sneezeSnap = false;
    bool frown = false;
    bool roundMouth = false;
    bool flatMouth = false;
    bool laughMouth = false;

    switch (moodSel)
    {
    case MOOD_HAPPY:
    {
      const float beat = static_cast<float>(elapsed % 2600) / 2600.0f;
      const int lift = static_cast<int>(5.0f * sinf(beat * PI));
      leftY -= lift;
      rightY -= lift;
      mouthY -= lift;
      break;
    }
    case MOOD_EXCITED:
    {
      const float beat = static_cast<float>(elapsed % 1500) / 1500.0f;
      const int lift = static_cast<int>(7.0f * fabsf(sinf(beat * PI)));
      leftY -= lift;
      rightY -= lift;
      mouthY -= lift;
      leftH = rightH = 17;
      break;
    }
    case MOOD_STRETCH:
    {
      action = liquidEnvelope(elapsed % 4300, 1050, 2850, 3950);
      leftY = rightY = 28 - static_cast<int>(6 * action);
      leftW = rightW = 22 - static_cast<int>(5 * action);
      leftH = rightH = 13 + static_cast<int>(7 * action);
      mouthY -= static_cast<int>(3 * action);
      happyEyes = action > 0.72f;
      break;
    }
    case MOOD_YAWN:
    {
      action = liquidEnvelope(elapsed % 4200, 900, 2700, 3700);
      leftH = rightH = 13 - static_cast<int>(10 * action);
      leftY = rightY = 28 + static_cast<int>(3 * action);
      mouthY += static_cast<int>(2 * action);
      roundMouth = true;
      break;
    }
    case MOOD_SNEEZE:
    {
      const uint32_t beat = elapsed % 3400;
      if (beat < 1350)
      {
        action = easeLiquid(static_cast<float>(beat) / 1350.0f);
        leftH = rightH = 13 - static_cast<int>(8 * action);
        leftX -= static_cast<int>(3 * action);
        rightX -= static_cast<int>(3 * action);
        mouthX -= static_cast<int>(2 * action);
        roundMouth = true;
      }
      else if (beat < 1680)
      {
        sneezeSnap = true;
        leftX += 5;
        rightX += 5;
        mouthX += 5;
        leftY += 3;
        rightY += 3;
        mouthY += 3;
      }
      else
      {
        action = liquidEnvelope(beat - 1680, 1, 1, 900);
        leftX += static_cast<int>(3 * action);
        rightX += static_cast<int>(3 * action);
        mouthX += static_cast<int>(3 * action);
        roundMouth = true;
      }
      break;
    }
    case MOOD_CURIOUS:
    {
      action = liquidEnvelope(elapsed % 4400, 900, 3000, 4000);
      leftX += static_cast<int>(6 * action);
      rightX += static_cast<int>(6 * action);
      mouthX += static_cast<int>(6 * action);
      leftY -= static_cast<int>(2 * action);
      rightY += static_cast<int>(2 * action);
      leftH += static_cast<int>(5 * action);
      rightH -= static_cast<int>(5 * action);
      roundMouth = true;
      break;
    }
    case MOOD_GLANCE_LEFT:
    case MOOD_GLANCE_RIGHT:
    {
      action = liquidEnvelope(elapsed % 3600, 700, 2500, 3350);
      const int direction = moodSel == MOOD_GLANCE_LEFT ? -1 : 1;
      const int shift = direction * static_cast<int>(9 * action);
      leftX += shift;
      rightX += shift;
      mouthX += shift / 2;
      if (direction < 0)
        leftH += static_cast<int>(4 * action);
      else
        rightH += static_cast<int>(4 * action);
      break;
    }
    case MOOD_WINK:
    {
      action = liquidEnvelope(elapsed % 3200, 180, 650, 900);
      leftH = 13 - static_cast<int>(11 * action);
      leftW = 22 + static_cast<int>(2 * action);
      break;
    }
    case MOOD_LAUGH:
    {
      const float beat = fabsf(sinf((elapsed % 1100) / 1100.0f * PI));
      const int lift = static_cast<int>(5.0f * beat);
      leftY -= lift;
      rightY -= lift;
      mouthY -= lift;
      happyEyes = true;
      laughMouth = true;
      action = beat;
      break;
    }
    case MOOD_SURPRISED:
    {
      const float pulse = sinf(elapsed * 0.004f) * 0.5f + 0.5f;
      leftW = rightW = 21 + static_cast<int>(4.0f * pulse);
      leftH = rightH = 16 + static_cast<int>(4.0f * pulse);
      leftY = rightY = 27 - static_cast<int>(2.0f * pulse);
      roundMouth = true;
      break;
    }
    case MOOD_NERVOUS:
    {
      const int jitter = static_cast<int>(2.0f * sinf(elapsed * 0.035f));
      leftX += jitter;
      rightX += jitter;
      mouthX += jitter;
      leftH = 12;
      rightH = 9;
      flatMouth = true;
      break;
    }
    case MOOD_SAD:
    {
      const int sink = static_cast<int>(2.0f * (sinf(elapsed * 0.0018f) * 0.5f + 0.5f));
      leftY = rightY = 30 + sink;
      leftH = rightH = 11;
      mouthY = 47;
      frown = true;
      break;
    }
    case MOOD_ANGRY:
    {
      const int shake = static_cast<int>(1.5f * sinf(elapsed * 0.02f));
      leftX += shake;
      rightX += shake;
      mouthX += shake;
      leftH = rightH = 8;
      frown = true;
      break;
    }
    case MOOD_CONFUSED:
    {
      const int sway = static_cast<int>(3.0f * sinf(elapsed * 0.0016f));
      leftX += sway;
      rightX += sway;
      mouthX += sway;
      leftH = 17;
      rightH = 9;
      flatMouth = true;
      break;
    }
    case MOOD_SLEEP:
    {
      const int breathe = static_cast<int>(2.0f * sinf(elapsed * 0.004f));
      leftY = rightY = 28 + breathe;
      mouthY = 45 + breathe;
      leftH = rightH = 3;
      flatMouth = true;
      break;
    }
    case MOOD_DND:
    {
      action = liquidEnvelope(elapsed % 4000, 800, 2800, 3900);
      leftH = rightH = 6;
      flatMouth = true;
      break;
    }
    default:
      break;
    }

    // Centre the face in the area below the header divider (captions removed).
    const int faceOffset = 8;
    leftY += faceOffset;
    rightY += faceOffset;
    mouthY += faceOffset;

    // Eyes.
    if (sneezeSnap)
    {
      display.drawLine(leftX - 10, leftY - 5, leftX + 10, leftY + 5, SSD1306_WHITE);
      display.drawLine(leftX + 10, leftY - 5, leftX - 10, leftY + 5, SSD1306_WHITE);
      display.drawLine(rightX - 10, rightY - 5, rightX + 10, rightY + 5, SSD1306_WHITE);
      display.drawLine(rightX + 10, rightY - 5, rightX - 10, rightY + 5, SSD1306_WHITE);
    }
    else if (happyEyes)
    {
      drawHappyEyes(leftY);
    }
    else
    {
      drawLiquidEye(leftX, leftY, leftW, leftH);
      drawLiquidEye(rightX, rightY, rightW, rightH);
      if (frown) // brows angled down toward the nose
      {
        display.drawLine(leftX - 11, leftY - 10, leftX + 8, leftY - 6, SSD1306_WHITE);
        display.drawLine(rightX - 8, rightY - 6, rightX + 11, rightY - 10, SSD1306_WHITE);
      }
      else if (moodSel == MOOD_SAD)
      {
        display.drawLine(leftX - 11, leftY - 6, leftX + 8, leftY - 10, SSD1306_WHITE);
        display.drawLine(rightX - 8, rightY - 10, rightX + 11, rightY - 6, SSD1306_WHITE);
      }
      else if (moodSel == MOOD_CONFUSED) // one quizzical raised brow
      {
        display.drawLine(rightX - 9, rightY - 12, rightX + 9, rightY - 14, SSD1306_WHITE);
      }
    }

    // Mouth.
    if (laughMouth)
    {
      const int mouthW = 10 + static_cast<int>(5 * action);
      const int mouthH = 5 + static_cast<int>(5 * action);
      display.drawRoundRect(mouthX - mouthW / 2, mouthY - mouthH / 2,
                            mouthW, mouthH, min(mouthW, mouthH) / 2, SSD1306_WHITE);
    }
    else if (moodSel == MOOD_YAWN)
    {
      const int mouthW = 7 + static_cast<int>(9 * action);
      const int mouthH = 4 + static_cast<int>(10 * action);
      display.drawRoundRect(mouthX - mouthW / 2, mouthY - mouthH / 2,
                            mouthW, mouthH, mouthW / 2, SSD1306_WHITE);
    }
    else if (sneezeSnap)
    {
      display.drawLine(mouthX - 9, mouthY + 2, mouthX - 4, mouthY - 2, SSD1306_WHITE);
      display.drawLine(mouthX - 4, mouthY - 2, mouthX + 1, mouthY + 2, SSD1306_WHITE);
      display.drawLine(mouthX + 1, mouthY + 2, mouthX + 7, mouthY - 2, SSD1306_WHITE);
    }
    else if (roundMouth && moodSel == MOOD_SNEEZE)
    {
      const int mouthW = 6 + static_cast<int>(7 * action);
      const int mouthH = 4 + static_cast<int>(7 * action);
      display.drawRoundRect(mouthX - mouthW / 2, mouthY - mouthH / 2,
                            mouthW, mouthH, mouthW / 2, SSD1306_WHITE);
    }
    else if (roundMouth) // curious: small surprised "o"
    {
      display.drawRoundRect(mouthX - 3, mouthY - 3, 7, 7, 3, SSD1306_WHITE);
    }
    else if (flatMouth)
    {
      display.drawLine(mouthX - 8, mouthY, mouthX + 8, mouthY, SSD1306_WHITE);
    }
    else if (frown)
    {
      display.drawLine(mouthX - 11, mouthY + 3, mouthX - 6, mouthY - 3, SSD1306_WHITE);
      display.drawLine(mouthX - 6, mouthY - 3, mouthX + 6, mouthY - 3, SSD1306_WHITE);
      display.drawLine(mouthX + 6, mouthY - 3, mouthX + 11, mouthY + 3, SSD1306_WHITE);
    }
    else // smile
    {
      display.drawLine(mouthX - 11, mouthY - 3, mouthX - 6, mouthY + 3, SSD1306_WHITE);
      display.drawLine(mouthX - 6, mouthY + 3, mouthX + 6, mouthY + 3, SSD1306_WHITE);
      display.drawLine(mouthX + 6, mouthY + 3, mouthX + 11, mouthY - 3, SSD1306_WHITE);
    }

    // Mood-specific extras.
    if (moodSel == MOOD_SAD)
    {
      const int ty1 = leftY + 6 + static_cast<int>(fmodf(elapsed * 0.02f, 24.0f));
      if (ty1 < 62)
        display.fillCircle(leftX - 6, ty1, 1, SSD1306_WHITE);
    }
    else if (moodSel == MOOD_SLEEP)
    {
      const int zy1 = 30 + faceOffset - static_cast<int>(fmodf(elapsed * 0.012f, 16.0f));
      display.setCursor(104, zy1);
      display.print('z');
      display.setCursor(110, zy1 - 5);
      display.print('Z');
    }
    else if (moodSel == MOOD_CONFUSED)
    {
      const int q = static_cast<int>(2.0f * sinf(elapsed * 0.004f));
      display.setCursor(104, 24 + faceOffset + q);
      display.print('?');
    }
    else if (moodSel == MOOD_DND)
    {
      const int r = 5 + static_cast<int>(2 * action);
      display.drawCircle(108, 26 + faceOffset, r, SSD1306_WHITE);
      display.drawLine(108 - r + 1, 26 + faceOffset + r - 1, 108 + r - 1, 26 + faceOffset - r + 1, SSD1306_WHITE);
    }
    else if (moodSel == MOOD_EXCITED)
    {
      const int sr = 2 + static_cast<int>((sinf(elapsed * 0.006f) * 0.5f + 0.5f) * 2.0f);
      display.drawLine(20, 26 + faceOffset - sr, 20, 26 + faceOffset + sr, SSD1306_WHITE);
      display.drawLine(20 - sr, 26 + faceOffset, 20 + sr, 26 + faceOffset, SSD1306_WHITE);
      display.drawLine(108, 28 + faceOffset - sr, 108, 28 + faceOffset + sr, SSD1306_WHITE);
      display.drawLine(108 - sr, 28 + faceOffset, 108 + sr, 28 + faceOffset, SSD1306_WHITE);
    }
    else if (moodSel == MOOD_NERVOUS)
    {
      const int drop = static_cast<int>(fmodf(elapsed * 0.018f, 22.0f));
      const int dropY = 24 + faceOffset + drop;
      if (dropY < 61)
      {
        display.drawLine(108, dropY, 106, dropY + 4, SSD1306_WHITE);
        display.drawLine(108, dropY, 110, dropY + 4, SSD1306_WHITE);
        display.fillCircle(108, dropY + 4, 2, SSD1306_WHITE);
      }
    }
  }

  // "Hi <name> !" animated in the top strip, replacing weather/clock.
  void drawHeaderGreeting()
  {
    const String hi = String(F("Hi ")) + userName + F(" !");
    display.setTextSize(1);
    const int textW = static_cast<int>(hi.length()) * 6;
    if (textW <= 122)
    {
      const int sway = static_cast<int>(3.0f * sinf(animationFrame * 0.2f));
      display.setCursor(max(0, (128 - textW) / 2) + sway, 5);
      display.print(hi);
    }
    else
    {
      // Too long to fit: scroll it across the strip as a marquee.
      const int span = textW + 128;
      const int x = 128 - static_cast<int>((animationFrame * 2) % span);
      display.setCursor(x, 5);
      display.print(hi);
    }
  }

  void updateRandomEmote()
  {
    if (!randomEmotes)
      return;

    const uint32_t now = millis();
    if (bootStage != BOOT_COMPLETE)
    {
      // Start the ten-minute cadence after the wake-up sequence is visible.
      moodSel = MOOD_AUTO;
      randomEmoteEndsAt = 0;
      nextRandomEmoteAt = now + RANDOM_EMOTE_INTERVAL_MS;
      return;
    }

    if (randomEmoteEndsAt && static_cast<int32_t>(now - randomEmoteEndsAt) >= 0)
    {
      moodSel = MOOD_AUTO;
      randomEmoteEndsAt = 0;
      animationFrame = 0;
      Serial.println(F("Random emote finished; returning to weather."));
    }

    if (!nextRandomEmoteAt)
    {
      nextRandomEmoteAt = now + RANDOM_EMOTE_INTERVAL_MS;
      return;
    }
    if (static_cast<int32_t>(now - nextRandomEmoteAt) < 0)
      return;

    uint8_t nextMood;
    do
    {
      nextMood = static_cast<uint8_t>(random(1, MOOD_COUNT));
    } while (MOOD_COUNT > 2 && nextMood == lastRandomMood);
    moodSel = nextMood; // Do not write periodic changes and wear out flash.
    lastRandomMood = nextMood;
    animationFrame = 0;
    randomEmoteEndsAt = now + RANDOM_EMOTE_DURATION_MS;
    nextRandomEmoteAt = now + RANDOM_EMOTE_INTERVAL_MS;
    Serial.printf("Random emote: %s\n", MOOD_LABEL[moodSel]);
  }

  void renderDisplay()
  {
    // Moods redraw faster (~22 fps) for fluid motion; weather stays ~11 fps.
    const uint32_t frameInterval = (moodSel == MOOD_AUTO) ? 90 : 45;
    if (!oledReady || millis() - lastAnimationFrame < frameInterval)
      return;
    lastAnimationFrame = millis();
    ++animationFrame;

    if (renderBootAnimation())
      return;

    const bool night = weather.valid && !weather.isDay;
    static int8_t previousNightMode = -1;
    if (previousNightMode != static_cast<int8_t>(night))
    {
      // Reduce contrast at night but keep it clearly visible (dim(true) would
      // drop contrast to 0 and blank the panel).
      display.ssd1306_command(SSD1306_SETCONTRAST);
      display.ssd1306_command(night ? 0x40 : 0xCF);
      previousNightMode = night;
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    const String clockText = localTimeString();

    // Animated loop: periodically greet the user in the top strip, replacing
    // the weather/clock while the face keeps animating below.
    const bool greet = userName.length() && (millis() % 14000UL) >= 10000UL;
    if (greet)
    {
      drawHeaderGreeting();
    }
    else if (weather.valid)
    {
      drawWeatherIcon(1, 1, weather.code);
      display.setCursor(20, 2);
      display.printf("%.1fC", weather.temperature);
      if (clockText.length())
      {
        display.setCursor(94, 2);
        display.print(clockText);
      }
    }
    else if (WiFi.status() == WL_CONNECTED)
    {
      display.setCursor(5, 3);
      display.print(F("Fetching weather..."));
    }
    else
    {
      display.setCursor(4, 3);
      display.print(F("Connecting WiFi..."));
    }
    display.drawLine(0, 17, 127, 17, SSD1306_WHITE);
    if (moodSel == MOOD_AUTO)
      drawFace();
    else
      drawMoodFace();
    display.display();
  }
} // namespace

void setup()
{
  Serial.begin(115200);
  delay(200);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH); // off (active LOW)

  Wire.begin(OLED_SDA, OLED_SCL);
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (oledReady)
  {
    // Push a completely blank frame first. The SSD1306 keeps its old pixels
    // across a microcontroller reset, so drawing the first animation frame
    // directly can briefly leave remnants of the previous screen visible.
    display.clearDisplay();
    display.display();
    delay(80);

    bootStageStartedAt = millis();
    renderBootAnimation();
  }
  else
  {
    Serial.println(F("SSD1306 display not found at 0x3C"));
  }

  loadCreds();
  beginWifi();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  ntpStarted = true;
  offlineSince = millis();
}

void loop()
{
  updateRandomEmote();
  renderDisplay();
  if (webStarted)
    server.handleClient();
  if (portalActive)
    dnsServer.processNextRequest();
  if (mdnsStarted)
    platformMdnsUpdate();

  static wl_status_t previousStatus = WL_IDLE_STATUS;
  const wl_status_t currentStatus = WiFi.status();
  if (currentStatus == WL_CONNECTED && previousStatus != WL_CONNECTED)
  {
    Serial.printf("Connected. IP: %s\n", WiFi.localIP().toString().c_str());
    digitalWrite(STATUS_LED, LOW); // on
    nextWeatherAttempt = 0;
    stopPortal();
    // Enable modem sleep now that we're associated, to cut idle power.
    platformWifiSleep(true);
    startWebServer();
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

  // Resolve city/country to coordinates once online.
  if (currentStatus == WL_CONNECTED && !locResolved &&
      static_cast<int32_t>(millis() - nextLocationAttempt) >= 0)
  {
    if (!resolveLocation())
      nextLocationAttempt = millis() + LOCATION_RETRY_MS;
  }

  if (currentStatus == WL_CONNECTED && locResolved &&
      static_cast<int32_t>(millis() - nextWeatherAttempt) >= 0)
  {
    updateWeather();
  }
  delay(5);
}
