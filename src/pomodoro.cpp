#include "pomodoro.h"

#include "core/settings.h"
#include "display/canvas.h"

namespace Pomodoro
{
    namespace
    {
        enum class State : uint8_t
        {
            IDLE,
            RUNNING,
            DONE
        };

        constexpr uint32_t CELEBRATE_MS = 15UL * 1000UL;

        State state = State::IDLE;
        uint32_t endsAt = 0;
        uint32_t sessionMs = 0;
        uint32_t doneAt = 0;
    }

    void start(uint16_t minutes)
    {
        if (minutes == 0)
            minutes = 25;
        if (minutes > 180)
            minutes = 180;
        sessionMs = static_cast<uint32_t>(minutes) * 60UL * 1000UL;
        endsAt = millis() + sessionMs;
        state = State::RUNNING;
        Settings::savePomodoroMinutes(minutes);
        Canvas::animationFrame = 0;
        Serial.printf("Pomodoro started: %u min\n", minutes);
    }

    void cancel()
    {
        if (state == State::IDLE)
            return;
        state = State::IDLE;
        Serial.println(F("Pomodoro cancelled."));
    }

    void update()
    {
        if (state == State::RUNNING)
        {
            if (static_cast<int32_t>(millis() - endsAt) >= 0)
            {
                state = State::DONE;
                doneAt = millis();
                Canvas::animationFrame = 0;
                Serial.println(F("Pomodoro complete!"));
            }
        }
        else if (state == State::DONE)
        {
            if (millis() - doneAt >= CELEBRATE_MS)
                state = State::IDLE;
        }
    }

    bool active() { return state != State::IDLE; }
    bool running() { return state == State::RUNNING; }
    bool finished() { return state == State::DONE; }

    uint32_t remainingMs()
    {
        if (state != State::RUNNING)
            return 0;
        const int32_t rem = static_cast<int32_t>(endsAt - millis());
        return rem > 0 ? static_cast<uint32_t>(rem) : 0;
    }

    uint32_t durationMs() { return sessionMs; }

    float progress()
    {
        if (sessionMs == 0 || state == State::IDLE)
            return 0.0f;
        if (state == State::DONE)
            return 1.0f;
        const float elapsed = static_cast<float>(sessionMs - remainingMs());
        float p = elapsed / static_cast<float>(sessionMs);
        if (p < 0.0f)
            p = 0.0f;
        if (p > 1.0f)
            p = 1.0f;
        return p;
    }
}
