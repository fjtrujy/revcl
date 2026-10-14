// Experimental source-level VUC accumulator classifier, 74/77 verified bytes.
// Original ELF: IsAccumulatorable(IR::Operator_Data *) at 0x0805a570.
// GCC 3.3.3 20040125 and GNU as 2.15 with -O2 -frename-registers
// -falign-jumps=8 reproduce the 77-byte layout and 74 instruction bytes.
// Three remaining differences are the orientation/displacements of the
// final conditional+unconditional branch at offsets 0x49, 0x4a and 0x4c.
// Predictions are compiler-layout hypotheses, NOT proven original syntax.
namespace IR {
struct Operand { int tag; int kind; };
struct Operator_Data { int tag; int kind; Operand *operand; };
}
bool IsAccumulatorable(IR::Operator_Data *op)
{
    if (__builtin_expect(op->kind == 11, 0)) {
        if (__builtin_expect(op->operand->kind == 3, 1)) return true;
    }
    if (__builtin_expect(op->kind == 13, 0)) {
        if (op->operand->kind == 3) return true;
    }
    if (op->kind == 12) {
        if (__builtin_expect(op->operand->kind == 3, 0)) return true;
    }
    return false;
}
