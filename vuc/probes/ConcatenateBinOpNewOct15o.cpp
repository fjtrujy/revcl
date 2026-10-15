// Original VUC operator text serialization, fresh C++03 reconstruction.
// Keep the unusual default empty operator text observed in the original.
#include "../src/MLGameString.h"
namespace IR {
struct Operator_Data {
    unsigned virtual_table;
    unsigned operation;
};
}

MLGameString ConcatenateBinOp(IR::Operator_Data *op, const MLGameString &lhs,
                              const MLGameString &rhs)
{
    const char *word = "";
    if (op->operation == 13) word = "+";
    else if (op->operation == 12) word = "-";
    else if (op->operation == 11) word = "*";
    else if (op->operation == 10) word = "/";
    else if (op->operation == 7) word = "==";
    else if (op->operation == 6) word = "!=";
    else if (op->operation == 5) word = ">";
    else if (op->operation == 4) word = "<";
    else if (op->operation == 3) word = ">=";
    else if (op->operation == 2) word = "<=";
    else if (op->operation == 1) word = "&&";
    else if (op->operation == 0) word = "||";
    else if (op->operation == 9) word = "&";
    else if (op->operation == 8) word = "|";
    MLTempString temporary("(%s%s%s)", lhs.c_str(), word, rhs.c_str());
    return MLGameString(temporary);
}
