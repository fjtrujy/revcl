// Original-style VUC 2004.1 recursion check, fresh source reconstruction.
// Extracted ASTNodeList 12-byte layout and ASTLValExpr +32 entry ownership
// from original ELF. Keep experimental pending strict original-size matching.
#include <iostream>
#include "../src/MLGameString.h"
#include "../src/ASTNodeList.h"
struct ASTNode { void *vtable; int line; };
struct ASTExpr;
struct SymbolTableEntry {
    unsigned reserved[8];
    MLGameString name;
};
struct ASTLValExpr {
    unsigned reserved[8];
    SymbolTableEntry *entry;
};
struct ASTFunCall {
    unsigned header[2];
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};
class SymbolTable {
public:
    SymbolTableEntry *Find(const MLGameString &name,bool localOnly);
};
extern MLGameString enclosingFunID;
extern ASTNodeList<ASTNode *> matchStack;
extern char *currFileName;
extern bool error;
void Bind(ASTLValExpr *callee,SymbolTable &symbols);
template <class T> void Bind(ASTNodeList<T> &list,SymbolTable &symbols);

void Bind(ASTFunCall *call, SymbolTable &symbols)
{
    Bind(call->callee,symbols);
    Bind(call->arguments,symbols);
    if (call->callee->entry != symbols.Find(enclosingFunID,false))
        return;
    const char *functionName=call->callee->entry->name.c_str();
    error=true;
    std::cout << currFileName;
    std::cout << "(" << matchStack.entries[matchStack.length-1]->line
              << "): function '" << functionName
              << "' uses recursion! Recursion is forbidden in VUC!"
              << std::endl;
}
