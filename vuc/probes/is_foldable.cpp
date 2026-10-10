// Candidate for VUC IR operator folding predicate.
namespace IR {
struct Operator_Data {
    unsigned tag;
    int kind;
};
}
bool IsFoldable(IR::Operator_Data *op)
{
    return op->kind == 13 || op->kind == 12 ||
           op->kind == 11 || op->kind == 10;
}
