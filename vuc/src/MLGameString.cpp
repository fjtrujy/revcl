// Reconstructed VUC MLGameString.cpp fragments.
// Every emitted function below is independently gated against the original
// ELF32 bytes with GCC 3.3.3 -O2 -frename-registers.
// The full translation unit and original flags are not yet authenticated.
#include "MLGameString.h"

MLGameString::MLGameString() : data_(0) {}

MLGameString::MLGameString(const MLGameString &other) : data_(other.data_) {}

const char *MLGameString::c_str() const
{
    return data_->text;
}
