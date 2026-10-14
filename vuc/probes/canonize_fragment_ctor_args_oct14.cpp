// Candidate VUC fragment canonicalization through a normal C++ node ctor.
// Hash is a constructor argument, evaluated after operator new but before
// the two MLGameString copy constructions in the 16-byte new object.
#include "../src/MLGameString.h"
#include "../src/MLHashFunGameString.h"
#include "../src/MLHashTable.h"
#include <new>
namespace IR {
struct Stmt_Data;
struct Fragment_Data {
    void *vtable; int kind;
    MLGameString name; MLGameString output;
    void *unused; Stmt_Data *stmt;
};
}
typedef MLHashTable<MLGameString,MLGameString,MLHashFun<MLGameString>,513> Env;
extern Env globalEnv;
void Canonize_Reduce(IR::Stmt_Data *);

struct FragmentHashNode {
    MLGameString key;
    unsigned hash;
    MLGameString value;
    FragmentHashNode *next;
    FragmentHashNode(const MLGameString &name, const MLGameString &mapped, unsigned h)
        : key(name), hash(h), value(mapped), next(0) {}
};

void Canonize_Reduce(IR::Fragment_Data *fragment)
{
    if (fragment->kind == 2) {
        if (fragment->stmt) {
            globalEnv.clear();
            Canonize_Reduce(fragment->stmt);
        }
        return;
    }
    if (fragment->kind != 1) return;

    FragmentHashNode *node =
        new FragmentHashNode(fragment->name, fragment->output,
                             MLHashFun<MLGameString>().Hash(fragment->name));
    unsigned slot = node->hash % 513;
    node->next = (FragmentHashNode *)globalEnv.heads[slot];
    globalEnv.heads[slot] = (Env::Node *)node;
}
