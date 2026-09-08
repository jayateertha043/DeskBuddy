#include "display/screen_header.h"

#include <Arduino.h>
#include <math.h>

#include "core/settings.h"
#include "display/canvas.h"

using Canvas::animationFrame;
using Canvas::display;

namespace ScreenHeader
{
    // "Hi <name> !" animated in the top strip, replacing weather/clock.
    void draw()
    {
        const String hi = String(F("Hi ")) + Settings::name() + F(" !");
        display.setTextSize(1);
        const int textW = static_cast<int>(hi.length()) * 6;
        if (textW <= 122)
        {
            const int sway = static_cast<int>(3.0f * sinf(animationFrame * 0.2f));
            display.setCursor(max(0, (128 - textW) / 2) + sway, 5);
            display.print(hi);
        }
        else
        {
            // Too long to fit: scroll it across the strip as a marquee.
            const int span = textW + 128;
            const int x = 128 - static_cast<int>((animationFrame * 2) % span);
            display.setCursor(x, 5);
            display.print(hi);
        }
    }
}
