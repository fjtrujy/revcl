// Byte-exact GCC 3.3 string-vector uninitialized copy specialization.
#include <vector>
#include <string>
#include <bits/stl_uninitialized.h>

typedef std::vector<std::string> StringVector;
typedef __gnu_cxx::__normal_iterator<std::string *, StringVector> StringIterator;

template StringIterator std::__uninitialized_copy_aux<StringIterator, StringIterator>(
    StringIterator, StringIterator, StringIterator, __false_type);
