// Reconstructed VUC MLHashTable.cpp fragment.
// Only MLHashFun<MLGameString>::Hash is accepted as byte-exact so far.
#include "MLGameString.h"

template <> class MLHashFun<MLGameString> {
public:
    unsigned Hash(const MLGameString &key);
};

unsigned MLHashFun<MLGameString>::Hash(const MLGameString &key)
{
    return key.data_ ? key.data_->hash : 0;
}
