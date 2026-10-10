// Provisional VUC hash/equality specializations for interned MLGameString.
#ifndef VUC_ML_HASH_FUN_GAME_STRING_H
#define VUC_ML_HASH_FUN_GAME_STRING_H
#include "MLGameString.h"
template <> class MLHashFun<MLGameString> {
public:
    unsigned Hash(const MLGameString &key);
    bool IsEqual(const MLGameString &lhs, const MLGameString &rhs);
};
#endif
