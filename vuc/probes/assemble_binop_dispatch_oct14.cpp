// Recover the original VUC AssembleBinOp value-return dispatcher.
// C++03 ABI: Register is an 8-byte struct containing MLGameString and mask.
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
    switch (GetOpType(expr)->kind) {
    case 1:
    case 2:
        return AssembleBinOpInt(target, expr, instructions);
    case 3:
    case 4:
        return AssembleBinOpFloat4(target, expr, instructions);
    default:
        return target;
    }
}
