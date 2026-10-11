// Experimental VUC IR module reduction helper.
#include "../src/MLHashTable.h"
#include "../src/MLHashFunGameString.h"
#include "../src/ASTNodeList.h"
namespace IR {
    struct Fragment_Data;
    struct Module_Data {
        int tag;
        int flag;
        ASTNodeList<Fragment_Data *> fragments;
    };
}
extern MLHashTable<MLGameString, MLGameString, MLHashFun<MLGameString>, 513> globalEnv;
void Canonize_Reduce(IR::Fragment_Data *);
void Canonize2_Reduce(IR::Module_Data *module)
{
    if (module->flag) return;
    ASTNodeList<IR::Fragment_Data *> &fragments = module->fragments;
    globalEnv.clear();
    for (int i = 0; i < fragments.length; ++i)
        Canonize_Reduce(fragments.entries[i]);
}
