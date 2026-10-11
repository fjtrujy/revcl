// Source-level ABI hypothesis: Set() returns this, rather than void.
#include "../src/MLTempStringData.h"
#include <cstdarg>

struct MLTempString {
    const char *text;
    MLTempString *Set(const char *format, ...);
};

inline MLTempStringData *MLTempStringData::Get()
{
    static MLTempStringData tsd;
    return &tsd;
}

MLTempString *MLTempString::Set(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
    return this;
}
