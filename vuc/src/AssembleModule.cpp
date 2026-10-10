// Byte-exact VUC AssembleModule fragment, verified against original ELF.
#include "ASTNodeList.h"
class InstructionList;
namespace IR {
struct Fragment_Data;
struct Module_Data {
    int tag;
    int error;
    ASTNodeList<Fragment_Data *> fragments;
};
}
void Assemble(IR::Fragment_Data *frag, InstructionList &instructions);
void AssembleModule(IR::Module_Data *module, InstructionList &instructions)
{
    if (module->error) return;
    ASTNodeList<IR::Fragment_Data *> &fragments = module->fragments;
    for (int i = 0; i < fragments.length; ++i) {
        IR::Fragment_Data **entries = fragments.entries;
        Assemble(entries[i], instructions);
    }
}
