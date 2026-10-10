// Exact GCC 3.3 std::__uninitialized_copy_aux instantiations for VUC Type.
// Source unit boundaries and the original Type declaration remain provisional.
#include <vector>
#include <bits/stl_uninitialized.h>

struct Type {
    unsigned a, b, c, d;
    Type(const Type &other)
        : a(other.a), b(other.b), c(other.c), d(other.d) {}
};
typedef std::vector<Type> TypeVector;
typedef __gnu_cxx::__normal_iterator<const Type *, TypeVector> ConstTypeIterator;
typedef __gnu_cxx::__normal_iterator<Type *, TypeVector> TypeIterator;

template Type *std::__uninitialized_copy_aux<ConstTypeIterator, Type *>(
    ConstTypeIterator, ConstTypeIterator, Type *, __false_type);
template TypeIterator std::__uninitialized_copy_aux<TypeIterator, TypeIterator>(
    TypeIterator, TypeIterator, TypeIterator, __false_type);
