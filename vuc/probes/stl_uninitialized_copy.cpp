// Source-shape probe using original GCC 3.3 libstdc++ uninitialized_copy.
#include <vector>
#include <bits/stl_uninitialized.h>
struct Type {
    unsigned a,b,c,d;
    Type(const Type &o) : a(o.a),b(o.b),c(o.c),d(o.d) {}
};
struct AsmVariable {
    unsigned a,b;
    AsmVariable(const AsmVariable &o) { b=o.b; a=o.a; }
};
typedef std::vector<Type> TypeVector;
typedef __gnu_cxx::__normal_iterator<const Type *, TypeVector> ConstTypeIterator;
typedef __gnu_cxx::__normal_iterator<Type *, TypeVector> TypeIterator;
typedef std::vector<AsmVariable> AsmVector;
typedef __gnu_cxx::__normal_iterator<const AsmVariable *, AsmVector> ConstAsmIterator;
typedef __gnu_cxx::__normal_iterator<AsmVariable *, AsmVector> AsmIterator;

template Type *std::__uninitialized_copy_aux<ConstTypeIterator, Type *>(
    ConstTypeIterator, ConstTypeIterator, Type *, __false_type);
template TypeIterator std::__uninitialized_copy_aux<TypeIterator, TypeIterator>(
    TypeIterator, TypeIterator, TypeIterator, __false_type);
template AsmVariable *std::__uninitialized_copy_aux<ConstAsmIterator, AsmVariable *>(
    ConstAsmIterator, ConstAsmIterator, AsmVariable *, __false_type);
template AsmIterator std::__uninitialized_copy_aux<AsmIterator, AsmIterator>(
    AsmIterator, AsmIterator, AsmIterator, __false_type);
