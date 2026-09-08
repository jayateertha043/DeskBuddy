#include "core/util.h"

#include <math.h>

namespace Util
{
    String urlEncode(const String &value)
    {
        static const char hex[] = "0123456789ABCDEF";
        String out;
        out.reserve(value.length() * 2);
        for (size_t i = 0; i < value.length(); ++i)
        {
            const uint8_t c = static_cast<uint8_t>(value[i]);
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')
                out += static_cast<char>(c);
            else if (c == ' ')
                out += "%20";
            else
            {
                out += '%';
                out += hex[c >> 4];
                out += hex[c & 0x0F];
            }
        }
        return out;
    }

    String htmlEscape(const String &in)
    {
        String out;
        out.reserve(in.length());
        for (char c : in)
        {
            if (c == '&')
                out += "&amp;";
            else if (c == '<')
                out += "&lt;";
            else if (c == '>')
                out += "&gt;";
            else if (c == '"')
                out += "&quot;";
            else if (c == '\'')
                out += "&#39;"; // values sit in single-quoted attributes; unescaped ' breaks the form
            else
                out += c;
        }
        return out;
    }

    bool coordinatesValid(double lat, double lon)
    {
        return isfinite(lat) && isfinite(lon) && lat >= -90.0 && lat <= 90.0 &&
               lon >= -180.0 && lon <= 180.0;
    }
}
