// Byte-exact original MLTempString formatted C1 constructor ABI.
// The separately emitted C2 constructor remains experimental.
// Get() is defined here to expose its body to historic GCC 3.3.3 inlining;
// the original complete translation unit is not yet recovered.
#include "MLGameString.h"
#include "MLTempStringData.h"
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
#ifdef REVCL_VUC_NATIVE_ABI
    text = MLTempStringData::Get()->VFormatString(format,
                                                  reinterpret_cast<char *>(&arguments));
#else
    text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
#endif
    va_end(arguments);
}
