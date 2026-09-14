#include "display/screen_boot.h"

#include <Arduino.h>
#include <math.h>

#include "app_state.h"
#include "core/clock.h"
#include "display/canvas.h"

using Canvas::display;
using Canvas::drawCenteredText;
using Canvas::drawHappyEyes;
using Canvas::drawLiquidEye;
using Canvas::easeLiquid;
using Canvas::fastSin;

namespace ScreenBoot
{
    namespace
    {
        BootStage bootStage = BOOT_WAITING;
        uint32_t bootStageStartedAt = 0;

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
    }

    void begin() { bootStageStartedAt = millis(); }

    bool complete() { return bootStage == BOOT_COMPLETE; }

    // Face-only wake-up choreography inspired by the smooth transitions and
    // directional gaze principles of the open-source FluxGarage RoboEyes project.
    // This original state machine stays non-blocking so networking keeps running.
    bool render()
    {
        if (bootStage == BOOT_COMPLETE)
            return false;

        const int hour = Clock::localHour();
        // Start the boot animation immediately on power-up. WiFi/time/weather are
        // fetched in parallel, so we no longer wait for them before animating.
        if (bootStage == BOOT_WAITING)
        {
            bootStage = BOOT_OPENING;
            bootStageStartedAt = millis();
        }

        uint32_t elapsed = millis() - bootStageStartedAt;
        const uint32_t duration = bootStage == BOOT_OPENING        ? 1250UL
                                  : bootStage == BOOT_LOOK_LEFT    ? 750UL
                                  : bootStage == BOOT_LOOK_RIGHT   ? 1050UL
                                  : bootStage == BOOT_DOUBLE_BLINK ? 1050UL
                                  : bootStage == BOOT_EXCITED      ? 1650UL
                                  : bootStage == BOOT_GREETING     ? 2200UL
                                  : bootStage == BOOT_INTRO        ? 2200UL
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
            const int breathe = static_cast<int>(fastSin(millis() * 0.004f));
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
            const float pop = fabsf(fastSin(progress * 2.0f * PI));
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
            const int bounce = static_cast<int>(2.0f * fastSin(elapsed * 0.009f));
            drawHappyEyes(33 + bounce);
            drawBootSmile(0, bounce);
            drawCenteredText(Clock::greeting(hour), 5);
        }
        else if (bootStage == BOOT_INTRO)
        {
            const int sway = static_cast<int>(2.0f * fastSin(elapsed * 0.008f));
            drawLiquidEye(43 + sway, 32, 23, 16);
            drawLiquidEye(85 + sway, 32, 23, 16);
            drawBootSmile(sway);
            drawCenteredText(F("Hi! I am DeskBuddy"), 5);
        }

        display.display();
        return true;
    }
}
