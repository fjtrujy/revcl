// Exact VUC 2004.1 IsAccumulatorable original 77-byte reconstruction.
// The natural negative guard keeps the original two-branch CFG at +0x49.
// Built using historical GCC 3.3.3 + GNU as 2.15, no assembly.
namespace IR {
struct Operand { int tag; int kind; };
struct Operator_Data { int tag; int kind; Operand *operand; };
}
bool IsAccumulatorable(IR::Operator_Data *op)
{
    if (__builtin_expect(op->kind == 11, 0)) {
        if (op->operand->kind != 3) goto next_case11;
        return true;
    }
    next_case11:
    if (__builtin_expect(op->kind == 13, 0)) {
        if (op->operand->kind == 3) return true;
    }
    if (op->kind == 12) {
        if (__builtin_expect(op->operand->kind == 3, 0)) return true;
    }
    return false;
}
