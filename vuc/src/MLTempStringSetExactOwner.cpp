// Historical VUC 2004.1 byte-exact MLTempString::Set owner.
//
// The original method is 117 bytes with eight authenticated relocations.
// Genuine preceding operator+= and default-constructor definitions, followed
// by the formatted constructor, recover GCC 3.3.3's exact scratch-register
// allocation under -O2 -frename-registers (Jan25 and Dec06 snapshots).
//
// Set's return type is a source-level hypothesis (not encoded by Itanium
// member-function mangling). Returning this agrees with the original code.
//
// Other methods emitted here are NOT all byte-exact: this is an independent
// original-function match-only TU, not an object to link alongside the other
// MLTempString owners. No handwritten assembly, register constraints or NOPs.
#include "MLTempStringData.h"
#include <cstdarg>
#include <cstring>

struct MLTempString {
    const char *text;
    MLTempString();
    MLTempString(const char *format, ...);
    MLTempString *Set(const char *format, ...);
    MLTempString &operator+=(const MLTempString &other);
    MLTempString operator+(const MLTempString &other);
};

inline MLTempStringData *MLTempStringData::Get()
{
    static MLTempStringData tsd;
    return &tsd;
}

MLTempString &MLTempString::operator+=(const MLTempString &other)
{
    const char *second = other.text;
    const char *first = text;
    Set("%s%s", first, second);
    return *this;
}

MLTempString::MLTempString()
{
    text = MLTempStringData::Get()->FormatString("");
}

MLTempString *MLTempString::Set(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
    return this;
}

MLTempString::MLTempString(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
}
