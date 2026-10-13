// Byte-exact numeric conversions via historical glibc internal entrypoints.
// The original source may have used std::strtod/strtol macros/inlines instead.
#include "MLGameString.h"
#include "MLHashTable.h"
#include "MLHashFunGameString.h"
#include "ASTNodeList.h"
extern "C" double __strtod_internal(const char *, char **, int);
extern "C" long __strtol_internal(const char *, char **, int, int);

bool ConvVal(const MLGameString &string, float &value)
{
    value = __strtod_internal(string.c_str(), 0, 0);
    return true;
}
bool ConvVal(const MLGameString &string, int &value)
{
    value = __strtol_internal(string.c_str(), 0, 10, 0);
    return true;
}
bool ConvVal(const MLGameString &string, unsigned &value)
{
    value = __strtol_internal(string.c_str(), 0, 10, 0);
    return true;
}

// The adjacent genuine conversion definitions are needed for the original
// Canonize2 register selection in GCC 3.3.3's compilation-unit context.
namespace IR {
struct Fragment_Data;
struct Module_Data {
    int tag;
    int flag;
    ASTNodeList<Fragment_Data *> fragments;
};
}
extern MLHashTable<MLGameString, MLGameString,
                   MLHashFun<MLGameString>, 513> globalEnv;
void Canonize_Reduce(IR::Fragment_Data *);
void Canonize2_Reduce(IR::Module_Data *module)
{
    if (module->flag) return;
    ASTNodeList<IR::Fragment_Data *> &fragments = module->fragments;
    globalEnv.clear();
    for (int i = 0; i < fragments.length; ++i)
        Canonize_Reduce(fragments.entries[i]);
}
