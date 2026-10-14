// Experimental 2004 VUC Canonize_Reduce(Fragment_Data *) skeleton.
// Not authentic original source; object field offsets and call graph are
// reconstructed from the original ELF and must pass strict comparison.
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
extern MLHashTable<MLGameString,MLGameString,MLHashFun<MLGameString>,513> globalEnv;
void Canonize_Reduce(IR::Stmt_Data *);
void Canonize_Reduce(IR::Fragment_Data *fragment)
{
    if (fragment->kind == 1) {
        typedef MLHashTable<MLGameString,MLGameString,MLHashFun<MLGameString>,513> Env;
        typedef Env::Node Node;
        unsigned hash = MLHashFun<MLGameString>().Hash(fragment->name);
        Node *node = static_cast<Node *>(::operator new(sizeof(Node)));
        new (&node->key) MLGameString(fragment->name);
        node->hash = hash;
        new (&node->value) MLGameString(fragment->output);
        unsigned bucket = hash % 513;
        node->next = globalEnv.heads[bucket];
        globalEnv.heads[bucket] = node;
    } else if (fragment->kind == 2 && fragment->stmt) {
        globalEnv.clear();
        Canonize_Reduce(fragment->stmt);
    }
}
