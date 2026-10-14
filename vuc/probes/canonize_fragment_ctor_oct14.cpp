// Experimental node-constructor lifetime for VUC's fragment canonicalizer.
// Same physical 16-byte node storage as the authenticated hash-node layout;
// explicit placement copies preserve exception cleanup ownership from 'new'.
#include "../src/MLGameString.h"
#include "../src/MLHashFunGameString.h"
#include "../src/MLHashTable.h"
#include <new>
namespace IR {
struct Stmt_Data;
struct Fragment_Data {
    void *vtable;
    int kind;
    MLGameString name;
    MLGameString output;
    void *unused;
    Stmt_Data *stmt;
};
}
typedef MLHashTable<MLGameString,MLGameString,MLHashFun<MLGameString>,513> Env;
extern Env globalEnv;
void Canonize_Reduce(IR::Stmt_Data *);

struct FragmentHashNode {
    char key[sizeof(MLGameString)];
    unsigned hash;
    char value[sizeof(MLGameString)];
    FragmentHashNode *next;
    FragmentHashNode(const MLGameString &input, const MLGameString &mapped)
    {
        hash = MLHashFun<MLGameString>().Hash(input);
        new (key) MLGameString(input);
        new (value) MLGameString(mapped);
    }
};

void Canonize_Reduce(IR::Fragment_Data *fragment)
{
    switch (fragment->kind) {
    case 1: {
        FragmentHashNode *node = new FragmentHashNode(fragment->name, fragment->output);
        unsigned slot = node->hash % 513;
        node->next = (FragmentHashNode *)globalEnv.heads[slot];
        globalEnv.heads[slot] = (Env::Node *)node;
        break;
    }
    case 2:
        if (fragment->stmt) {
            globalEnv.clear();
            Canonize_Reduce(fragment->stmt);
        }
        break;
    }
}
