// Candidate Printer_UglyPrint: delegation to the original AST-list printer.
#include <ostream>
struct ASTDecl;
template <class T> class ASTNodeList {
public:
    void *begin_, *end_;
};
class ASTModule {
public:
    char reserved_[0x20];
    ASTNodeList<ASTDecl *> declarations_;
};
template <class T>
void Print(ASTNodeList<T> &, std::ostream &, const char *);

void Printer_UglyPrint(ASTModule *module, std::ostream &output)
{
    Print(module->declarations_, output, "\n");
}
