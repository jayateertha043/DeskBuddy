#include "display/screen_weather.h"

#include <Arduino.h>
#include <math.h>

#include "app_state.h"
#include "display/canvas.h"
#include "net/weather_service.h"

using Canvas::animationFrame;
using Canvas::display;
using Canvas::drawHappyEyes;
using Canvas::drawLiquidEye;

namespace ScreenWeather
{
  namespace
  {
    void drawWeatherEffects(bool night)
    {
      const Weather &weather = WeatherService::data();
      const int fall = (animationFrame * 2) % 30;
      if (night)
      {
        display.fillCircle(112, 28, 7, SSD1306_WHITE);
        display.fillCircle(115, 25, 7, SSD1306_BLACK);
        display.drawPixel(16, 25, SSD1306_WHITE);
        display.drawPixel(109, 49, SSD1306_WHITE);
        if ((animationFrame / 5) % 2)
          display.drawPixel(18, 27, SSD1306_WHITE);
        const int z = (animationFrame / 8) % 3;
        display.drawLine(105 + z, 44 - z, 110 + z, 44 - z, SSD1306_WHITE);
        display.drawLine(110 + z, 44 - z, 105 + z, 49 - z, SSD1306_WHITE);
        display.drawLine(105 + z, 49 - z, 110 + z, 49 - z, SSD1306_WHITE);
      }
      if (!weather.valid)
        return;
      if (isRainCode(weather.code))
      {
        for (int i = 0; i < 4; ++i)
        {
          const int x = i < 2 ? 8 + i * 10 : 106 + (i - 2) * 10;
          const int y = 22 + (fall + i * 9) % 35;
          display.drawLine(x, y, x - 2, y + 4, SSD1306_WHITE);
        }
      }
      else if (isSnowCode(weather.code))
      {
        for (int i = 0; i < 4; ++i)
        {
          const int x = i < 2 ? 10 + i * 10 : 107 + (i - 2) * 10;
          const int y = 23 + (fall / 2 + i * 11) % 34;
          display.drawPixel(x, y, SSD1306_WHITE);
          display.drawPixel(x - 1, y, SSD1306_WHITE);
          display.drawPixel(x + 1, y, SSD1306_WHITE);
          display.drawPixel(x, y - 1, SSD1306_WHITE);
          display.drawPixel(x, y + 1, SSD1306_WHITE);
        }
      }
      else if (isStormCode(weather.code))
      {
        const int flash = animationFrame % 24 < 5 ? 2 : 0;
        display.drawLine(11, 23, 6 + flash, 36, SSD1306_WHITE);
        display.drawLine(6 + flash, 36, 12, 35, SSD1306_WHITE);
        display.drawLine(12, 35, 7 + flash, 50, SSD1306_WHITE);
        display.drawLine(115, 24, 109, 37, SSD1306_WHITE);
        display.drawLine(109, 37, 116, 35, SSD1306_WHITE);
        display.drawLine(116, 35, 111, 49, SSD1306_WHITE);
      }
      else if (isFogCode(weather.code))
      {
        const int drift = (animationFrame / 3) % 6;
        display.drawLine(3 + drift, 27, 24 + drift, 27, SSD1306_WHITE);
        display.drawLine(103 - drift, 35, 124 - drift, 35, SSD1306_WHITE);
        display.drawLine(5, 49, 25, 49, SSD1306_WHITE);
      }
      else if (weather.code >= 1 && weather.code <= 3)
      {
        const int drift = (animationFrame / 8) % 7;
        display.fillCircle(8 + drift, 29, 4, SSD1306_WHITE);
        display.fillCircle(14 + drift, 27, 6, SSD1306_WHITE);
        display.fillRect(5 + drift, 29, 15, 4, SSD1306_WHITE);
      }
      else if (!night)
      {
        const int pulse = (animationFrame / 5) % 2;
        display.drawLine(10, 26 - pulse, 10, 21 - pulse, SSD1306_WHITE);
        display.drawLine(6, 23 - pulse, 3, 20 - pulse, SSD1306_WHITE);
        display.drawLine(14, 23 - pulse, 17, 20 - pulse, SSD1306_WHITE);
      }
    }
  }

