// Exact ASTExpr, ASTStmt and ASTDecl list printers using GCC 3.3 ostream.
#include "ASTNodeList.h"
#include <ostream>
struct ASTExpr {};
void Print(ASTExpr *entry, std::ostream &out);
void Print(ASTStmt *entry, std::ostream &out);
void Print(ASTDecl *entry, std::ostream &out);

template <class T>
void Print(ASTNodeList<T> &list, std::ostream &output, const char *separator)
{
    for (int i = 0; i < list.length; ++i) {
        T *entries = list.entries;
        Print(entries[i], output);
        if (i + 1 != list.length)
            output << separator;
    }
}
template void Print(ASTNodeList<ASTExpr *> &, std::ostream &, const char *);
template void Print(ASTNodeList<ASTStmt *> &, std::ostream &, const char *);
template void Print(ASTNodeList<ASTDecl *> &, std::ostream &, const char *);
