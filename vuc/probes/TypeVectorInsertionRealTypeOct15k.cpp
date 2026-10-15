// Probe authentic original-era std::vector<Type> template machine code using
// the established 16-byte Type copy layout; not an invented application owner.
#include "../src/TypeUninitializedCopy.cpp"

template void std::vector<Type>::_M_insert_aux(TypeIterator, const Type &);
