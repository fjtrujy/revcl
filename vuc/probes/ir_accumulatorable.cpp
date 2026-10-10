// Candidate for VUC conditional operator-accumulator classification.
namespace IR {
struct Operand { int tag; int kind; };
struct Operator_Data { int tag; int kind; Operand *operand; };
}
bool IsAccumulatorable(IR::Operator_Data *op)
{
    if (op->kind == 11) {
        if (op->operand->kind == 3) return true;
    }
    if (op->kind == 13) {
        if (op->operand->kind == 3) return true;
    }
    if (op->kind == 12) {
        if (op->operand->kind == 3) return true;
    }
    return false;
}
