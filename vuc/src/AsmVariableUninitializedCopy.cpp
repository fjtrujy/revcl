// Byte-exact GCC 3.3 STL instantiations for VUC's 8-byte AsmVariable.
// The original field names and constructor spelling remain provisional.
#include <vector>
#include <bits/stl_uninitialized.h>

struct AsmVariable {
    unsigned a;
    unsigned b;
    AsmVariable() : a(0), b(0) {}
};

typedef std::vector<AsmVariable> AsmVariableVector;
typedef __gnu_cxx::__normal_iterator<const AsmVariable *, AsmVariableVector>
    ConstAsmIterator;
typedef __gnu_cxx::__normal_iterator<AsmVariable *, AsmVariableVector>
    AsmIterator;

template AsmVariable *std::__uninitialized_copy_aux<ConstAsmIterator, AsmVariable *>(
    ConstAsmIterator, ConstAsmIterator, AsmVariable *, __false_type);
template AsmIterator std::__uninitialized_copy_aux<AsmIterator, AsmIterator>(
    AsmIterator, AsmIterator, AsmIterator, __false_type);
