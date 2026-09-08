#include "net/weather_service.h"

#include "config.h"
#include "core/clock.h"
#include "core/json_lite.h"
#include "core/settings.h"
#include "core/util.h"

using namespace cfg;

namespace WeatherService
{
    namespace
    {
        Weather weather;
        uint32_t nextWeatherAttempt = 0;
        uint32_t nextLocationAttempt = 0;

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
    }

    const Weather &data() { return weather; }

    void requestNow() { nextWeatherAttempt = 0; }
    void requestLocationRefresh() { nextLocationAttempt = 0; }

    // Resolve city/country -> coordinates via Open-Meteo geocoding.
    bool resolveLocation()
    {
        if (!Settings::city().length() || !Settings::country().length() ||
            WiFi.status() != WL_CONNECTED)
            return false;
        Settings::setStatus(String(F("Locating ")) + Settings::city() + F("..."));
        Serial.println(Settings::status());

        SecureHttp secure;
        HTTPClient http;
        String url = F("https://geocoding-api.open-meteo.com/v1/search?name=");
        url += Util::urlEncode(Settings::city());
        url += F("&count=10&language=en&format=json");
        if (!http.begin(secure.ref(), url))
        {
            Settings::setStatus(F("Location service unavailable; retrying"));
            return false;
        }
        platformHttpTimeouts(http);
        const int status = http.GET();
        if (status != HTTP_CODE_OK)
        {
            Settings::setStatus(String(F("Location request failed (")) + status + F("); retrying"));
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
            const String resultCountry = Json::stringFrom(body, "country", objectStart, objectEnd);
            const String countryCode = Json::stringFrom(body, "country_code", objectStart, objectEnd);
            if (resultCountry.equalsIgnoreCase(Settings::country()) ||
                countryCode.equalsIgnoreCase(Settings::country()))
            {
                const double lat = Json::numberFrom(body, "latitude", objectStart, 999);
                const double lon = Json::numberFrom(body, "longitude", objectStart, 999);
                if (Util::coordinatesValid(lat, lon))
                {
                    const String resultCity = Json::stringFrom(body, "name", objectStart, objectEnd);
                    Settings::applyResolvedLocation(resultCity, resultCountry, lat, lon);
                    Serial.printf("Location: %s (%.4f, %.4f)\n", Settings::status().c_str(), lat, lon);
                    nextWeatherAttempt = 0;
                    return true;
                }
            }
            position = objectEnd + 1;
        }
        Settings::setStatus(F("City/country not found; check spelling"));
        return false;
    }

    void updateWeather()
    {
        if (WiFi.status() != WL_CONNECTED || !Settings::resolved())
        {
            nextWeatherAttempt = millis() + WEATHER_RETRY_MS;
            return;
        }

        SecureHttp secure;
        HTTPClient http;
        String url = F("https://api.open-meteo.com/v1/forecast?latitude=");
        url += String(Settings::lat(), 6);
        url += F("&longitude=");
        url += String(Settings::lon(), 6);
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
            Clock::setUtcOffset(static_cast<int32_t>(
                Json::numberFrom(body, "utc_offset_seconds", 0, Clock::utcOffset())));
            weather.temperature = Json::number(body, "temperature_2m", 0);
            weather.code = static_cast<int>(Json::number(body, "weather_code", -1));
            weather.isDay = Json::number(body, "is_day", 1) > 0.5f;
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

    void loop()
    {
        const bool connected = WiFi.status() == WL_CONNECTED;

        // Resolve city/country to coordinates once online.
        if (connected && !Settings::resolved() &&
            static_cast<int32_t>(millis() - nextLocationAttempt) >= 0)
        {
            if (!resolveLocation())
                nextLocationAttempt = millis() + LOCATION_RETRY_MS;
        }

        if (connected && Settings::resolved() &&
            static_cast<int32_t>(millis() - nextWeatherAttempt) >= 0)
        {
            updateWeather();
        }
    }
}
