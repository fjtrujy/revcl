// Reconstructed VUC 2004.1 IR binary-operation reducer.
// Historical GCC 3.3.3's unlikely division-branch hint preserves the
// original multiplication/division landing alignment for BOTH real template
// specializations. This is a normal C++03 source-level prediction, not
// inline assembly, a forced register or explicit instruction padding.
namespace IR {
struct Operator_Data {
    unsigned virtual_table;
    unsigned operation;
};
}

template<class T>
bool ReduceBinOp(IR::Operator_Data *op, const T &lhs, const T &rhs, T &out)
{
    unsigned operation = op->operation;
    if (operation == 13) out = lhs + rhs;
    else if (operation == 12) out = lhs - rhs;
    else if (operation == 11) out = lhs * rhs;
    else if (__builtin_expect(operation == 10, 0)) out = lhs / rhs;
    else if (operation == 7) out = lhs == rhs;
    else if (operation == 6) out = lhs != rhs;
    else if (operation == 5) out = lhs > rhs;
    else if (operation == 4) out = lhs < rhs;
    else if (operation == 3) out = lhs >= rhs;
    else if (operation == 2) out = lhs <= rhs;
    else if (operation == 1) out = lhs && rhs;
    else if (operation == 0) out = lhs || rhs;
    else if (operation == 9) out = lhs & rhs;
    else if (operation == 8) out = lhs | rhs;
    else return true;
    return true;
}

template bool ReduceBinOp<unsigned>(IR::Operator_Data *, const unsigned &,
                                    const unsigned &, unsigned &);
template bool ReduceBinOp<int>(IR::Operator_Data *, const int &,
                               const int &, int &);
