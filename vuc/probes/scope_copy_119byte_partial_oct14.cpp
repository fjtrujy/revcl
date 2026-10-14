// Forensic C++03 partial of VUC's Scope* uninitialized copy (0x0804f220).
// Historical GCC 3.3.3 emits 119 bytes versus the original 120.
// The first 75 bytes agree exactly, with six real external relocations.
// Never classify this source as a byte-exact/selected 120-byte function.
#include <vector>
#include <new>
#include <bits/stl_uninitialized.h>
#include "../src/MLHashFunGameString.h"
#include "../src/MLHashTable.h"

struct SymbolTableEntry;
typedef MLHashTable<MLGameString, SymbolTableEntry *,
                    MLHashFun<MLGameString>, 513> Scope;

namespace std {
template <>
Scope *__uninitialized_copy_aux<Scope *, Scope *>(
    Scope *first, Scope *last, Scope *result, __false_type)
{
    Scope *current = result;
    try {
        for (; first != last; ++first, ++current) {
            if (current)
                ::new((void *)current) Scope(*first);
        }
        return current;
    } catch (...) {
        Scope *ptr = result;
        while (ptr != current) {
            ptr->clear();
            ++ptr;
        }
        throw;
    }
}
}
