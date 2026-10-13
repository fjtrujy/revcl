// Reconstructed VUC AST.cpp global initialization and return-name symbol.
// The three-pointer match stack layout is supported by the original 12-byte BSS
// object and its constructor stores; the original template name is unknown.
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
