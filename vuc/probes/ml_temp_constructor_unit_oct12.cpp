// Historical MLTempString constructor source-unit hypothesis, in original symbol order.
#include "../src/MLTempStringData.h"
#include <cstdarg>
struct MLTempString {
    const char *text;
    MLTempString();
    MLTempString(const char *format, ...);
};
inline MLTempStringData *MLTempStringData::Get()
{
    static MLTempStringData tsd;
    return &tsd;
}
MLTempString::MLTempString(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
}
MLTempString::MLTempString()
{
    text = MLTempStringData::Get()->FormatString("");
}
