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
        uint8_t emoteModeVal = EMOTE_STATIC;
        String userName;
        uint16_t pomoMinutes = 25;
        uint8_t brightPct = 80;
        uint8_t loopSeq[MOOD_COUNT];
        uint8_t loopSeqLen = 0;
        uint16_t loopSecs = 10;
        String stickyMsg = "Hello!";
        uint8_t powerModeVal = POWER_NONE;
        bool nightSleepOn = false;
        uint16_t nightStartMin = 60;  // 01:00
        uint16_t nightEndMin = 420;   // 07:00

        void setDefaultLoopSeq()
        {
            loopSeqLen = 0;
            for (uint8_t i = 1; i < MOOD_COUNT; ++i)
                loopSeq[loopSeqLen++] = i;
        }

        // Parse an ordered CSV of mood indices into loopSeq (dedup, validated).
        // Falls back to the full default list when the CSV yields nothing.
        void parseLoopSeq(const String &csv)
        {
            loopSeqLen = 0;
            int start = 0;
            while (start < static_cast<int>(csv.length()) && loopSeqLen < MOOD_COUNT)
            {
                int comma = csv.indexOf(',', start);
                if (comma < 0)
                    comma = csv.length();
                String tok = csv.substring(start, comma);
                tok.trim();
                if (tok.length())
                {
                    const int value = tok.toInt();
                    if (value >= 0 && value < MOOD_COUNT) // 0 = auto (weather) face
                    {
                        bool dup = false;
                        for (uint8_t k = 0; k < loopSeqLen; ++k)
                            if (loopSeq[k] == value)
                            {
                                dup = true;
                                break;
                            }
                        if (!dup)
                            loopSeq[loopSeqLen++] = static_cast<uint8_t>(value);
                    }
                }
                start = comma + 1;
            }
            if (loopSeqLen == 0)
                setDefaultLoopSeq();
        }
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
        // Try to load new emoteMode first; if not found, check old "random" bool key
        emoteModeVal = prefs.getUChar("emotemode", 255);
        if (emoteModeVal == 255)
        {
            // Fallback for old installations using bool "random"
            bool oldRandom = prefs.getBool("random", false);
            emoteModeVal = oldRandom ? EMOTE_RANDOM : EMOTE_STATIC;
        }
        if (emoteModeVal >= 3) // Ensure it's a valid mode
            emoteModeVal = EMOTE_STATIC;
        if (emoteModeVal == EMOTE_RANDOM || emoteModeVal == EMOTE_LOOP)
            moodSel = MOOD_AUTO; // Random/Loop modes rest on the weather face between reactions.
        userName = prefs.getString("name", "");
        pomoMinutes = prefs.getUShort("pomomin", 25);
        if (pomoMinutes == 0 || pomoMinutes > 180)
            pomoMinutes = 25;
        brightPct = prefs.getUChar("bright", 80);
        if (brightPct < 5)
            brightPct = 5;
        if (brightPct > 100)
            brightPct = 100;
        {
            const String seqCsv = prefs.getString("loopseq", "");
            if (seqCsv.length())
                parseLoopSeq(seqCsv);
            else
                setDefaultLoopSeq();
        }
        loopSecs = prefs.getUShort("loopsec", 10);
        if (loopSecs < 3)
            loopSecs = 3;
        if (loopSecs > 300)
            loopSecs = 300;
        stickyMsg = prefs.getString("sticky", "Hello!");
        powerModeVal = prefs.getUChar("pwrmode", POWER_NONE);
        if (powerModeVal > POWER_SAVING)
            powerModeVal = POWER_NONE;
        nightSleepOn = prefs.getBool("slpen", false);
        nightStartMin = prefs.getUShort("slpstart", 60);
        nightEndMin = prefs.getUShort("slpend", 420);
        if (nightStartMin > 1439)
            nightStartMin = 60;
        if (nightEndMin > 1439)
            nightEndMin = 420;
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
    uint8_t emoteMode() { return emoteModeVal; }
    bool randomMode() { return emoteModeVal == EMOTE_RANDOM; }
    const String &name() { return userName; }
    uint16_t pomodoroMinutes() { return pomoMinutes; }
    uint8_t brightnessPercent() { return brightPct; }
    uint8_t brightness() { return static_cast<uint8_t>((static_cast<uint16_t>(brightPct) * 255) / 100); }
    uint8_t loopCount() { return loopSeqLen; }
    uint8_t loopAt(uint8_t i) { return i < loopSeqLen ? loopSeq[i] : MOOD_AUTO; }
    uint16_t loopSeconds() { return loopSecs; }
    const String &stickyText() { return stickyMsg; }
    uint8_t powerMode() { return powerModeVal; }
    bool nightSleepEnabled() { return nightSleepOn; }
    uint16_t nightSleepStart() { return nightStartMin; }
    uint16_t nightSleepEnd() { return nightEndMin; }

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

    void saveEmoteMode(uint8_t mode)
    {
        if (mode >= 3) // Ensure valid mode
            mode = EMOTE_STATIC;
        emoteModeVal = mode;
        prefs.begin("deskbuddy", false);
        prefs.putUChar("emotemode", mode);
        prefs.end();
    }

    void saveName(const String &name)
    {
        userName = name;
        prefs.begin("deskbuddy", false);
        prefs.putString("name", name);
        prefs.end();
    }

    void savePomodoroMinutes(uint16_t minutes)
    {
        if (minutes == 0 || minutes > 180)
            minutes = 25;
        pomoMinutes = minutes;
        prefs.begin("deskbuddy", false);
        prefs.putUShort("pomomin", minutes);
        prefs.end();
    }

    void saveBrightness(uint8_t percent)
    {
        if (percent < 5)
            percent = 5;
        if (percent > 100)
            percent = 100;
        brightPct = percent;
        prefs.begin("deskbuddy", false);
        prefs.putUChar("bright", percent);
        prefs.end();
    }

    void saveLoopSequence(const String &csv)
    {
        parseLoopSeq(csv);
        String normalized;
        for (uint8_t i = 0; i < loopSeqLen; ++i)
        {
            if (i)
                normalized += ',';
            normalized += loopSeq[i];
        }
        prefs.begin("deskbuddy", false);
        prefs.putString("loopseq", normalized);
        prefs.end();
    }

    void saveLoopSeconds(uint16_t seconds)
    {
        if (seconds < 3)
            seconds = 3;
        if (seconds > 300)
            seconds = 300;
        loopSecs = seconds;
        prefs.begin("deskbuddy", false);
        prefs.putUShort("loopsec", seconds);
        prefs.end();
    }

    void saveStickyText(const String &text)
    {
        stickyMsg = text;
        prefs.begin("deskbuddy", false);
        prefs.putString("sticky", text);
        prefs.end();
    }

    void savePowerMode(uint8_t mode)
    {
        if (mode > POWER_SAVING)
            mode = POWER_NONE;
        powerModeVal = mode;
        prefs.begin("deskbuddy", false);
        prefs.putUChar("pwrmode", mode);
        prefs.end();
    }

    void saveNightSleep(bool enabled, uint16_t startMin, uint16_t endMin)
    {
        if (startMin > 1439)
            startMin = 0;
        if (endMin > 1439)
            endMin = 0;
        nightSleepOn = enabled;
        nightStartMin = startMin;
        nightEndMin = endMin;
        prefs.begin("deskbuddy", false);
        prefs.putBool("slpen", enabled);
        prefs.putUShort("slpstart", startMin);
        prefs.putUShort("slpend", endMin);
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
