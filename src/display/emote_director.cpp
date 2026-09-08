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
  }

  void resetSchedule()
  {
    nextRandomEmoteAt = millis() + RANDOM_EMOTE_INTERVAL_MS;
    randomEmoteEndsAt = 0;
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
    if (!Settings::randomMode())
      return;

    const uint32_t now = millis();
    if (!ScreenBoot::complete())
    {
      // Start the ten-minute cadence after the wake-up sequence is visible.
      Settings::setMood(MOOD_AUTO);
      randomEmoteEndsAt = 0;
      nextRandomEmoteAt = now + RANDOM_EMOTE_INTERVAL_MS;
      return;
    }

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
}
