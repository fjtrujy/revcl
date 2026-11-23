// Historical VUC 2004.1 C++03 function-owner reconstruction.
//
// A default constructor and Set preceding the formatted constructor change
// GCC 3.3's register allocation enough to reproduce both C1 and C2 ABI
// functions of the FORMATTED constructor exactly under
// -O2 -frename-registers -falign-jumps=1. Other methods in this source
// unit are source-level context, not yet independently exact from this TU;
// keep this duplicate-definition owner out of a final link until the
// authentic original source-unit boundaries are established.
#include "MLTempStringData.h"
#include <cstdarg>

struct MLTempString {
    const char *text;
    MLTempString();
    void Set(const char *format, ...);
    MLTempString(const char *format, ...);
};

inline MLTempStringData *MLTempStringData::Get()
{
    static MLTempStringData tsd;
    return &tsd;
}

MLTempString::MLTempString()
{
    text = MLTempStringData::Get()->FormatString("");
}

void MLTempString::Set(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
}

MLTempString::MLTempString(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
}
