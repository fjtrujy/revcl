// VUC source-unit hypothesis: original GetOpType followed by IsAccumulatorable.
#include "../src/GetOpType.cpp"
bool IsAccumulatorable(IR::Operator_Data *op)
{
    if (op->kind == 11) {
        if (op->type->kind == 3) return true;
    }
    if (op->kind == 13) {
        if (op->type->kind == 3) return true;
    }
    if (op->kind == 12) {
        if (op->type->kind == 3) return true;
    }
    return false;
}
