// Original AST.cpp candidate: trivial pointer container with an inline initializer.
#include <iostream>
#include "../src/MLGameString.h"
struct MatchStack {
    void *begin;
    void *end;
    void *capacity;
    MatchStack() : begin(0), end(0), capacity(0) {}
};
MatchStack matchStack;
struct ASTReturnStmt { static MLGameString regName; };
MLGameString ASTReturnStmt::regName("_return_value", true);
