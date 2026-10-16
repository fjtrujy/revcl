#include <cstring>
#include <cstdio>
#include <cassert>
#include <cstdarg>
// Byte-exact C++03 reconstruction of VUC VFormatString at 0x08071a50.
// The original ELF authenticates the source filename/line diagnostics and
// 238 instruction bytes. Shared-header provenance remains unestablished.
#ifndef VUC_ML_TEMP_STRING_DATA_H
#define VUC_ML_TEMP_STRING_DATA_H

#include <cstring>
class MLTempStringData {
public:
    static MLTempStringData *Get();
    MLTempStringData() {
        memset(bytes_, 'X', sizeof bytes_);
        bytes_[0x1fbf] = 0;
        used_ = 0;
    }
    char *FormatString(const char *format, ...);
    int Max() const { return 0x1fbf; }
    char *Reserve(int length);
    char *VFormatString(const char *format, char *arguments);
private:
    char bytes_[0x2fc0];
    unsigned used_;
};
#endif

inline char *MLTempStringData::Reserve(int strLen)
{
    if (Max() - (int)used_ < strLen) {
        used_ = 0;
#line 145 "../megalib/MLGameString.cpp"
        assert(Max() > strLen);
    }
    return bytes_ + used_;
}

char *MLTempStringData::VFormatString(const char *format, char *arguments)
{
    int strLen = strlen(format);
    char *start = Reserve(strLen + 1);
    int numWritten = vsprintf(start, format, (va_list)arguments);
    if (numWritten < 0) {
        used_ = 0;
        start = bytes_;
        numWritten = vsprintf(start, format, (va_list)arguments);
#line 200 "../megalib/MLGameString.cpp"
        assert(numWritten >= 0);
    }
    used_ += numWritten + 1;
    return start;
}
