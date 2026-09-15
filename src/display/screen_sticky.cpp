// Renders the Sticky Note emote: arbitrary user text drawn as large as it can
// fit on the 128x64 OLED without collisions, word-wrapped and centered. A
// curated set of emojis is rendered as monochrome bitmaps so notes stay lively.
#include "display/screen_sticky.h"

#include <Arduino.h>

#include "app_state.h"
#include "core/settings.h"
#include "display/canvas.h"

using Canvas::display;

namespace ScreenSticky
{
    namespace
    {
        constexpr int kScreenW = 128;
        constexpr int kScreenH = 64;
        constexpr int kMaxW = 126; // 1px side margins
        constexpr int kMaxH = 62;
        constexpr int kMaxGlyphs = 160;
        constexpr int kMaxLines = 8;

        // --- 8x8 monochrome emoji tiles (MSB = leftmost pixel) -----------------
        // Each is scaled up by the current text size when drawn so it matches the
        // GFX character cell height (8*size).
        const uint8_t kSmile[8] = {0x3C, 0x42, 0xA5, 0x81, 0xA5, 0x99, 0x42, 0x3C};
        const uint8_t kSad[8] = {0x3C, 0x42, 0xA5, 0x81, 0x99, 0xA5, 0x42, 0x3C};
        const uint8_t kHeart[8] = {0x66, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00};
        const uint8_t kStar[8] = {0x18, 0x3C, 0xFF, 0x7E, 0x3C, 0x66, 0xC3, 0x00};
        const uint8_t kThumb[8] = {0x06, 0x0C, 0x0C, 0x7C, 0xFC, 0xFC, 0xFC, 0x78};
        const uint8_t kFire[8] = {0x10, 0x18, 0x3C, 0x7E, 0x7E, 0xFF, 0xE7, 0x7E};
        const uint8_t kCheck[8] = {0x01, 0x03, 0x06, 0x8C, 0xD8, 0x70, 0x20, 0x00};
        const uint8_t kCoffee[8] = {0x00, 0x7C, 0xFE, 0xFF, 0xFE, 0x7C, 0x38, 0x00};
        const uint8_t kBulb[8] = {0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x3C, 0x3C, 0x18};
        const uint8_t kParty[8] = {0x02, 0x06, 0x0E, 0x1E, 0x3E, 0x7E, 0xFE, 0x00};
        const uint8_t kCool[8] = {0x3C, 0x42, 0xFF, 0xDB, 0x81, 0xA5, 0x42, 0x3C};
        const uint8_t kSun[8] = {0x18, 0x99, 0x3C, 0x7E, 0x7E, 0x3C, 0x99, 0x18};
        const uint8_t kMoon[8] = {0x1C, 0x3C, 0x78, 0x70, 0x70, 0x78, 0x3C, 0x1C};
        const uint8_t kSleep[8] = {0xFE, 0x0C, 0x18, 0x30, 0x60, 0xFE, 0x00, 0x00};
        const uint8_t kSparkle[8] = {0x10, 0x54, 0x38, 0xEE, 0x38, 0x54, 0x10, 0x00};
        const uint8_t kNote[8] = {0xFE, 0x82, 0xBA, 0x82, 0xBA, 0x82, 0xFE, 0x00};

        // Maps a Unicode codepoint to one of the tiles above (nullptr if none).
        const uint8_t *emojiTile(uint32_t cp)
        {
            switch (cp)
            {
            case 0x1F600:
            case 0x1F601:
            case 0x1F603:
            case 0x1F604:
            case 0x1F60A:
            case 0x1F642:
            case 0x263A:
                return kSmile;
            case 0x1F622:
            case 0x1F62D:
            case 0x1F641:
            case 0x2639:
                return kSad;
            case 0x2764:
            case 0x1F495:
            case 0x1F496:
            case 0x1F499:
            case 0x1F49A:
            case 0x1F49B:
            case 0x1F9E1:
            case 0x1F49C:
                return kHeart;
            case 0x2B50:
            case 0x1F31F:
                return kStar;
            case 0x1F44D:
                return kThumb;
            case 0x1F525:
                return kFire;
            case 0x2705:
            case 0x2714:
                return kCheck;
            case 0x2615:
                return kCoffee;
            case 0x1F4A1:
                return kBulb;
            case 0x1F389:
            case 0x1F38A:
                return kParty;
            case 0x1F60E:
                return kCool;
            case 0x2600:
            case 0x1F31E:
                return kSun;
            case 0x1F319:
            case 0x1F31B:
            case 0x1F31C:
            case 0x1F311:
                return kMoon;
            case 0x1F634:
            case 0x1F4A4:
                return kSleep;
            case 0x2728:
                return kSparkle;
            case 0x1F4DD:
                return kNote;
            default:
                return nullptr;
            }
        }

        struct Glyph
        {
            const uint8_t *tile; // non-null => emoji tile, else printable char
            char ch;
            bool space;
        };

        Glyph gGlyphs[kMaxGlyphs];
        int gCount = 0;

