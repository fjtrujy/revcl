// Fresh original-GCC C++03 Type vector insertion specialization.
// Original VUC ELF _M_insert_aux(std::vector<Type>*) at 0x0804ee10,
// original function size 525. Keep probe until strictly authenticated.
#include <vector>
struct Type {
    unsigned a,b,c,d;
    Type() {}
    Type(const Type &other):a(other.a),b(other.b),c(other.c),d(other.d) {}
};
typedef std::vector<Type> TypeVector;
typedef __gnu_cxx::__normal_iterator<Type*,TypeVector> TypeIterator;
template void TypeVector::_M_insert_aux(TypeIterator,const Type&);
