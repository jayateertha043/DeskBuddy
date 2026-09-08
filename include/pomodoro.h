// Pomodoro focus timer: countdown state machine driven from the dashboard.
#pragma once

#include <Arduino.h>

namespace Pomodoro
{
    void start(uint16_t minutes);
    void cancel();
    void update(); // advance the state machine each loop

    bool active();   // RUNNING or the post-session celebration
    bool running();  // counting down
    bool finished(); // in the short "done" celebration window

    uint32_t remainingMs();
    uint32_t durationMs();
    float progress(); // 0..1 elapsed fraction
}
