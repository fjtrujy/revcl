// Source-level reconstruction of the original Printer.cpp module-printing fragment.
// Only Printer_UglyPrint is presently accepted byte-exact; the preceding
// AST function-call printer preserves genuine source-unit compiler context.
#include "../src/ASTNodeList.h"
#include <iostream>
struct ASTExpr {};
struct ASTLValExpr {};
struct ASTFunCall {
    int type;
    int attributes;
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};
struct ASTModule {
    char reserved_[0x20];
    ASTNodeList<ASTDecl *> declarations_;
};
void Print(ASTLValExpr *, std::ostream &);
template <class T> void Print(ASTNodeList<T> &, std::ostream &, const char *);
void Print(ASTFunCall *call, std::ostream &output)
{
    Print(call->callee, output);
    output << "(";
    Print(call->arguments, output, ", ");
    output << ")";
}
void Printer_UglyPrint(ASTModule *module, std::ostream &output)
{
    Print(module->declarations_, output, "\n");
}
