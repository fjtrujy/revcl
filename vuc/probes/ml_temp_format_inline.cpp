// Provisional formatted string constructor fragment.
// Get() is defined here to expose its body to historic GCC 3.3.3 inlining;
// the original complete translation unit is not yet recovered.
#include "../src/MLGameString.h"
#include "../src/MLTempStringData.h"
#include <cstdarg>

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
