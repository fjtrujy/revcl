// Experimental original-length AST statement-list source: 60/119 original
// VUC bytes and three original relocation sites verified, using period GCC
// 3.3.3 with -O2. Probability hints are compiler layout hypotheses and are
// not proven to have appeared in the original C++ translation unit.
// Not accepted as byte-exact source or original ABI provenance.
#include "../src/ASTNodeList.h"
namespace IR {
class Data {
public:
    int kind;
    Data() : kind(1) {}
};
class Stmt_Data : public Data {
public:
    Stmt_Data() : Data() {}
};
class Seq_Data : public Data {
public:
    virtual ~Seq_Data();
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) : Data(), first(a), second(b) {}
};
}
IR::Stmt_Data *Translate(ASTStmt *);
IR::Stmt_Data *Translate(ASTNodeList<ASTStmt *> &statements)
{
    IR::Stmt_Data *result = 0;
    for (int i=0; __builtin_expect(i<statements.length, 1); ++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (__builtin_expect(item != 0, 0)) {
            if (__builtin_expect(result != 0, 0))
                result=(IR::Stmt_Data *)new IR::Seq_Data(result, item);
            else
                result=item;
        }
    }
    return result;
}
