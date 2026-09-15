// Shared application types: moods, weather snapshot, boot stages.
#pragma once

#include <Arduino.h>

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
    MOOD_MOON,
    MOOD_CLOCK,
    MOOD_STICKY,
    MOOD_COUNT
};

enum EmoteMode : uint8_t
{
    EMOTE_STATIC = 0,
    EMOTE_RANDOM = 1,
    EMOTE_LOOP = 2
};

// Power-saving strategy for the WiFi radio.
enum PowerMode : uint8_t
{
    POWER_NONE = 0,  // WiFi always on (default behaviour)
    POWER_SAVING = 1 // WiFi on-demand: scheduler wakes it before internet tasks
};

extern const char *const MOOD_SLUG[MOOD_COUNT];
extern const char *const MOOD_LABEL[MOOD_COUNT];
extern const char *const MOOD_ICON[MOOD_COUNT];

uint8_t moodFromSlug(const String &slug);

struct Weather
{
    bool valid = false;
    float temperature = 0;
    int code = -1;
    bool isDay = true;
    String summary = "Waiting";
    uint32_t updatedAt = 0;
};

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

inline bool isRainCode(int code) { return (code >= 51 && code <= 67) || (code >= 80 && code <= 82); }
inline bool isSnowCode(int code) { return (code >= 71 && code <= 77) || code == 85 || code == 86; }
inline bool isStormCode(int code) { return code >= 95; }
inline bool isFogCode(int code) { return code == 45 || code == 48; }
