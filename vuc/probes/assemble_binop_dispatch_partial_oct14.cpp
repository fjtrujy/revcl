// Experimental VUC 2004.1 GCC 3.3.3 AssembleBinOp dispatcher.
// This authentic C++ call/return graph gives 162/193 matched original bytes.
// The builtin expectation below is a *hypothesis* for original GCC block
// frequency; original authorship is unproven. Do not promote to exact source.
#include "../src/MLGameString.h"
namespace IR {
struct Register {
    MLGameString name;
    unsigned channels;
};
struct Expr_Data;
struct Type_Data {
    virtual void zzzzMakeRTTIWork();
    int kind;
};
}
struct InstructionList;
IR::Type_Data *GetOpType(IR::Expr_Data *);
IR::Register AssembleBinOpInt(const IR::Register &, IR::Expr_Data *, InstructionList &);
IR::Register AssembleBinOpFloat4(const IR::Register &, IR::Expr_Data *, InstructionList &);

IR::Register AssembleBinOp(const IR::Register &target,
                            IR::Expr_Data *expr,
                            InstructionList &instructions)
{
    const int code = GetOpType(expr)->kind;
    if (code == 3)
        return AssembleBinOpFloat4(target, expr, instructions);
    if (code == 4)
        return AssembleBinOpFloat4(target, expr, instructions);
    if (code == 2)
        return AssembleBinOpInt(target, expr, instructions);
    if (__builtin_expect(code == 1, 0))
        return AssembleBinOpInt(target, expr, instructions);
    return target;
}
