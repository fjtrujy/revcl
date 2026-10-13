// AST.cpp source-level hypothesis: three-pointer dynamic match stack.
#include <iostream>
#include <vector>
#include "../src/MLGameString.h"
std::vector<int> matchStack;
struct ASTReturnStmt {
    static MLGameString regName;
};
MLGameString ASTReturnStmt::regName("_return_value", true);
