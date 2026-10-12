// C++ RTTI type-name and single-base inheritance fragments.
// Class names and single-inheritance links are original ELF evidence.
// Member fields, virtual methods, and runtime object layouts remain
// unverified. These data-only probes emit no reconstructed functions.
#include <typeinfo>

struct ASTNode {};
struct ASTStmt : public ASTNode {};
struct ASTAsmStmt : public ASTStmt {};
struct ASTExpr : public ASTNode {};
struct ASTBinExpr : public ASTExpr {};
struct ASTForStmt : public ASTStmt {};
struct ASTFunCall : public ASTNode {};
struct ASTDecl : public ASTNode {};
struct ASTVarDecl : public ASTDecl {};
struct ASTFuncDecl : public ASTDecl {};
struct ASTLValExpr : public ASTExpr {};
struct FlexLexer {};
struct yyFlexLexer : public FlexLexer {};
struct ASTBlockStmt : public ASTStmt {};
struct ASTBreakStmt : public ASTStmt {};
struct ASTAssignStmt : public ASTStmt {};
struct ASTLValIDExpr : public ASTLValExpr {};
struct ASTReturnStmt : public ASTStmt {};
struct ASTFunCallExpr : public ASTExpr {};
struct ASTFunCallStmt : public ASTStmt {};
struct ASTLValMemExpr : public ASTLValExpr {};
struct ASTVarDeclStmt : public ASTStmt {};
struct ASTArrayVarDecl : public ASTVarDecl {};
struct ASTConstValExpr : public ASTExpr {};
struct ASTFunctionDecl : public ASTDecl {};
struct ASTLValArrayExpr : public ASTLValExpr {};
struct ASTDoStmt : public ASTStmt {};
struct ASTIfStmt : public ASTStmt {};
struct ASTModule : public ASTNode {};
struct ASTUnExpr : public ASTExpr {};

const std::type_info &vuc_rtti_name_top_00 = typeid(ASTAsmStmt);
const std::type_info &vuc_rtti_name_top_01 = typeid(ASTBinExpr);
const std::type_info &vuc_rtti_name_top_02 = typeid(ASTForStmt);
const std::type_info &vuc_rtti_name_top_03 = typeid(ASTFunCall);
const std::type_info &vuc_rtti_name_top_04 = typeid(ASTVarDecl);
const std::type_info &vuc_rtti_name_top_05 = typeid(ASTFuncDecl);
const std::type_info &vuc_rtti_name_top_06 = typeid(ASTLValExpr);
const std::type_info &vuc_rtti_name_top_07 = typeid(yyFlexLexer);
const std::type_info &vuc_rtti_name_top_08 = typeid(ASTBlockStmt);
const std::type_info &vuc_rtti_name_top_09 = typeid(ASTBreakStmt);
const std::type_info &vuc_rtti_name_top_10 = typeid(ASTAssignStmt);
const std::type_info &vuc_rtti_name_top_11 = typeid(ASTLValIDExpr);
const std::type_info &vuc_rtti_name_top_12 = typeid(ASTReturnStmt);
const std::type_info &vuc_rtti_name_top_13 = typeid(ASTFunCallExpr);
const std::type_info &vuc_rtti_name_top_14 = typeid(ASTFunCallStmt);
const std::type_info &vuc_rtti_name_top_15 = typeid(ASTLValMemExpr);
const std::type_info &vuc_rtti_name_top_16 = typeid(ASTVarDeclStmt);
const std::type_info &vuc_rtti_name_top_17 = typeid(ASTArrayVarDecl);
const std::type_info &vuc_rtti_name_top_18 = typeid(ASTConstValExpr);
const std::type_info &vuc_rtti_name_top_19 = typeid(ASTFunctionDecl);
const std::type_info &vuc_rtti_name_top_20 = typeid(ASTLValArrayExpr);
const std::type_info &vuc_rtti_name_top_21 = typeid(ASTDecl);
const std::type_info &vuc_rtti_name_top_22 = typeid(ASTExpr);
const std::type_info &vuc_rtti_name_top_23 = typeid(ASTNode);
const std::type_info &vuc_rtti_name_top_24 = typeid(ASTStmt);
const std::type_info &vuc_rtti_name_top_25 = typeid(ASTDoStmt);
const std::type_info &vuc_rtti_name_top_26 = typeid(ASTIfStmt);
const std::type_info &vuc_rtti_name_top_27 = typeid(ASTModule);
const std::type_info &vuc_rtti_name_top_28 = typeid(ASTUnExpr);
const std::type_info &vuc_rtti_name_top_29 = typeid(FlexLexer);
