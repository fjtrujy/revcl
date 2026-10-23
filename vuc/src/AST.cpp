// Reconstructed VUC AST.cpp global initialization and return-name symbol.
// The original 12-byte BSS match stack was initially modeled as three
// pointers. Four AST printers identify the actual source-level semantics:
// signed count, signed capacity, then a pointer array of ASTNode*.
// Its exact original template/class spelling is still unknown.
#include <iostream>
#include "../src/MLGameString.h"
struct ASTNode;
struct MatchStack {
    int length;
    int capacity;
    ASTNode **entries;
    MatchStack() : length(0), capacity(0), entries(0) {}
};
MatchStack matchStack;
struct ASTReturnStmt { static MLGameString regName; };
MLGameString ASTReturnStmt::regName("_return_value", true);
