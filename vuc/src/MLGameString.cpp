// Reconstructed VUC MLGameString.cpp fragments.
// Every emitted function below is independently gated against the original
// ELF32 bytes with GCC 3.3.3 -O2 -frename-registers.
// The full translation unit and original flags are not yet authenticated.
#include "MLGameString.h"

class MLGameStringTable {
public:
    static MLGameStringTable *Get();
    MLGameStringData *Add(const char *text, bool temporary);
};

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
