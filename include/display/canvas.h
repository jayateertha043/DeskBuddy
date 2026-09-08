// Owns the SSD1306 panel plus low-level drawing primitives shared by screens.
#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

namespace Canvas
{
    extern Adafruit_SSD1306 display;
    extern uint16_t animationFrame;

    bool begin(); // Wire + panel init; returns true if the OLED is present
    bool ready();

    float easeLiquid(float value);
    float liquidEnvelope(uint32_t beat, uint32_t riseEnd, uint32_t holdEnd, uint32_t fallEnd);
    void drawLiquidEye(int centerX, int centerY, int width, int height);
    void drawHappyEyes(int y, int xOffset = 0);
    void drawCenteredText(const __FlashStringHelper *text, int y);
}
