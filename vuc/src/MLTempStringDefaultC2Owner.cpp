// Historical VUC 2004.1 C++03 function-owner reconstruction.
//
// This translation-unit order (formatted constructor, Set, default
// constructor) makes both ABI C1 and C2 of the DEFAULT constructor exact
// under GCC 3.3.3 -O2 -frename-registers. The other two methods are genuine
// source hypotheses included to reproduce original codegen context, but
// are NOT byte-exact from this owner and must not be linked from this object.
// This is an independent historical-only match unit, not a VUC executable.
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

MLTempString::MLTempString(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
}

void MLTempString::Set(const char *format, ...)
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
