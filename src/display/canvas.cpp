#include "display/canvas.h"

#include "config.h"

using namespace cfg;

namespace Canvas
{
  Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
  uint16_t animationFrame = 0;

  namespace
  {
    bool oledReady = false;
  }

  bool begin()
  {
    Wire.begin(OLED_SDA, OLED_SCL);
    oledReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    if (oledReady)
    {
      // Push a completely blank frame first. The SSD1306 keeps its old pixels
      // across a microcontroller reset, so drawing the first animation frame
      // directly can briefly leave remnants of the previous screen visible.
      display.clearDisplay();
      display.display();
      delay(80);
    }
    return oledReady;
  }

  bool ready() { return oledReady; }

  float easeLiquid(float value)
  {
    if (value <= 0.0f)
      return 0.0f;
    if (value >= 1.0f)
      return 1.0f;
    return value * value * (3.0f - 2.0f * value);
  }

  float liquidEnvelope(uint32_t beat, uint32_t riseEnd, uint32_t holdEnd,
                       uint32_t fallEnd)
  {
    if (beat < riseEnd)
      return easeLiquid(static_cast<float>(beat) / riseEnd);
    if (beat < holdEnd)
      return 1.0f;
    if (beat < fallEnd)
    {
      return 1.0f - easeLiquid(static_cast<float>(beat - holdEnd) /
                               static_cast<float>(fallEnd - holdEnd));
    }
    return 0.0f;
  }

  void drawLiquidEye(int centerX, int centerY, int width, int height)
  {
    if (width < 4)
      width = 4;
    if (height < 2)
      height = 2;
    const int radius = min(width, height) / 2;
    display.fillRoundRect(centerX - width / 2, centerY - height / 2,
                          width, height, radius, SSD1306_WHITE);
  }

  void drawHappyEyes(int y, int xOffset)
  {
    display.drawLine(35 + xOffset, y + 3, 44 + xOffset, y - 3, SSD1306_WHITE);
    display.drawLine(44 + xOffset, y - 3, 53 + xOffset, y + 3, SSD1306_WHITE);
    display.drawLine(75 + xOffset, y + 3, 84 + xOffset, y - 3, SSD1306_WHITE);
    display.drawLine(84 + xOffset, y - 3, 93 + xOffset, y + 3, SSD1306_WHITE);
  }

  void drawCenteredText(const __FlashStringHelper *text, int y)
  {
    const String line(text);
    const int textW = static_cast<int>(line.length()) * 6;
    display.setCursor(max(0, (static_cast<int>(OLED_WIDTH) - textW) / 2), y);
    display.print(line);
  }
}
