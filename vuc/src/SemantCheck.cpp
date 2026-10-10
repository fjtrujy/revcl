// Byte-exact Semant_Check entrypoint; other semantic passes unresolved.
#include "ASTNodeList.h"

class ASTModule {
public:
    char header_[0x14];
    ASTNodeList<ASTVarDecl *> vars;
    ASTNodeList<ASTDecl *> decls;
};

void Semant_Check(ASTModule *module)
{
    Check(module->vars);
    Check(module->decls);
}
