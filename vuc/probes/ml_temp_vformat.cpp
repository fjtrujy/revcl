// Experimental source-level reconstruction of VUC VFormatString (238 bytes).
#include "../src/MLTempStringData.h"
#include <cstring>
#include <cstdio>
#include <cassert>
#include <cstdarg>
extern "C" void __assert_fail(const char*,const char*,unsigned int,const char*) __attribute__((noreturn));

char *MLTempStringData::VFormatString(const char *format, char *arguments)
{
    int strLen = strlen(format);
    if (0x1fbf - (int)used_ < strLen + 1) {
        used_ = 0;
        if (!(0x1fbf > strLen + 1))
            __assert_fail("Max() > strLen", "../megalib/MLGameString.cpp", 145, "char* MLTempStringData::Reserve(int)");
    }
    char *start = bytes_ + used_;
    int numWritten = vsprintf(start, format, (va_list)arguments);
    if (numWritten < 0) {
        used_ = 0;
        start = bytes_;
        numWritten = vsprintf(start, format, (va_list)arguments);
        if (!(numWritten >= 0))
            __assert_fail("numWritten >= 0", "../megalib/MLGameString.cpp", 200, "char* MLTempStringData::VFormatString(const char*, char*)");
    }
    used_ += numWritten + 1;
    return start;
}
