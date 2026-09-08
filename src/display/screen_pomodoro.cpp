#include "display/screen_pomodoro.h"

#include <Arduino.h>
#include <math.h>

#include "display/canvas.h"
#include "pomodoro.h"

using Canvas::animationFrame;
using Canvas::display;

namespace ScreenPomodoro
{
    namespace
    {
        // Hourglass geometry within the face area (below the y=17 divider).
        constexpr int CX = 64;
        constexpr int TOP_Y = 21;
        constexpr int NECK_Y = 40;
        constexpr int BOT_Y = 59;
        constexpr int HALF_W = 17;

        int topHalfAt(int y) { return HALF_W * (NECK_Y - y) / (NECK_Y - TOP_Y); }
        int botHalfAt(int y) { return HALF_W * (y - NECK_Y) / (BOT_Y - NECK_Y); }

        void drawHourglass(float p)
        {
            // Frame plates top and bottom.
            display.drawLine(CX - HALF_W - 3, TOP_Y, CX + HALF_W + 3, TOP_Y, SSD1306_WHITE);
            display.drawLine(CX - HALF_W - 3, BOT_Y, CX + HALF_W + 3, BOT_Y, SSD1306_WHITE);
            // Glass outline (bowtie).
            display.drawLine(CX - HALF_W, TOP_Y, CX, NECK_Y, SSD1306_WHITE);
            display.drawLine(CX + HALF_W, TOP_Y, CX, NECK_Y, SSD1306_WHITE);
            display.drawLine(CX, NECK_Y, CX - HALF_W, BOT_Y, SSD1306_WHITE);
            display.drawLine(CX, NECK_Y, CX + HALF_W, BOT_Y, SSD1306_WHITE);

            // Top chamber sand drains as progress rises (surface falls toward neck).
            const int ySurfTop = NECK_Y - static_cast<int>(sqrtf(1.0f - p) * (NECK_Y - TOP_Y));
            for (int y = ySurfTop; y < NECK_Y; ++y)
            {
                const int h = topHalfAt(y) - 1;
                if (h > 0)
                    display.drawLine(CX - h, y, CX + h, y, SSD1306_WHITE);
            }

            // Bottom chamber mound grows as progress rises. Its surface mirrors the
            // top (both equidistant from the neck) so the halves fill/drain in step.
            const int ySurfBot = NECK_Y + static_cast<int>(sqrtf(1.0f - p) * (BOT_Y - NECK_Y));
            for (int y = ySurfBot; y < BOT_Y; ++y)
            {
                const int h = botHalfAt(y) - 1;
                if (h > 0)
                    display.drawLine(CX - h, y, CX + h, y, SSD1306_WHITE);
            }

            // Falling grains through the neck.
            const int span = ySurfBot - NECK_Y - 1;
            if (span > 1)
            {
                for (int k = 0; k < 3; ++k)
                {
                    const int y = NECK_Y + 1 + ((animationFrame * 2 + k * 5) % span);
                    display.drawPixel(CX, y, SSD1306_WHITE);
                }
            }
        }

        void drawRunning()
        {
            display.setTextSize(1);
            display.setCursor(2, 2);
            display.print(F("Focus"));

            const uint32_t rem = Pomodoro::remainingMs();
            const uint32_t totalSec = (rem + 999) / 1000; // ceil to the whole second
            char buf[8];
            snprintf(buf, sizeof(buf), "%02u:%02u",
                     static_cast<unsigned>(totalSec / 60),
                     static_cast<unsigned>(totalSec % 60));
            const int textW = static_cast<int>(strlen(buf)) * 6;
            display.setCursor(126 - textW, 2);
            display.print(buf);

            display.drawLine(0, 17, 127, 17, SSD1306_WHITE);
            drawHourglass(Pomodoro::progress());
        }

        void drawFinish()
        {
            Canvas::drawCenteredText(F("Focus done!"), 3);
            display.drawLine(0, 17, 127, 17, SSD1306_WHITE);
            Canvas::drawHappyEyes(38);
            display.drawLine(54, 50, 59, 55, SSD1306_WHITE);
            display.drawLine(59, 55, 69, 55, SSD1306_WHITE);
            display.drawLine(69, 55, 74, 50, SSD1306_WHITE);
            const int s = 2 + (animationFrame / 4) % 3;
            display.drawLine(20, 34 - s, 20, 34 + s, SSD1306_WHITE);
            display.drawLine(20 - s, 34, 20 + s, 34, SSD1306_WHITE);
            display.drawLine(108, 36 - s, 108, 36 + s, SSD1306_WHITE);
            display.drawLine(108 - s, 36, 108 + s, 36, SSD1306_WHITE);
        }
    }

    void draw()
    {
        if (Pomodoro::finished())
            drawFinish();
        else
            drawRunning();
    }
}
