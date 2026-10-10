// Reconstructed VUC MLGameString.cpp fragments.
// Every emitted function below is independently gated against the original
// ELF32 bytes with GCC 3.3.3 -O2 -frename-registers.
// The full translation unit and original flags are not yet authenticated.
#include "MLGameString.h"
#include "MLTempStringData.h"
#include <cstring>
#include <cstdarg>

char *MLTempStringData::FormatString(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    char *result = VFormatString(format, (char *)arguments);
    va_end(arguments);
    return result;
}

MLTempString &MLTempString::operator+=(const MLTempString &other)
{
    const char *second = other.text;
    const char *first = text;
    Set("%s%s", first, second);
    return *this;
}


class MLGameStringTable {
public:
    static MLGameStringTable *Get();
    MLGameStringData *Add(const char *text, bool temporary);
};

MLGameStringData::MLGameStringData(const char *value, bool is_temporary)
{
    temporary = is_temporary;
    length = strlen(value);
    MLHashFun<const char *> hasher;
    hash = hasher.Hash(value);
    if (is_temporary)
        text = value;
    else {
        char *copy = new char[length + 1];
        strcpy(copy, value);
        text = copy;
    }
}

MLGameString::MLGameString() : data_(0) {}

MLGameString::MLGameString(const MLGameString &other) : data_(other.data_) {}

MLGameString::MLGameString(const char *text, bool temporary)
    : data_(MLGameStringTable::Get()->Add(text, temporary)) {}

MLGameString::MLGameString(const MLTempString &temporary)
{
    const char *text = temporary.text;
    data_ = MLGameStringTable::Get()->Add(text, false);
}

const char *MLGameString::c_str() const
{
    return data_->text;
}

MLTempString MLTempString::operator+(const MLTempString &other)
{
    const char *second = other.text;
    const char *first = text;
    return MLTempString("%s%s", first, second);
}
