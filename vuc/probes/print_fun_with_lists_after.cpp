// Experimental ASTFunCall original stream printer.
#include "ASTNodeList.h"
#include <ostream>
struct ASTExpr {};
struct ASTLValExpr {};
struct ASTFunCall {
    int type;
    int attributes;
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};
void Print(ASTLValExpr *, std::ostream &);
template <class T>
void Print(ASTNodeList<T> &, std::ostream &, const char *);
void Print(ASTFunCall *call, std::ostream &output)
{
    Print(call->callee, output);
    output << "(";
    Print(call->arguments, output, ", ");
    output << ")";
}

void Print(ASTExpr *, std::ostream &);
void Print(ASTStmt *, std::ostream &);
void Print(ASTDecl *, std::ostream &);
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
