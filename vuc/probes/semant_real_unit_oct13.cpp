// Original semantic analyzer functions in plausible source declaration order.
#include "../src/CheckASTFunCall.cpp"
#include <iostream>

// Source-level semantic check: compare argument count and types.
// The error-reporting policy is provisional, not a reconstruction claim.
void CheckActualParamList(const ASTNodeList<ASTExpr *> &args,
                          const std::vector<Type> &params,
                          const MLGameString &name)
{
    if (args.length != (int)params.size()) {
        std::cerr << "Incorrect argument count: " << name.c_str() << std::endl;
        return;
    }
    for (int i = 0; i < args.length; ++i) {
        Type actual = Check(args.entries[i]);
        if (actual.a != params[i].a || actual.b != params[i].b) {
            std::cerr << "Incorrect argument type: " << name.c_str() << std::endl;
            return;
        }
    }
}
#include "../src/SemantCheck.cpp"
