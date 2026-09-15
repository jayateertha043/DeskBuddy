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
    void setContrast(uint8_t value); // 0-255 OLED contrast (brightness)
    void displayOff();               // power down the panel (before deep sleep)

    float easeLiquid(float value);
    float liquidEnvelope(uint32_t beat, uint32_t riseEnd, uint32_t holdEnd, uint32_t fallEnd);
    float fastSin(float x);           // software sine (no libm), visually matches sinf
    float fastCos(float x);           // fastSin(x + PI/2)
    float fastWrap(float a, float m); // fmodf replacement, valid for a >= 0
    void drawLiquidEye(int centerX, int centerY, int width, int height);
    void drawHappyEyes(int y, int xOffset = 0);
    void drawCenteredText(const __FlashStringHelper *text, int y);
}
