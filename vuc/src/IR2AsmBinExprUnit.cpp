// VUC original-era partial source-unit reconstruction.
// Only the accepted startup callback, not the large application body, is byte-exact.
// Original IR2AsmBinExpr.cpp first function, partially reconstructed in C++03.
// The full operator table and all instruction templates remain unresolved.
#include <iostream>
#include "MLGameString.h"
namespace IR {
struct Register { MLGameString name; unsigned channels; };
struct Expr_Data {
    void *vtable;
    unsigned kind;
    Expr_Data *lhs;
    Expr_Data *rhs;
};
}
struct InstructionList { void Add(const char *format, ...); };
extern bool error;
IR::Register AssembleBinOpFloat4(const IR::Register &target,
                                  IR::Expr_Data *expr,
                                  InstructionList &instructions)
{
    IR::Register result(target);
    if (!expr || !expr->lhs || !expr->rhs) {
        error = true;
        std::cout << "Invalid floating-point binary expression";
        return result;
    }
    unsigned lhs = expr->lhs->kind;
    unsigned rhs = expr->rhs->kind;
    switch (expr->kind) {
    case 10:
        if (lhs == 5 && rhs == 5) instructions.Add("add %s", target.name.c_str());
        break;
    case 11:
        if (lhs == 5 && rhs == 5) instructions.Add("sub %s", target.name.c_str());
        break;
    case 12:
        if (lhs == 5 && rhs == 5) instructions.Add("mul %s", target.name.c_str());
        break;
    case 13:
        if (lhs == 5 && rhs == 5) instructions.Add("div %s", target.name.c_str());
        break;
    default:
        error = true;
        std::cout << "Unsupported floating-point binary operator";
    }
    return result;
}
