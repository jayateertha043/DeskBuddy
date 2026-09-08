#include "app_state.h"

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

uint8_t moodFromSlug(const String &slug)
{
  for (uint8_t i = 0; i < MOOD_COUNT; ++i)
    if (slug.equalsIgnoreCase(MOOD_SLUG[i]))
      return i;
  return MOOD_COUNT;
}