        // Decode the UTF-8 note into a flat glyph list. Emoji modifiers (skin
        // tone, variation selector, ZWJ) are skipped so base emojis still map.
        void decode(const String &text)
        {
            gCount = 0;
            const uint8_t *s = reinterpret_cast<const uint8_t *>(text.c_str());
            const int n = text.length();
            int i = 0;
            while (i < n && gCount < kMaxGlyphs)
            {
                uint8_t b = s[i];
                uint32_t cp = 0;
                int len = 1;
                if (b < 0x80)
                {
                    cp = b;
                    len = 1;
                }
                else if ((b & 0xE0) == 0xC0)
                {
                    cp = b & 0x1F;
                    len = 2;
                }
                else if ((b & 0xF0) == 0xE0)
                {
                    cp = b & 0x0F;
                    len = 3;
                }
                else if ((b & 0xF8) == 0xF0)
                {
                    cp = b & 0x07;
                    len = 4;
                }
                else
                {
                    i += 1; // stray continuation byte
                    continue;
                }
                if (i + len > n)
                    break;
                for (int k = 1; k < len; ++k)
                    cp = (cp << 6) | (s[i + k] & 0x3F);
                i += len;

                // Drop combining/joining modifiers that would otherwise render
                // as blanks or split emoji sequences.
                if (cp == 0xFE0F || cp == 0x200D ||
                    (cp >= 0x1F3FB && cp <= 0x1F3FF))
                    continue;

                Glyph g{nullptr, ' ', false};
                if (cp == '\n' || cp == '\r' || cp == '\t' || cp == ' ')
                {
                    g.space = true;
                }
                else if (const uint8_t *tile = emojiTile(cp))
                {
                    g.tile = tile;
                }
                else if (cp >= 0x20 && cp <= 0xFF)
                {
                    g.ch = static_cast<char>(cp); // ASCII / Latin-1 via GFX font
                }
                else
                {
                    g.ch = '?'; // unsupported glyph placeholder
                }
                gGlyphs[gCount++] = g;
            }
        }

        int glyphWidth(const Glyph &g, int size)
        {
            return g.tile ? 8 * size : 6 * size; // emoji tile vs GFX char cell
        }

        int rangeWidth(int start, int end, int size)
        {
            int w = 0;
            for (int i = start; i < end; ++i)
                w += glyphWidth(gGlyphs[i], size);
            return w;
        }

        struct Line
        {
            int start;
            int end;
            int width;
        };

        // Greedy word-wrap of the glyph list at a given text size. Returns whether
        // the whole note fits within the screen (used by the auto-size search).
        bool layout(int size, Line lines[], int &lineCount, int &totalH)
        {
            const int cellH = 8 * size;
            const int vgap = size >= 2 ? 3 : 2;
            const int lineH = cellH + vgap;
            lineCount = 0;
            bool fits = true;
            int i = 0;
            while (i < gCount && lineCount < kMaxLines)
            {
                while (i < gCount && gGlyphs[i].space)
                    ++i; // trim leading spaces on each line
                if (i >= gCount)
                    break;
                const int lineStart = i;
                int w = 0;
                int lastSpace = -1;
                while (i < gCount)
                {
                    const int gw = glyphWidth(gGlyphs[i], size);
                    if (w + gw > kMaxW && i > lineStart)
                        break;
                    w += gw;
                    if (gGlyphs[i].space)
                        lastSpace = i;
                    ++i;
                }
                int lineEnd;
                if (i < gCount && lastSpace > lineStart)
                {
                    lineEnd = lastSpace; // break at the last space
                    i = lastSpace + 1;
                }
                else
                {
                    lineEnd = i; // hard break (single overlong word)
                }
                const int lw = rangeWidth(lineStart, lineEnd, size);
                if (lw > kMaxW)
                    fits = false;
                lines[lineCount++] = {lineStart, lineEnd, lw};
            }
            if (i < gCount)
                fits = false; // ran out of lines
            totalH = lineCount > 0 ? lineCount * lineH - vgap : 0;
            if (totalH > kMaxH)
                fits = false;
            return fits;
        }

        void drawEmoji(int x, int y, const uint8_t *tile, int scale)
        {
            for (int row = 0; row < 8; ++row)
                for (int col = 0; col < 8; ++col)
                    if (tile[row] & (0x80 >> col))
                        display.fillRect(x + col * scale, y + row * scale,
                                         scale, scale, SSD1306_WHITE);
        }
    }

    void draw()
    {
        decode(Settings::stickyText());

        if (gCount == 0)
        {
            display.setTextSize(1);
            const char *hint = "(empty note)";
            const int w = strlen(hint) * 6;
            display.setCursor((kScreenW - w) / 2, (kScreenH - 8) / 2);
            display.print(hint);
            return;
        }

        // Pick the largest size (3..1) whose word-wrapped layout fits.
        Line lines[kMaxLines];
        int lineCount = 0;
        int totalH = 0;
        int size = 1;
        for (int s = 3; s >= 1; --s)
        {
            if (layout(s, lines, lineCount, totalH))
            {
                size = s;
                break;
            }
            size = s; // remember smallest attempted as the fallback
        }
        // Recompute at the chosen size (fallback path may hold a larger layout).
        layout(size, lines, lineCount, totalH);

        const int cellH = 8 * size;
        const int vgap = size >= 2 ? 3 : 2;
        const int lineH = cellH + vgap;
        int y = (kScreenH - totalH) / 2;
        if (y < 0)
            y = 0;

        display.setTextWrap(false);
        display.setTextSize(size);
        for (int li = 0; li < lineCount; ++li)
        {
            const Line &ln = lines[li];
            int x = (kScreenW - ln.width) / 2;
            if (x < 0)
                x = 0;
            for (int gi = ln.start; gi < ln.end; ++gi)
            {
                const Glyph &g = gGlyphs[gi];
                if (g.tile)
                {
                    drawEmoji(x, y, g.tile, size);
                }
                else if (!g.space)
                {
                    display.setCursor(x, y);
                    display.write(static_cast<uint8_t>(g.ch));
                }
                x += glyphWidth(g, size);
            }
            y += lineH;
        }
        display.setTextWrap(true);
    }
}
