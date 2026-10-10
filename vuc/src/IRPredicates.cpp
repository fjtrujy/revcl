// Byte-exact VUC IsFoldable() fragment; original IR types remain provisional.
namespace IR {
struct Operator_Data {
    unsigned tag;
    int kind;
};
}

bool IsFoldable(IR::Operator_Data *op)
{
    bool found = false;
    if (op->kind == 13) found = true;
    else if (op->kind == 12) found = true;
    else if (op->kind == 11) found = true;
    else if (op->kind == 10) found = true;
    return found;
}

bool IsCommutative(IR::Operator_Data *op)
{
    if (op->kind == 13) return true;
    if (op->kind == 11) return true;
    if (op->kind == 7) return true;
    if (op->kind == 6) return true;
    if (op->kind == 1) return true;
    if (op->kind == 0) return true;
    if (op->kind == 9) return true;
    return op->kind == 8;
}
