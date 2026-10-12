// DATA-ONLY compiler emission probe for original C++ vtables.
// Synthetic static instances are NOT original VUC global objects.
// Exact acceptance applies only to named 12-byte original vtables.
// Each inheritance edge and virtual method is verified from original ELF.

struct ASTNode { virtual void apa(); };
struct ASTStmt : public ASTNode { };
struct ASTAsmStmt : public ASTStmt { };
struct ASTExpr : public ASTNode { };
struct ASTBinExpr : public ASTExpr { };
struct ASTForStmt : public ASTStmt { };
struct ASTFunCall : public ASTNode { };
struct ASTDecl : public ASTNode { };
struct ASTVarDecl : public ASTDecl { };
struct ASTFuncDecl : public ASTDecl { };
struct ASTLValExpr : public ASTExpr { };
struct FlexLexer { };
struct yyFlexLexer : public FlexLexer { };
struct ASTBlockStmt : public ASTStmt { };
struct ASTBreakStmt : public ASTStmt { };
struct ASTAssignStmt : public ASTStmt { };
struct ASTLValIDExpr : public ASTLValExpr { };
struct ASTReturnStmt : public ASTStmt { };
struct ASTFunCallExpr : public ASTExpr { };
struct ASTFunCallStmt : public ASTStmt { };
struct ASTLValMemExpr : public ASTLValExpr { };
struct ASTVarDeclStmt : public ASTStmt { };
struct ASTArrayVarDecl : public ASTVarDecl { };
struct ASTConstValExpr : public ASTExpr { };
struct ASTFunctionDecl : public ASTDecl { };
struct ASTLValArrayExpr : public ASTLValExpr { };
struct ASTDoStmt : public ASTStmt { };
struct ASTIfStmt : public ASTStmt { };
struct ASTModule : public ASTNode { };
struct ASTUnExpr : public ASTExpr { };

static ASTArrayVarDecl vuc_vtable_emission_ASTArrayVarDecl __attribute__((used));
static ASTAsmStmt vuc_vtable_emission_ASTAsmStmt __attribute__((used));
static ASTAssignStmt vuc_vtable_emission_ASTAssignStmt __attribute__((used));
static ASTBinExpr vuc_vtable_emission_ASTBinExpr __attribute__((used));
static ASTBlockStmt vuc_vtable_emission_ASTBlockStmt __attribute__((used));
static ASTBreakStmt vuc_vtable_emission_ASTBreakStmt __attribute__((used));
static ASTConstValExpr vuc_vtable_emission_ASTConstValExpr __attribute__((used));
static ASTDoStmt vuc_vtable_emission_ASTDoStmt __attribute__((used));
static ASTForStmt vuc_vtable_emission_ASTForStmt __attribute__((used));
static ASTFunCall vuc_vtable_emission_ASTFunCall __attribute__((used));
static ASTFunCallExpr vuc_vtable_emission_ASTFunCallExpr __attribute__((used));
static ASTFunCallStmt vuc_vtable_emission_ASTFunCallStmt __attribute__((used));
static ASTFuncDecl vuc_vtable_emission_ASTFuncDecl __attribute__((used));
static ASTFunctionDecl vuc_vtable_emission_ASTFunctionDecl __attribute__((used));
static ASTIfStmt vuc_vtable_emission_ASTIfStmt __attribute__((used));
static ASTLValArrayExpr vuc_vtable_emission_ASTLValArrayExpr __attribute__((used));
static ASTLValIDExpr vuc_vtable_emission_ASTLValIDExpr __attribute__((used));
static ASTLValMemExpr vuc_vtable_emission_ASTLValMemExpr __attribute__((used));
static ASTModule vuc_vtable_emission_ASTModule __attribute__((used));
static ASTReturnStmt vuc_vtable_emission_ASTReturnStmt __attribute__((used));
static ASTUnExpr vuc_vtable_emission_ASTUnExpr __attribute__((used));
static ASTVarDecl vuc_vtable_emission_ASTVarDecl __attribute__((used));
static ASTVarDeclStmt vuc_vtable_emission_ASTVarDeclStmt __attribute__((used));
