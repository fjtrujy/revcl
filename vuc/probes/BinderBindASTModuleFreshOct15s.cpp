// New source-level owner reconstruction for the original Binder_Bind(ASTModule*)
// at 0x0804e980. Ordinary C++03 with real Scope vector lifetime and helpers.
// Do not select until original 552-byte function and relocations authenticate.
#include <vector>
#include "../src/MLGameString.h"
#include "../src/MLHashFunGameString.h"
#include "../src/MLHashTable.h"
#include "../src/ASTNodeList.h"
struct SymbolTableEntry;
typedef MLHashTable<MLGameString,SymbolTableEntry*,MLHashFun<MLGameString>,513> Scope;
struct SymbolTable {
    std::vector<Scope> scopes;
    SymbolTable() {scopes.reserve(100);}
    void BeginScope();
    void EndScope();
    SymbolTableEntry *Find(const MLGameString&, bool);
};
struct ASTModule {
    unsigned header[2];
    ASTNodeList<MLGameString> globals;
    ASTNodeList<ASTVarDecl*> variables;
    ASTNodeList<ASTDecl*> declarations;
};
extern SymbolTable float4MemSymbols;
extern bool exportModule;
void AddFloat4Members(SymbolTable&);
template<class T>void Bind(ASTNodeList<T>&,SymbolTable&);

void Binder_Bind(ASTModule *module)
{
    float4MemSymbols.BeginScope();
    AddFloat4Members(float4MemSymbols);
    SymbolTable table;
    table.BeginScope();
    Bind(module->globals,table);
    Bind(module->variables,table);
    Bind(module->declarations,table);
    MLGameString mainName("main",false);
    if (!table.Find(mainName,false)) exportModule=true;
    table.EndScope();
    float4MemSymbols.EndScope();
}
