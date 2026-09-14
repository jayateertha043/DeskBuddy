#include "display/emote_director.h"

#include "config.h"
#include "app_state.h"
#include "core/settings.h"
#include "display/canvas.h"
#include "display/screen_boot.h"

using namespace cfg;

namespace EmoteDirector
{
    namespace
    {
        uint32_t nextRandomEmoteAt = 0;
        uint32_t randomEmoteEndsAt = 0;
        uint8_t lastRandomMood = MOOD_AUTO;
        uint32_t loopNextChangeAt = 0;
        uint8_t loopCurrentIndex = 0; // Index into loop sequence (1 to MOOD_COUNT-1)
    }

    void resetSchedule()
    {
        nextRandomEmoteAt = millis() + RANDOM_EMOTE_INTERVAL_MS;
        randomEmoteEndsAt = 0;
        loopNextChangeAt = millis() + Settings::loopSeconds() * 1000UL;
        loopCurrentIndex = 0;
    }

    void setNextInterval() { nextRandomEmoteAt = millis() + RANDOM_EMOTE_INTERVAL_MS; }

    void startReaction(uint8_t mood)
    {
        randomEmoteEndsAt = millis() + RANDOM_EMOTE_DURATION_MS;
        lastRandomMood = mood;
    }

    void clearReaction() { randomEmoteEndsAt = 0; }

    void update()
    {
        const uint32_t now = millis();
        const uint8_t mode = Settings::emoteMode();

        if (mode == EMOTE_STATIC)
            return;

        if (!ScreenBoot::complete())
        {
            // Start the cadence after the wake-up sequence is visible.
            Settings::setMood(MOOD_AUTO);
            randomEmoteEndsAt = 0;
            loopNextChangeAt = now + Settings::loopSeconds() * 1000UL;
            nextRandomEmoteAt = now + RANDOM_EMOTE_INTERVAL_MS;
            return;
        }

        // Handle random mode (original logic)
        if (mode == EMOTE_RANDOM)
        {
            if (randomEmoteEndsAt && static_cast<int32_t>(now - randomEmoteEndsAt) >= 0)
            {
                Settings::setMood(MOOD_AUTO);
                randomEmoteEndsAt = 0;
                Canvas::animationFrame = 0;
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
            Settings::setMood(nextMood); // Do not write periodic changes and wear out flash.
            lastRandomMood = nextMood;
            Canvas::animationFrame = 0;
            randomEmoteEndsAt = now + RANDOM_EMOTE_DURATION_MS;
            nextRandomEmoteAt = now + RANDOM_EMOTE_INTERVAL_MS;
            Serial.printf("Random emote: %s\n", MOOD_LABEL[Settings::mood()]);
        }
        else if (mode == EMOTE_LOOP)
        {
            // Loop mode: cycle through the user's chosen emote playlist every 10s.
            if (static_cast<int32_t>(now - loopNextChangeAt) >= 0)
            {
                const uint8_t count = Settings::loopCount();
                const uint32_t interval = Settings::loopSeconds() * 1000UL;
                if (count == 0)
                {
                    Settings::setMood(MOOD_AUTO);
                    loopNextChangeAt = now + interval;
                    return;
                }
                if (loopCurrentIndex >= count)
                    loopCurrentIndex = 0;

                const uint8_t nextMood = Settings::loopAt(loopCurrentIndex);
                loopCurrentIndex = (loopCurrentIndex + 1) % count;

                Settings::setMood(nextMood); // Do not write periodic changes and wear out flash.
                Canvas::animationFrame = 0;
                loopNextChangeAt = now + interval;
                Serial.printf("Loop emote: %s\n", MOOD_LABEL[Settings::mood()]);
            }
        }
    }
}
