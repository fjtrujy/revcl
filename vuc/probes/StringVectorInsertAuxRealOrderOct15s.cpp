// Original VUC std::vector<std::string> GCC 3.3 template source unit.
// Original i386 ELF: _M_insert_aux at 0x0805b040 before the already exact
// uninitialized_copy_aux at 0x0805b2e0. Preserve true instantiation order.
#include <vector>
#include <string>
#include <bits/stl_uninitialized.h>
typedef std::vector<std::string> StringVector;
typedef __gnu_cxx::__normal_iterator<std::string *, StringVector> StringIterator;

template void StringVector::_M_insert_aux(StringIterator,const std::string&);
template StringIterator std::__uninitialized_copy_aux<StringIterator, StringIterator>(
    StringIterator, StringIterator, StringIterator, __false_type);
