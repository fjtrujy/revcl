// Exact ASTVarDecl printer: source evaluation order differs from other T.
#include "ASTNodeList.h"
#include <ostream>
void Print(ASTDecl *entry, std::ostream &out);
template <class T>
void Print(ASTNodeList<T> &list, std::ostream &output, const char *separator)
{
    for (int i = 0; i < list.length; ++i) {
        T item = list.entries[i];
        Print(item, output);
        if (i + 1 != list.length)
            output << separator;
    }
}
template void Print(ASTNodeList<ASTVarDecl *> &, std::ostream &, const char *);
