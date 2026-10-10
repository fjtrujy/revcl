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
