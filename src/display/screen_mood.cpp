#include "display/screen_mood.h"

#include <Arduino.h>
#include <math.h>

#include "app_state.h"
#include "core/settings.h"
#include "display/canvas.h"

using Canvas::display;
using Canvas::drawHappyEyes;
using Canvas::drawLiquidEye;
using Canvas::easeLiquid;
using Canvas::liquidEnvelope;

namespace ScreenMood
{
  void draw()
  {
    // Liquid-eye personality style (matches the 8266). The selected mood stays
    // fixed; only its own animation loops via millis().
    const uint8_t moodSel = Settings::mood();
    const uint32_t elapsed = millis();
    int leftX = 44, rightX = 84, leftY = 28, rightY = 28;
    int leftW = 22, rightW = 22, leftH = 13, rightH = 13;
    int mouthX = 64, mouthY = 45;
    float action = 0.0f;
    bool happyEyes = false;
    bool sneezeSnap = false;
    bool frown = false;
    bool roundMouth = false;
    bool flatMouth = false;
    bool laughMouth = false;

    switch (moodSel)
    {
    case MOOD_HAPPY:
    {
      const float beat = static_cast<float>(elapsed % 2600) / 2600.0f;
      const int lift = static_cast<int>(5.0f * sinf(beat * PI));
      leftY -= lift;
      rightY -= lift;
      mouthY -= lift;
      break;
    }
    case MOOD_EXCITED:
    {
      const float beat = static_cast<float>(elapsed % 1500) / 1500.0f;
      const int lift = static_cast<int>(7.0f * fabsf(sinf(beat * PI)));
      leftY -= lift;
      rightY -= lift;
      mouthY -= lift;
      leftH = rightH = 17;
      break;
    }
    case MOOD_STRETCH:
    {
      action = liquidEnvelope(elapsed % 4300, 1050, 2850, 3950);
      leftY = rightY = 28 - static_cast<int>(6 * action);
      leftW = rightW = 22 - static_cast<int>(5 * action);
      leftH = rightH = 13 + static_cast<int>(7 * action);
      mouthY -= static_cast<int>(3 * action);
      happyEyes = action > 0.72f;
      break;
    }
    case MOOD_YAWN:
    {
      action = liquidEnvelope(elapsed % 4200, 900, 2700, 3700);
      leftH = rightH = 13 - static_cast<int>(10 * action);
      leftY = rightY = 28 + static_cast<int>(3 * action);
      mouthY += static_cast<int>(2 * action);
      roundMouth = true;
      break;
    }
    case MOOD_SNEEZE:
    {
      const uint32_t beat = elapsed % 3400;
      if (beat < 1350)
      {
        action = easeLiquid(static_cast<float>(beat) / 1350.0f);
        leftH = rightH = 13 - static_cast<int>(8 * action);
        leftX -= static_cast<int>(3 * action);
        rightX -= static_cast<int>(3 * action);
        mouthX -= static_cast<int>(2 * action);
        roundMouth = true;
      }
      else if (beat < 1680)
      {
        sneezeSnap = true;
        leftX += 5;
        rightX += 5;
        mouthX += 5;
        leftY += 3;
        rightY += 3;
        mouthY += 3;
      }
      else
      {
        action = liquidEnvelope(beat - 1680, 1, 1, 900);
        leftX += static_cast<int>(3 * action);
        rightX += static_cast<int>(3 * action);
        mouthX += static_cast<int>(3 * action);
        roundMouth = true;
      }
      break;
    }
    case MOOD_CURIOUS:
    {
      action = liquidEnvelope(elapsed % 4400, 900, 3000, 4000);
      leftX += static_cast<int>(6 * action);
      rightX += static_cast<int>(6 * action);
      mouthX += static_cast<int>(6 * action);
      leftY -= static_cast<int>(2 * action);
      rightY += static_cast<int>(2 * action);
      leftH += static_cast<int>(5 * action);
      rightH -= static_cast<int>(5 * action);
      roundMouth = true;
      break;
    }
    case MOOD_GLANCE_LEFT:
    case MOOD_GLANCE_RIGHT:
    {
      action = liquidEnvelope(elapsed % 3600, 700, 2500, 3350);
      const int direction = moodSel == MOOD_GLANCE_LEFT ? -1 : 1;
      const int shift = direction * static_cast<int>(9 * action);
      leftX += shift;
      rightX += shift;
      mouthX += shift / 2;
      if (direction < 0)
        leftH += static_cast<int>(4 * action);
      else
        rightH += static_cast<int>(4 * action);
      break;
    }
    case MOOD_WINK:
    {
      action = liquidEnvelope(elapsed % 3200, 180, 650, 900);
      leftH = 13 - static_cast<int>(11 * action);
      leftW = 22 + static_cast<int>(2 * action);
      break;
    }
    case MOOD_LAUGH:
    {
      const float beat = fabsf(sinf((elapsed % 1100) / 1100.0f * PI));
      const int lift = static_cast<int>(5.0f * beat);
      leftY -= lift;
      rightY -= lift;
      mouthY -= lift;
      happyEyes = true;
      laughMouth = true;
      action = beat;
      break;
    }
    case MOOD_SURPRISED:
    {
      const float pulse = sinf(elapsed * 0.004f) * 0.5f + 0.5f;
      leftW = rightW = 21 + static_cast<int>(4.0f * pulse);
      leftH = rightH = 16 + static_cast<int>(4.0f * pulse);
      leftY = rightY = 27 - static_cast<int>(2.0f * pulse);
      roundMouth = true;
      break;
    }
    case MOOD_NERVOUS:
    {
      const int jitter = static_cast<int>(2.0f * sinf(elapsed * 0.035f));
      leftX += jitter;
      rightX += jitter;
      mouthX += jitter;
      leftH = 12;
      rightH = 9;
      flatMouth = true;
      break;
    }
    case MOOD_SAD:
    {
      const int sink = static_cast<int>(2.0f * (sinf(elapsed * 0.0018f) * 0.5f + 0.5f));
      leftY = rightY = 30 + sink;
      leftH = rightH = 11;
      mouthY = 47;
      frown = true;
      break;
    }
    case MOOD_ANGRY:
    {
      const int shake = static_cast<int>(1.5f * sinf(elapsed * 0.02f));
      leftX += shake;
      rightX += shake;
      mouthX += shake;
      leftH = rightH = 8;
      frown = true;
      break;
    }
    case MOOD_CONFUSED:
    {
      const int sway = static_cast<int>(3.0f * sinf(elapsed * 0.0016f));
      leftX += sway;
      rightX += sway;
      mouthX += sway;
      leftH = 17;
      rightH = 9;
      flatMouth = true;
      break;
    }
    case MOOD_SLEEP:
    {
      const int breathe = static_cast<int>(2.0f * sinf(elapsed * 0.004f));
      leftY = rightY = 28 + breathe;
      mouthY = 45 + breathe;
      leftH = rightH = 3;
      flatMouth = true;
      break;
    }
    case MOOD_DND:
    {
      action = liquidEnvelope(elapsed % 4000, 800, 2800, 3900);
      leftH = rightH = 6;
      flatMouth = true;
      break;
    }
    default:
      break;
    }

    // Centre the face in the area below the header divider (captions removed).
    const int faceOffset = 8;
    leftY += faceOffset;
    rightY += faceOffset;
    mouthY += faceOffset;

    // Eyes.
    if (sneezeSnap)
    {
      display.drawLine(leftX - 10, leftY - 5, leftX + 10, leftY + 5, SSD1306_WHITE);
      display.drawLine(leftX + 10, leftY - 5, leftX - 10, leftY + 5, SSD1306_WHITE);
      display.drawLine(rightX - 10, rightY - 5, rightX + 10, rightY + 5, SSD1306_WHITE);
      display.drawLine(rightX + 10, rightY - 5, rightX - 10, rightY + 5, SSD1306_WHITE);
    }
    else if (happyEyes)
    {
      drawHappyEyes(leftY);
    }
    else
    {
      drawLiquidEye(leftX, leftY, leftW, leftH);
      drawLiquidEye(rightX, rightY, rightW, rightH);
      if (frown) // brows angled down toward the nose
      {
        display.drawLine(leftX - 11, leftY - 10, leftX + 8, leftY - 6, SSD1306_WHITE);
        display.drawLine(rightX - 8, rightY - 6, rightX + 11, rightY - 10, SSD1306_WHITE);
      }
      else if (moodSel == MOOD_SAD)
      {
        display.drawLine(leftX - 11, leftY - 6, leftX + 8, leftY - 10, SSD1306_WHITE);
        display.drawLine(rightX - 8, rightY - 10, rightX + 11, rightY - 6, SSD1306_WHITE);
      }
      else if (moodSel == MOOD_CONFUSED) // one quizzical raised brow
      {
        display.drawLine(rightX - 9, rightY - 12, rightX + 9, rightY - 14, SSD1306_WHITE);
      }
    }

    // Mouth.
    if (laughMouth)
    {
      const int mouthW = 10 + static_cast<int>(5 * action);
      const int mouthH = 5 + static_cast<int>(5 * action);
      display.drawRoundRect(mouthX - mouthW / 2, mouthY - mouthH / 2,
                            mouthW, mouthH, min(mouthW, mouthH) / 2, SSD1306_WHITE);
    }
    else if (moodSel == MOOD_YAWN)
    {
      const int mouthW = 7 + static_cast<int>(9 * action);
      const int mouthH = 4 + static_cast<int>(10 * action);
      display.drawRoundRect(mouthX - mouthW / 2, mouthY - mouthH / 2,
                            mouthW, mouthH, mouthW / 2, SSD1306_WHITE);
    }
    else if (sneezeSnap)
    {
      display.drawLine(mouthX - 9, mouthY + 2, mouthX - 4, mouthY - 2, SSD1306_WHITE);
      display.drawLine(mouthX - 4, mouthY - 2, mouthX + 1, mouthY + 2, SSD1306_WHITE);
      display.drawLine(mouthX + 1, mouthY + 2, mouthX + 7, mouthY - 2, SSD1306_WHITE);
    }
    else if (roundMouth && moodSel == MOOD_SNEEZE)
    {
      const int mouthW = 6 + static_cast<int>(7 * action);
      const int mouthH = 4 + static_cast<int>(7 * action);
      display.drawRoundRect(mouthX - mouthW / 2, mouthY - mouthH / 2,
                            mouthW, mouthH, mouthW / 2, SSD1306_WHITE);
    }
    else if (roundMouth) // curious: small surprised "o"
    {
      display.drawRoundRect(mouthX - 3, mouthY - 3, 7, 7, 3, SSD1306_WHITE);
    }
    else if (flatMouth)
    {
      display.drawLine(mouthX - 8, mouthY, mouthX + 8, mouthY, SSD1306_WHITE);
    }
    else if (frown)
    {
      display.drawLine(mouthX - 11, mouthY + 3, mouthX - 6, mouthY - 3, SSD1306_WHITE);
      display.drawLine(mouthX - 6, mouthY - 3, mouthX + 6, mouthY - 3, SSD1306_WHITE);
      display.drawLine(mouthX + 6, mouthY - 3, mouthX + 11, mouthY + 3, SSD1306_WHITE);
    }
    else // smile
    {
      display.drawLine(mouthX - 11, mouthY - 3, mouthX - 6, mouthY + 3, SSD1306_WHITE);
      display.drawLine(mouthX - 6, mouthY + 3, mouthX + 6, mouthY + 3, SSD1306_WHITE);
      display.drawLine(mouthX + 6, mouthY + 3, mouthX + 11, mouthY - 3, SSD1306_WHITE);
    }

    // Mood-specific extras.
    if (moodSel == MOOD_SAD)
    {
      const int ty1 = leftY + 6 + static_cast<int>(fmodf(elapsed * 0.02f, 24.0f));
      if (ty1 < 62)
        display.fillCircle(leftX - 6, ty1, 1, SSD1306_WHITE);
    }
    else if (moodSel == MOOD_SLEEP)
    {
      const int zy1 = 30 + faceOffset - static_cast<int>(fmodf(elapsed * 0.012f, 16.0f));
      display.setCursor(104, zy1);
      display.print('z');
      display.setCursor(110, zy1 - 5);
      display.print('Z');
    }
    else if (moodSel == MOOD_CONFUSED)
    {
      const int q = static_cast<int>(2.0f * sinf(elapsed * 0.004f));
      display.setCursor(104, 24 + faceOffset + q);
      display.print('?');
    }
    else if (moodSel == MOOD_DND)
    {
      const int r = 5 + static_cast<int>(2 * action);
      display.drawCircle(108, 26 + faceOffset, r, SSD1306_WHITE);
      display.drawLine(108 - r + 1, 26 + faceOffset + r - 1, 108 + r - 1, 26 + faceOffset - r + 1, SSD1306_WHITE);
    }
    else if (moodSel == MOOD_EXCITED)
    {
      const int sr = 2 + static_cast<int>((sinf(elapsed * 0.006f) * 0.5f + 0.5f) * 2.0f);
      display.drawLine(20, 26 + faceOffset - sr, 20, 26 + faceOffset + sr, SSD1306_WHITE);
      display.drawLine(20 - sr, 26 + faceOffset, 20 + sr, 26 + faceOffset, SSD1306_WHITE);
      display.drawLine(108, 28 + faceOffset - sr, 108, 28 + faceOffset + sr, SSD1306_WHITE);
      display.drawLine(108 - sr, 28 + faceOffset, 108 + sr, 28 + faceOffset, SSD1306_WHITE);
    }
    else if (moodSel == MOOD_NERVOUS)
    {
      const int drop = static_cast<int>(fmodf(elapsed * 0.018f, 22.0f));
      const int dropY = 24 + faceOffset + drop;
      if (dropY < 61)
      {
        display.drawLine(108, dropY, 106, dropY + 4, SSD1306_WHITE);
        display.drawLine(108, dropY, 110, dropY + 4, SSD1306_WHITE);
        display.fillCircle(108, dropY + 4, 2, SSD1306_WHITE);
      }
    }
  }
}