  void drawIcon(int x, int y, int code)
  {
    if (code == 0)
    {
      display.drawCircle(x + 6, y + 6, 4, SSD1306_WHITE);
      for (int a = 0; a < 8; ++a)
      {
        float angle = a * PI / 4.0f;
        display.drawLine(x + 6 + cos(angle) * 6, y + 6 + sin(angle) * 6,
                         x + 6 + cos(angle) * 8, y + 6 + sin(angle) * 8, SSD1306_WHITE);
      }
    }
    else if (isRainCode(code) || isStormCode(code))
    {
      display.fillCircle(x + 5, y + 5, 4, SSD1306_WHITE);
      display.fillCircle(x + 10, y + 6, 5, SSD1306_WHITE);
      display.fillRect(x + 3, y + 6, 12, 4, SSD1306_WHITE);
      display.drawLine(x + 5, y + 12, x + 4, y + 15, SSD1306_WHITE);
      display.drawLine(x + 11, y + 12, x + 10, y + 15, SSD1306_WHITE);
    }
    else
    {
      display.fillCircle(x + 5, y + 7, 4, SSD1306_WHITE);
      display.fillCircle(x + 10, y + 6, 5, SSD1306_WHITE);
      display.fillRect(x + 3, y + 7, 13, 4, SSD1306_WHITE);
    }
  }

  void drawFace()
  {
    const Weather &weather = WeatherService::data();
    const bool night = weather.valid && !weather.isDay;
    const bool rain = weather.valid && isRainCode(weather.code);
    const bool snow = weather.valid && isSnowCode(weather.code);
    const bool storm = weather.valid && isStormCode(weather.code);
    const bool fog = weather.valid && isFogCode(weather.code);
    const uint16_t blinkPhase = animationFrame % 82;
    int eyeHeight = 12;
    if (blinkPhase == 76 || blinkPhase == 79)
      eyeHeight = 7;
    else if (blinkPhase >= 77 && blinkPhase <= 78)
      eyeHeight = 2;
    const int drift = static_cast<int>(2.0f * sinf(animationFrame * 0.035f));

    drawWeatherEffects(night);
    if ((night && !storm) || fog)
    {
      display.drawLine(35 + drift, 35, 44 + drift, 38, SSD1306_WHITE);
      display.drawLine(44 + drift, 38, 53 + drift, 35, SSD1306_WHITE);
      display.drawLine(75 + drift, 35, 84 + drift, 38, SSD1306_WHITE);
      display.drawLine(84 + drift, 38, 93 + drift, 35, SSD1306_WHITE);
    }
    else if (snow)
    {
      drawHappyEyes(35, drift);
    }
    else
    {
      drawLiquidEye(44 + drift, 35, 20, storm ? 15 : eyeHeight);
      drawLiquidEye(84 + drift, 35, 20, storm ? 15 : eyeHeight);
    }

    if (storm)
    {
      display.drawRoundRect(60 + drift, 47, 8, 7, 3, SSD1306_WHITE);
    }
    else if (rain)
    {
      display.drawLine(56 + drift, 54, 60 + drift, 50, SSD1306_WHITE);
      display.drawLine(60 + drift, 50, 68 + drift, 50, SSD1306_WHITE);
      display.drawLine(68 + drift, 50, 72 + drift, 54, SSD1306_WHITE);
    }
    else if (night || fog)
    {
      display.drawLine(57 + drift, 51, 71 + drift, 51, SSD1306_WHITE);
    }
    else
    {
      display.drawLine(54 + drift, 48, 59 + drift, 53, SSD1306_WHITE);
      display.drawLine(59 + drift, 53, 69 + drift, 53, SSD1306_WHITE);
      display.drawLine(69 + drift, 53, 74 + drift, 48, SSD1306_WHITE);
    }
  }
}
