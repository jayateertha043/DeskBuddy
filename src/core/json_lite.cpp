#include "core/json_lite.h"

namespace Json
{
    float number(const String &body, const char *key, float fallback)
    {
        const int currentStart = body.indexOf("\"current\":");
        if (currentStart < 0)
            return fallback;
        String needle = String('"') + key + "\":";
        int start = body.indexOf(needle, currentStart);
        if (start < 0)
            return fallback;
        start += needle.length();
        return body.substring(start).toFloat();
    }

    float numberFrom(const String &body, const char *key, int from, float fallback)
    {
        String needle = String('"') + key + "\":";
        int start = body.indexOf(needle, from);
        if (start < 0)
            return fallback;
        start += needle.length();
        return body.substring(start).toFloat();
    }

    String stringFrom(const String &body, const char *key, int from, int until)
    {
        String needle = String('"') + key + "\":\"";
        int start = body.indexOf(needle, from);
        if (start < 0 || start >= until)
            return String();
        start += needle.length();
        String value;
        bool escaped = false;
        for (int i = start; i < until; ++i)
        {
            const char c = body[i];
            if (!escaped && c == '"')
                break;
            if (!escaped && c == '\\')
            {
                escaped = true;
                continue;
            }
            value += c;
            escaped = false;
        }
        return value;
    }
}
