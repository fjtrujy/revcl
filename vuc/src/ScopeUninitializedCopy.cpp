// Authenticated VUC GCC 3.3.3 STL scope-copy function at 0x0804f220.
// Recovered C++03 EH declaration preserves original cleanup-loop alignment.
// This is a function-level match, not a complete historical source unit.
#include <vector>
#include <new>
#include <bits/stl_uninitialized.h>
#include "MLHashFunGameString.h"
#include "ScopeCopyTable.h"

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
