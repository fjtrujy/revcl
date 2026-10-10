// Empty historical VUC API hooks from canonical original ELF symbols.
struct ASTLValExpr;
class Value { public: void Set(const ASTLValExpr *); };
void Value::Set(const ASTLValExpr *) {}
class ASTNode { public: void apa(); };
void ASTNode::apa() {}
class InstructionList { public: void GenerateImage(unsigned char *); };
void InstructionList::GenerateImage(unsigned char *) {}
