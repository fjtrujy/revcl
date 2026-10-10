// Experimental ASTFunCall original stream printer.
#include "../src/ASTNodeList.h"
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
