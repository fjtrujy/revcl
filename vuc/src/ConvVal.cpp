// Byte-exact numeric conversions via historical glibc internal entrypoints.
// The original source may have used std::strtod/strtol macros/inlines instead.
#include "MLGameString.h"
extern "C" double __strtod_internal(const char *, char **, int);
extern "C" long __strtol_internal(const char *, char **, int, int);

bool ConvVal(const MLGameString &string, float &value)
{
    value = __strtod_internal(string.c_str(), 0, 0);
    return true;
}
bool ConvVal(const MLGameString &string, int &value)
{
    value = __strtol_internal(string.c_str(), 0, 10, 0);
    return true;
}
bool ConvVal(const MLGameString &string, unsigned &value)
{
    value = __strtol_internal(string.c_str(), 0, 10, 0);
    return true;
}
