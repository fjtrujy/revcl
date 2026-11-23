# VUC 2004.1 — original-function byte-match inventory

Generated from the canonical VUC ELF symbol inventory, accepted exact
function manifests, and a successful historical GCC 3.3.3
relocation/literal-verified partial-checkpoint run.

**Function scores do not measure the entire linked ELF.** No runnable
or complete byte-exact VUC executable has been reconstructed yet.

## Totals

- Named text symbols: **298**.
- **211** exact functions, **17,081** verified exact instruction bytes.
- Exact functions occupy **17,081/163,241** original bytes across all named function extents (**10.5%**). This is a *strict accepted-function extent ratio*, NOT whole-ELF byte matching.
- **33** selected experimental functions, **32** eligible original-length partials matching **6,857/10,776** bytes
  within those partial functions only.
- **54** unresolved symbols, including **6** zero-sized symbols.
- Whole executable: **not yet buildable; no overall byte-match score**.

## Experimental byte matches (sorted by missing instruction bytes)

| Original address | Function | Original size | Verified bytes | Missing bytes |
| --- | --- | ---: | ---: | ---: |
| 0x08051980 | `__tcf_1` | 46 | 45/46 | 1 |
| 0x0805b770 | `Print(ASTFunCall*, std::ostream&)` | 98 | 96/98 | 2 |
| 0x0806fa80 | `script_error(char const*)` | 161 | 155/161 | 6 |
| 0x0806a550 | `std::vector<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>, std::allocator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>>>::_M_insert_aux(__gnu_cxx::__normal_iterator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*, std::vector<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>, std::allocator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>>>>, MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513> const&)` | 746 | 733/746 | 13 |
| 0x0806e9b0 | `Value::operator+(Value const&) const` | 219 | 203/219 | 16 |
| 0x0805a3f0 | `AssembleBinOp(IR::Register const&, IR::Expr_Data*, InstructionList&)` | 193 | 162/193 | 31 |
| 0x0804ee10 | `std::vector<Type, std::allocator<Type>>::_M_insert_aux(__gnu_cxx::__normal_iterator<Type*, std::vector<Type, std::allocator<Type>>>, Type const&)` | 525 | 481/525 | 44 |
| 0x08049c70 | `AsmOutput_PrintInstruction(Instruction*, std::basic_ofstream<char, std::char_traits<char>>&)` | 382 | 334/382 | 48 |
| 0x0806ec50 | `Value::operator/(Value const&) const` | 235 | 178/235 | 57 |
| 0x0804ec50 | `std::vector<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>, std::allocator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>>>::reserve(unsigned int)` | 241 | 183/241 | 58 |
| 0x0806ada0 | `Translate(ASTNodeList<ASTStmt*>&)` | 119 | 60/119 | 59 |
| 0x0806f6f0 | `Value::ToString() const` | 221 | 159/221 | 62 |
| 0x0806f8b0 | `Value::TypeConvertForce(TypeID, Value const&, Value const&, Value&, Value&)` | 257 | 193/257 | 64 |
| 0x080511e0 | `GetNextWord(char const*)` | 135 | 50/135 | 85 |
| 0x0804f420 | `MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>::MLHashTable(MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513> const&)` | 166 | 80/166 | 86 |
| 0x0804ca30 | `TryOpenFile(char const*)` | 754 | 650/754 | 104 |
| 0x0804f2a0 | `operator>>(std::istream&, Type&)` | 369 | 264/369 | 105 |
| 0x080611c0 | `yyFlexLexer::yy_get_next_buffer()` | 550 | 430/550 | 120 |
| 0x080507c0 | `ConcatenateBinOp(IR::Operator_Data*, MLGameString const&, MLGameString const&)` | 346 | 223/346 | 123 |
| 0x0806e0a0 | `ASTNodeList<IR::Fragment_Data*> TranslateFragList<ASTVarDecl*>(ASTNodeList<ASTVarDecl*>&)` | 240 | 110/240 | 130 |
| 0x0806e190 | `ASTNodeList<IR::Fragment_Data*> TranslateFragList<ASTDecl*>(ASTNodeList<ASTDecl*>&)` | 240 | 110/240 | 130 |
| 0x0806f340 | `Value::operator&&(Value const&) const` | 325 | 192/325 | 133 |
| 0x0806ae20 | `Translate(ASTNodeList<ASTExpr*>&)` | 256 | 112/256 | 144 |
| 0x08050460 | `Canonize_Reduce(IR::Fragment_Data*)` | 224 | 78/224 | 146 |
| 0x0805abc0 | `ListFile::WriteToFile(char const*)` | 1071 | 875/1071 | 196 |
| 0x08051b30 | `BroadcastMove(IR::Register const&, IR::Register const&, InstructionList&)` | 416 | 191/416 | 225 |
| 0x0806e5b0 | `ASTNodeList<IR::Fragment_Data*> TranslateFragList<MLGameString>(ASTNodeList<MLGameString>&)` | 306 | 75/306 | 231 |
| 0x0804ffb0 | `bool Canonize_TryReduceExpr<unsigned int>(IR::Expr_Data*, unsigned int&)` | 359 | 116/359 | 243 |
| 0x08050120 | `bool Canonize_TryReduceExpr<int>(IR::Expr_Data*, int&)` | 359 | 116/359 | 243 |
| 0x0806a9b0 | `Translate(Type const&, unsigned int, int)` | 352 | 64/352 | 288 |
| 0x080502c0 | `Canonize_FoldExpr(IR::Expr_Data*)` | 410 | 72/410 | 338 |
| 0x0806e7d0 | `Value::Set(ASTConstValExpr const*)` | 455 | 67/455 | 388 |
| 0x0806a330 | `SymbolTable::Enter(MLGameString const&, SymbolTableEntry*)` | 185 | UNVERIFIED | — |

## Unresolved function symbols

| Original address | Function | Original size | Verified bytes |
| --- | --- | ---: | ---: |
| 0x0806a840 | `__gnu_cxx::__normal_iterator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*, std::vector<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>, std::allocator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>>>> std::__uninitialized_copy_aux<__gnu_cxx::__normal_iterator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*, std::vector<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>, std::allocator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>>>>, __gnu_cxx::__normal_iterator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*, std::vector<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>, std::allocator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>>>>>(__gnu_cxx::__normal_iterator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*, std::vector<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>, std::allocator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>>>>, __gnu_cxx::__normal_iterator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*, std::vector<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>, std::allocator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>>>>, __gnu_cxx::__normal_iterator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*, std::vector<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>, std::allocator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>>>>, __false_type)` | 149 | — |
| 0x0806bdd0 | `TranslateConst(MLGameString const&, Type const&)` | 290 | — |
| 0x0804aad0 | `Bind(ASTFunCall*, SymbolTable&)` | 304 | — |
| 0x08050d30 | `BindInlineAsm(ASTAsmStmt*, SymbolTable&)` | 450 | — |
| 0x08054550 | `Assemble(IR::Fragment_Data*, InstructionList&)` | 457 | — |
| 0x0804e980 | `Binder_Bind(ASTModule*)` | 552 | — |
| 0x08050920 | `Canonize_Reduce(IR::Stmt_Data*)` | 608 | — |
| 0x0806ab10 | `TranslateCallInfo(ASTFunCall*)` | 644 | — |
| 0x0806e310 | `Translate(ASTBinExpr::Operator, Type, unsigned int, unsigned int)` | 663 | — |
| 0x0805b040 | `std::vector<std::string, std::allocator<std::string>>::_M_insert_aux(__gnu_cxx::__normal_iterator<std::string*, std::vector<std::string, std::allocator<std::string>>>, std::string const&)` | 668 | — |
| 0x0806dd90 | `Translate_Translate(ASTModule*)` | 673 | — |
| 0x08050f00 | `GenInlineAsm(MLGameString&, std::vector<AsmVariable, std::allocator<AsmVariable>>&, InstructionList&)` | 711 | — |
| 0x0805c4f0 | `Print(ASTLValExpr*, std::ostream&)` | 765 | — |
| 0x0805b3e0 | `ParseArguments(int, char**)` | 777 | — |
| 0x0805c7f0 | `Print(ASTDecl*, std::ostream&)` | 844 | — |
| 0x0804bbf0 | `Bind(ASTLValExpr*, SymbolTable&)` | 911 | — |
| 0x080541b0 | `AssembleFunCall(IR::FunCallInfo const&, IR::Register const&, MLGameString const&, ASTNodeList<IR::Expr_Data*>&, InstructionList&)` | 919 | — |
| 0x0804f690 | `Canonize_Reduce(IR::Expr_Data*)` | 955 | — |
| 0x0804b830 | `Bind(ASTExpr*, SymbolTable&)` | 959 | — |
| 0x0805c0a0 | `Print(ASTExpr*, std::ostream&)` | 1089 | — |
| 0x0806ce20 | `Translate(ASTDecl*)` | 1170 | — |
| 0x08069be0 | `Check(ASTDecl*)` | 1183 | — |
| 0x0805a610 | `ListFile::Generate(char const*)` | 1206 | — |
| 0x0804fa50 | `Canonize_Rotate(IR::Expr_Data*)` | 1277 | — |
| 0x08051cd0 | `AssembleExprReg(IR::Register const&, IR::Expr_Data*, InstructionList&, int)` | 1315 | — |
| 0x0806c8d0 | `Translate(ASTLValExpr*, unsigned int, int, Type const*)` | 1351 | — |
| 0x08054720 | `Assemble(IR::Stmt_Data*, InstructionList&)` | 1654 | — |
| 0x08060810 | `yyFlexLexer::yylex()` | 1698 | — |
| 0x0806fb30 | `ParseFile()` | 2059 | — |
| 0x08049e70 | `AsmOutput_Emit(InstructionList&)` | 2168 | — |
| 0x0805b7e0 | `Print(ASTStmt*, std::ostream&)` | 2231 | — |
| 0x0806bf00 | `Translate(ASTExpr*, unsigned int, int)` | 2501 | — |
| 0x08061a20 | `CheckActualParamList(ASTNodeList<ASTExpr*> const&, std::vector<Type, std::allocator<Type>> const&, MLGameString const&)` | 2510 | — |
| 0x0806d2c0 | `TranslateFrag(ASTDecl*)` | 2659 | — |
| 0x0804cd30 | `Bind(MLGameString&, SymbolTable&)` | 2691 | — |
| 0x0804bf80 | `Bind(ASTDecl*, SymbolTable&)` | 2726 | — |
| 0x0804ac00 | `Bind(ASTStmt*, SymbolTable&)` | 3119 | — |
| 0x0806af20 | `Translate(ASTStmt*)` | 3749 | — |
| 0x0804d7c0 | `AddFloat4Members(SymbolTable&)` | 4538 | — |
| 0x08070340 | `ExportSymbols(ASTModule*)` | 4583 | — |
| 0x08054da0 | `AssembleAssign(IR::Stmt_Data*, InstructionList&)` | 5110 | — |
| 0x080687a0 | `Check(ASTLValExpr*)` | 5174 | — |
| 0x08062470 | `Check(ASTStmt*)` | 6119 | — |
| 0x08058640 | `AssembleBinOpInt(IR::Register const&, IR::Expr_Data*, InstructionList&)` | 7588 | — |
| 0x08052200 | `Assemble(IR::Register const&, IR::Expr_Data*, InstructionList&)` | 7990 | — |
| 0x08056210 | `AssembleBinOpFloat4(IR::Register const&, IR::Expr_Data*, InstructionList&)` | 9258 | — |
| 0x0805cd30 | `script_parse()` | 14922 | — |
| 0x08063c60 | `Check(ASTExpr*)` | 19262 | — |
| 0x08049a20 | `_start` | 0 | — |
| 0x08049a44 | `call_gmon_start` | 0 | — |
| 0x08049a70 | `__do_global_dtors_aux` | 0 | — |
| 0x08049ab0 | `frame_dummy` | 0 | — |
| 0x08072020 | `__i686.get_pc_thunk.bx` | 0 | — |
| 0x08072030 | `__do_global_ctors_aux` | 0 | — |

## Accepted byte-exact function symbols

| Original address | Function | Original size | Verified bytes |
| --- | --- | ---: | ---: |
| 0x08049af0 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x08049b40 | `AsmOutput_WritePrologue(std::basic_ofstream<char, std::char_traits<char>>&)` | 30 | 30/30 |
| 0x08049b60 | `AsmOutput_WriteEpilogue(std::basic_ofstream<char, std::char_traits<char>>&)` | 30 | 30/30 |
| 0x08049b80 | `AlignString(char const*, int)` | 145 | 145/145 |
| 0x08049c20 | `AlignConstant(char const*)` | 69 | 69/69 |
| 0x08049df0 | `AsmOutput_TabConstants(InstructionList&)` | 127 | 127/127 |
| 0x0804a6f0 | `__tcf_0` | 16 | 16/16 |
| 0x0804a700 | `_GLOBAL__I_constantsTabStop` | 31 | 31/31 |
| 0x0804a720 | `__static_initialization_and_destruction_0(int, int)` | 145 | 145/145 |
| 0x0804a7c0 | `__tcf_0` | 16 | 16/16 |
| 0x0804a7d0 | `_GLOBAL__I_matchStack` | 31 | 31/31 |
| 0x0804a7f0 | `__static_initialization_and_destruction_0(int, int)` | 138 | 138/138 |
| 0x0804a880 | `ASTNode::operator new(unsigned int)` | 86 | 86/86 |
| 0x0804a8e0 | `__tcf_0` | 16 | 16/16 |
| 0x0804a8f0 | `__tcf_1` | 46 | 46/46 |
| 0x0804a920 | `_GLOBAL__I__ZN7ASTNode3memE` | 31 | 31/31 |
| 0x0804a940 | `__static_initialization_and_destruction_0(int, int)` | 393 | 393/393 |
| 0x0804ebb0 | `__tcf_0` | 16 | 16/16 |
| 0x0804ebc0 | `__tcf_1` | 107 | 107/107 |
| 0x0804ec30 | `_GLOBAL__I_exportModule` | 31 | 31/31 |
| 0x0804ed50 | `void Bind<ASTExpr*>(ASTNodeList<ASTExpr*>&, SymbolTable&)` | 57 | 57/57 |
| 0x0804ed90 | `void Bind<ASTStmt*>(ASTNodeList<ASTStmt*>&, SymbolTable&)` | 57 | 57/57 |
| 0x0804edd0 | `void Bind<ASTVarDecl*>(ASTNodeList<ASTVarDecl*>&, SymbolTable&)` | 57 | 57/57 |
| 0x0804f020 | `MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>::clear()` | 64 | 64/64 |
| 0x0804f060 | `void Bind<MLGameString>(ASTNodeList<MLGameString>&, SymbolTable&)` | 57 | 57/57 |
| 0x0804f0a0 | `void Bind<ASTDecl*>(ASTNodeList<ASTDecl*>&, SymbolTable&)` | 57 | 57/57 |
| 0x0804f0e0 | `Type* std::__uninitialized_copy_aux<__gnu_cxx::__normal_iterator<Type const*, std::vector<Type, std::allocator<Type>>>, Type*>(__gnu_cxx::__normal_iterator<Type const*, std::vector<Type, std::allocator<Type>>>, __gnu_cxx::__normal_iterator<Type const*, std::vector<Type, std::allocator<Type>>>, Type*, __false_type)` | 79 | 79/79 |
| 0x0804f130 | `MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>* std::vector<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>, std::allocator<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>>>::_M_allocate_and_copy<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*>(unsigned int, MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*, MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*)` | 143 | 143/143 |
| 0x0804f1c0 | `__gnu_cxx::__normal_iterator<Type*, std::vector<Type, std::allocator<Type>>> std::__uninitialized_copy_aux<__gnu_cxx::__normal_iterator<Type*, std::vector<Type, std::allocator<Type>>>, __gnu_cxx::__normal_iterator<Type*, std::vector<Type, std::allocator<Type>>>>(__gnu_cxx::__normal_iterator<Type*, std::vector<Type, std::allocator<Type>>>, __gnu_cxx::__normal_iterator<Type*, std::vector<Type, std::allocator<Type>>>, __gnu_cxx::__normal_iterator<Type*, std::vector<Type, std::allocator<Type>>>, __false_type)` | 86 | 86/86 |
| 0x0804f220 | `MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>* std::__uninitialized_copy_aux<MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*, MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*>(MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*, MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*, MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>*, __false_type)` | 120 | 120/120 |
| 0x0804f4d0 | `__static_initialization_and_destruction_0(int, int)` | 193 | 193/193 |
| 0x0804f5a0 | `IsFoldable(IR::Operator_Data*)` | 41 | 41/41 |
| 0x0804f5d0 | `ConvVal(MLGameString const&, float&)` | 51 | 51/51 |
| 0x0804f610 | `ConvVal(MLGameString const&, int&)` | 60 | 60/60 |
| 0x0804f650 | `ConvVal(MLGameString const&, unsigned int&)` | 60 | 60/60 |
| 0x0804ff50 | `Canonize2_Reduce(IR::Module_Data*)` | 85 | 85/85 |
| 0x08050290 | `__tcf_0` | 16 | 16/16 |
| 0x080502a0 | `__tcf_1` | 16 | 16/16 |
| 0x080502b0 | `__tcf_2` | 16 | 16/16 |
| 0x08050540 | `bool ReduceBinOp<unsigned int>(IR::Operator_Data*, unsigned int const&, unsigned int const&, unsigned int&)` | 315 | 315/315 |
| 0x08050680 | `bool ReduceBinOp<int>(IR::Operator_Data*, int const&, int const&, int&)` | 315 | 315/315 |
| 0x08050b80 | `_GLOBAL__I__Z10IsFoldablePN2IR13Operator_DataE` | 31 | 31/31 |
| 0x08050ba0 | `MLHashTable<MLGameString, MLGameString, MLHashFun<MLGameString>, 513>::FindPtr(MLGameString const&)` | 114 | 114/114 |
| 0x08050c20 | `IR::Type_Data::zzzzMakeRTTIWork()` | 5 | 5/5 |
| 0x08050c30 | `IR::Operator_Data::zzzzMakeRTTIWork()` | 5 | 5/5 |
| 0x08050c40 | `IR::Expr_Data::zzzzMakeRTTIWork()` | 5 | 5/5 |
| 0x08050c50 | `IsCommutative(IR::Operator_Data*)` | 66 | 66/66 |
| 0x08050ca0 | `MLHashTable<MLGameString, MLGameString, MLHashFun<MLGameString>, 513>::clear()` | 64 | 64/64 |
| 0x08050ce0 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x080511d0 | `__tcf_0` | 16 | 16/16 |
| 0x08051270 | `_GLOBAL__I__Z13BindInlineAsmP10ASTAsmStmtR11SymbolTable` | 31 | 31/31 |
| 0x08051290 | `std::vector<AsmVariable, std::allocator<AsmVariable>>::_M_insert_aux(__gnu_cxx::__normal_iterator<AsmVariable*, std::vector<AsmVariable, std::allocator<AsmVariable>>>, AsmVariable const&)` | 444 | 444/444 |
| 0x08051450 | `__gnu_cxx::__normal_iterator<AsmVariable*, std::vector<AsmVariable, std::allocator<AsmVariable>>> std::__uninitialized_copy_aux<__gnu_cxx::__normal_iterator<AsmVariable*, std::vector<AsmVariable, std::allocator<AsmVariable>>>, __gnu_cxx::__normal_iterator<AsmVariable*, std::vector<AsmVariable, std::allocator<AsmVariable>>>>(__gnu_cxx::__normal_iterator<AsmVariable*, std::vector<AsmVariable, std::allocator<AsmVariable>>>, __gnu_cxx::__normal_iterator<AsmVariable*, std::vector<AsmVariable, std::allocator<AsmVariable>>>, __gnu_cxx::__normal_iterator<AsmVariable*, std::vector<AsmVariable, std::allocator<AsmVariable>>>, __false_type)` | 74 | 74/74 |
| 0x080514a0 | `__static_initialization_and_destruction_0(int, int)` | 138 | 138/138 |
| 0x08051530 | `InstructionList::Add(Instruction*)` | 55 | 55/55 |
| 0x08051570 | `InstructionList::Add(char const*, ...)` | 146 | 146/146 |
| 0x08051610 | `InstructionList::AddB(char const*, ...)` | 146 | 146/146 |
| 0x080516b0 | `InstructionList::AddL(MLGameString const&)` | 107 | 107/107 |
| 0x08051720 | `InstructionList::AddLineComment(int)` | 162 | 162/162 |
| 0x080517d0 | `InstructionList::GenerateImage(unsigned char*)` | 5 | 5/5 |
| 0x080517e0 | `InstructionList::GenLabel(char const*)` | 85 | 85/85 |
| 0x08051840 | `InstructionList::GenTmp(char const*)` | 85 | 85/85 |
| 0x080518a0 | `InstructionList::Print(std::ostream&)` | 87 | 87/87 |
| 0x08051900 | `Instruction::ToString()` | 9 | 9/9 |
| 0x08051910 | `Instruction::operator new(unsigned int)` | 86 | 86/86 |
| 0x08051970 | `__tcf_0` | 16 | 16/16 |
| 0x080519b0 | `_GLOBAL__I__ZN11Instruction9instrDataE` | 31 | 31/31 |
| 0x080519d0 | `std::vector<Instruction*, std::allocator<Instruction*>>::_M_insert_aux(__gnu_cxx::__normal_iterator<Instruction**, std::vector<Instruction*, std::allocator<Instruction*>>>, Instruction* const&)` | 265 | 265/265 |
| 0x08051ae0 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x08054140 | `AssembleModule(IR::Module_Data*, InstructionList&)` | 89 | 89/89 |
| 0x080541a0 | `__tcf_0` | 16 | 16/16 |
| 0x080561a0 | `_GLOBAL__I__Z13BroadcastMoveRKN2IR8RegisterES2_R15InstructionList` | 31 | 31/31 |
| 0x080561c0 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x0805a4c0 | `__tcf_0` | 16 | 16/16 |
| 0x0805a4d0 | `_GLOBAL__I__Z19AssembleBinOpFloat4RKN2IR8RegisterEPNS_9Expr_DataER15InstructionList` | 31 | 31/31 |
| 0x0805a4f0 | `GetOpType(IR::Expr_Data*)` | 117 | 117/117 |
| 0x0805a570 | `IsAccumulatorable(IR::Operator_Data*)` | 77 | 77/77 |
| 0x0805a5c0 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x0805aad0 | `ListFile::PrintLine(int, std::basic_ofstream<char, std::char_traits<char>>&)` | 133 | 133/133 |
| 0x0805ab60 | `ListFile::Print(int, int, std::basic_ofstream<char, std::char_traits<char>>&)` | 88 | 88/88 |
| 0x0805aff0 | `ListFile::GetNumLines()` | 18 | 18/18 |
| 0x0805b010 | `__tcf_0` | 16 | 16/16 |
| 0x0805b020 | `_GLOBAL__I__ZN8ListFile8GenerateEPKc` | 31 | 31/31 |
| 0x0805b2e0 | `__gnu_cxx::__normal_iterator<std::string*, std::vector<std::string, std::allocator<std::string>>> std::__uninitialized_copy_aux<__gnu_cxx::__normal_iterator<std::string*, std::vector<std::string, std::allocator<std::string>>>, __gnu_cxx::__normal_iterator<std::string*, std::vector<std::string, std::allocator<std::string>>>>(__gnu_cxx::__normal_iterator<std::string*, std::vector<std::string, std::allocator<std::string>>>, __gnu_cxx::__normal_iterator<std::string*, std::vector<std::string, std::allocator<std::string>>>, __gnu_cxx::__normal_iterator<std::string*, std::vector<std::string, std::allocator<std::string>>>, __false_type)` | 161 | 161/161 |
| 0x0805b390 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x0805b6f0 | `__tcf_0` | 16 | 16/16 |
| 0x0805b700 | `_GLOBAL__I_inputFileName` | 31 | 31/31 |
| 0x0805b720 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x0805cb40 | `Printer_UglyPrint(ASTModule*, std::ostream&)` | 40 | 40/40 |
| 0x0805cb70 | `__tcf_0` | 16 | 16/16 |
| 0x0805cb80 | `_GLOBAL__I__Z5PrintP10ASTFunCallRSo` | 31 | 31/31 |
| 0x0805cba0 | `void Print<ASTExpr*>(ASTNodeList<ASTExpr*>&, std::ostream&, char const*)` | 80 | 80/80 |
| 0x0805cbf0 | `void Print<ASTStmt*>(ASTNodeList<ASTStmt*>&, std::ostream&, char const*)` | 80 | 80/80 |
| 0x0805cc40 | `void Print<ASTVarDecl*>(ASTNodeList<ASTVarDecl*>&, std::ostream&, char const*)` | 80 | 80/80 |
| 0x0805cc90 | `void Print<ASTDecl*>(ASTNodeList<ASTDecl*>&, std::ostream&, char const*)` | 80 | 80/80 |
| 0x0805cce0 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x08060780 | `__tcf_0` | 16 | 16/16 |
| 0x08060790 | `_GLOBAL__I_script_debug` | 31 | 31/31 |
| 0x080607b0 | `ASTNode::apa()` | 5 | 5/5 |
| 0x080607c0 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x08060ec0 | `yyFlexLexer::yyFlexLexer(std::istream*, std::ostream*)` | 138 | 138/138 |
| 0x08060f50 | `yyFlexLexer::yyFlexLexer(std::istream*, std::ostream*)` | 138 | 138/138 |
| 0x08060fe0 | `yyFlexLexer::~yyFlexLexer()` | 78 | 78/78 |
| 0x08061030 | `yyFlexLexer::~yyFlexLexer()` | 78 | 78/78 |
| 0x08061080 | `yyFlexLexer::~yyFlexLexer()` | 78 | 78/78 |
| 0x080610d0 | `yyFlexLexer::switch_streams(std::istream*, std::ostream*)` | 96 | 96/96 |
| 0x08061130 | `yyFlexLexer::LexerInput(char*, int)` | 97 | 97/97 |
| 0x080611a0 | `yyFlexLexer::LexerOutput(char const*, int)` | 18 | 18/18 |
| 0x080613f0 | `yyFlexLexer::yy_get_previous_state()` | 205 | 205/205 |
| 0x080614c0 | `yyFlexLexer::yy_try_NUL_trans(int)` | 161 | 161/161 |
| 0x08061570 | `yyFlexLexer::yyunput(int, char*)` | 167 | 167/167 |
| 0x08061620 | `yyFlexLexer::yyinput()` | 170 | 170/170 |
| 0x080616d0 | `yyFlexLexer::yyrestart(std::istream*)` | 93 | 93/93 |
| 0x08061730 | `yyFlexLexer::yy_switch_to_buffer(yy_buffer_state*)` | 75 | 75/75 |
| 0x08061780 | `yyFlexLexer::yy_load_buffer_state()` | 39 | 39/39 |
| 0x080617b0 | `yyFlexLexer::yy_create_buffer(std::istream*, int)` | 139 | 139/139 |
| 0x08061840 | `yyFlexLexer::yy_delete_buffer(yy_buffer_state*)` | 87 | 87/87 |
| 0x080618a0 | `yyFlexLexer::yy_init_buffer(yy_buffer_state*, std::istream*)` | 53 | 53/53 |
| 0x080618e0 | `yyFlexLexer::yy_flush_buffer(yy_buffer_state*)` | 80 | 80/80 |
| 0x08061930 | `yyFlexLexer::LexerError(char const*)` | 54 | 54/54 |
| 0x08061970 | `__tcf_0` | 16 | 16/16 |
| 0x08061980 | `_GLOBAL__I__ZN11yyFlexLexer5yylexEv` | 31 | 31/31 |
| 0x080619a0 | `FlexLexer::~FlexLexer()` | 14 | 14/14 |
| 0x080619b0 | `FlexLexer::~FlexLexer()` | 18 | 18/18 |
| 0x080619d0 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x080623f0 | `Check(ASTFunCall*)` | 118 | 118/118 |
| 0x0806a080 | `Semant_Check(ASTModule*)` | 40 | 40/40 |
| 0x0806a0b0 | `__tcf_0` | 16 | 16/16 |
| 0x0806a0c0 | `_GLOBAL__I__Z20CheckActualParamListRK11ASTNodeListIP7ASTExprERKSt6vectorI4TypeSaIS6_EERK12MLGameString` | 31 | 31/31 |
| 0x0806a0e0 | `void Check<ASTVarDecl*>(ASTNodeList<ASTVarDecl*>&)` | 45 | 45/45 |
| 0x0806a110 | `void Check<ASTExpr*>(ASTNodeList<ASTExpr*>&)` | 66 | 66/66 |
| 0x0806a160 | `void Check<ASTStmt*>(ASTNodeList<ASTStmt*>&)` | 45 | 45/45 |
| 0x0806a190 | `void Check<ASTDecl*>(ASTNodeList<ASTDecl*>&)` | 45 | 45/45 |
| 0x0806a1c0 | `Type::IsAssignable(Type const&, bool) const` | 151 | 151/151 |
| 0x0806a260 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x0806a2b0 | `__tcf_0` | 16 | 16/16 |
| 0x0806a2c0 | `_GLOBAL__I_StdAfx.cppxtsrAa` | 31 | 31/31 |
| 0x0806a2e0 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x0806a3f0 | `SymbolTable::Find(MLGameString const&, bool)` | 115 | 115/115 |
| 0x0806a470 | `SymbolTable::BeginScope()` | 140 | 140/140 |
| 0x0806a500 | `SymbolTable::EndScope()` | 27 | 27/27 |
| 0x0806a520 | `__tcf_0` | 16 | 16/16 |
| 0x0806a530 | `_GLOBAL__I__ZN11SymbolTable5EnterERK12MLGameStringP16SymbolTableEntry` | 31 | 31/31 |
| 0x0806a8e0 | `MLHashTable<MLGameString, SymbolTableEntry*, MLHashFun<MLGameString>, 513>::FindCopy(MLGameString const&)` | 114 | 114/114 |
| 0x0806a960 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x0806dd30 | `TranslateFrag(MLGameString const&)` | 87 | 87/87 |
| 0x0806e040 | `__tcf_0` | 16 | 16/16 |
| 0x0806e050 | `_GLOBAL__I__Z9TranslateRK4Typeji` | 31 | 31/31 |
| 0x0806e070 | `void Translate<ASTVarDecl*>(ASTNodeList<ASTVarDecl*>&)` | 45 | 45/45 |
| 0x0806e280 | `AsmVariable* std::__uninitialized_copy_aux<__gnu_cxx::__normal_iterator<AsmVariable const*, std::vector<AsmVariable, std::allocator<AsmVariable>>>, AsmVariable*>(__gnu_cxx::__normal_iterator<AsmVariable const*, std::vector<AsmVariable, std::allocator<AsmVariable>>>, __gnu_cxx::__normal_iterator<AsmVariable const*, std::vector<AsmVariable, std::allocator<AsmVariable>>>, AsmVariable*, __false_type)` | 67 | 67/67 |
| 0x0806e2d0 | `IR::UnOperator_Data::zzzzMakeRTTIWork()` | 5 | 5/5 |
| 0x0806e2e0 | `IR::Stmt_Data::zzzzMakeRTTIWork()` | 5 | 5/5 |
| 0x0806e2f0 | `IR::Fragment_Data::zzzzMakeRTTIWork()` | 5 | 5/5 |
| 0x0806e300 | `IR::Module_Data::zzzzMakeRTTIWork()` | 5 | 5/5 |
| 0x0806e6f0 | `__static_initialization_and_destruction_0(int, int)` | 66 | 66/66 |
| 0x0806e740 | `Value::Value()` | 44 | 44/44 |
| 0x0806e770 | `Value::Value()` | 44 | 44/44 |
| 0x0806e7a0 | `Value::Set(ASTExpr const*)` | 42 | 42/42 |
| 0x0806e9a0 | `Value::Set(ASTLValExpr const*)` | 5 | 5/5 |
| 0x0806ea90 | `Value::operator-(Value const&) const` | 221 | 221/221 |
| 0x0806eb70 | `Value::operator*(Value const&) const` | 222 | 222/222 |
| 0x0806ed40 | `Value::operator==(Value const&) const` | 254 | 254/254 |
| 0x0806ee40 | `Value::operator!=(Value const&) const` | 254 | 254/254 |
| 0x0806ef40 | `Value::operator>(Value const&) const` | 254 | 254/254 |
| 0x0806f040 | `Value::operator<(Value const&) const` | 254 | 254/254 |
| 0x0806f140 | `Value::operator>=(Value const&) const` | 254 | 254/254 |
| 0x0806f240 | `Value::operator<=(Value const&) const` | 254 | 254/254 |
| 0x0806f490 | `Value::operator||(Value const&) const` | 309 | 309/309 |
| 0x0806f5d0 | `Value::operator&(Value const&) const` | 143 | 143/143 |
| 0x0806f660 | `Value::operator|(Value const&) const` | 143 | 143/143 |
| 0x0806f7d0 | `Value::TypeConvert(Value const&, Value const&, Value&, Value&)` | 220 | 220/220 |
| 0x0806f9c0 | `__tcf_0` | 16 | 16/16 |
| 0x0806f9d0 | `_GLOBAL__I__ZN5ValueC2Ev` | 31 | 31/31 |
| 0x0806f9f0 | `__static_initialization_and_destruction_0(int, int)` | 88 | 88/88 |
| 0x0806fa50 | `script_lex()` | 39 | 39/39 |
| 0x08071530 | `main` | 63 | 63/63 |
| 0x08071570 | `__tcf_0` | 16 | 16/16 |
| 0x08071580 | `_GLOBAL__I_currFileName` | 31 | 31/31 |
| 0x080715a0 | `MLHashTable<int, int, MLHashFun<int>, 513>::clear()` | 64 | 64/64 |
| 0x080715e0 | `MLGameStringData::MLGameStringData(char const*, bool)` | 111 | 111/111 |
| 0x08071650 | `MLGameStringData::MLGameStringData(char const*, bool)` | 111 | 111/111 |
| 0x080716c0 | `MLGameString::MLGameString()` | 14 | 14/14 |
| 0x080716d0 | `MLGameString::MLGameString()` | 14 | 14/14 |
| 0x080716e0 | `MLGameString::MLGameString(char const*, bool)` | 52 | 52/52 |
| 0x08071720 | `MLGameString::MLGameString(char const*, bool)` | 52 | 52/52 |
| 0x08071760 | `MLGameString::MLGameString(MLGameString const&)` | 15 | 15/15 |
| 0x08071770 | `MLGameString::MLGameString(MLGameString const&)` | 15 | 15/15 |
| 0x08071780 | `MLGameString::MLGameString(MLTempString const&)` | 49 | 49/49 |
| 0x080717c0 | `MLGameString::MLGameString(MLTempString const&)` | 49 | 49/49 |
| 0x08071800 | `MLGameString::c_str() const` | 13 | 13/13 |
| 0x08071810 | `MLGameStringTable::Get()` | 106 | 106/106 |
| 0x08071880 | `MLGameStringTable::Add(char const*, bool)` | 263 | 263/263 |
| 0x08071990 | `MLHashFun<MLGameString>::Hash(MLGameString const&)` | 20 | 20/20 |
| 0x080719b0 | `MLHashFun<MLGameString>::IsEqual(MLGameString const&, MLGameString const&)` | 24 | 24/24 |
| 0x080719d0 | `MLTempStringData::Get()` | 77 | 77/77 |
| 0x08071a20 | `MLTempStringData::FormatString(char const*, ...)` | 35 | 35/35 |
| 0x08071a50 | `MLTempStringData::VFormatString(char const*, char*)` | 238 | 238/238 |
| 0x08071b40 | `MLTempString::MLTempString(char const*, ...)` | 110 | 110/110 |
| 0x08071bb0 | `MLTempString::MLTempString(char const*, ...)` | 110 | 110/110 |
| 0x08071c20 | `MLTempString::MLTempString()` | 101 | 101/101 |
| 0x08071c90 | `MLTempString::MLTempString()` | 102 | 102/102 |
| 0x08071d00 | `MLTempString::Set(char const*, ...)` | 117 | 117/117 |
| 0x08071d80 | `MLTempString::operator+=(MLTempString const&)` | 53 | 53/53 |
| 0x08071dc0 | `MLTempString::operator+(MLTempString const&)` | 58 | 58/58 |
| 0x08071e00 | `__tcf_0` | 22 | 22/22 |
| 0x08071e20 | `MLHashTable<char const*, MLGameStringData*, MLHashFun<char const*>, 513>::FindCopy(char const* const&)` | 114 | 114/114 |
| 0x08071ea0 | `MLHashTable<char const*, MLGameStringData*, MLHashFun<char const*>, 513>::clear()` | 64 | 64/64 |
| 0x08071ee0 | `MLHashFun<char const*>::Hash(char const* const&)` | 86 | 86/86 |
| 0x08071f40 | `MLHashFun<char const*>::IsEqual(char const* const&, char const* const&)` | 40 | 40/40 |
| 0x08071f70 | `__libc_csu_init` | 84 | 84/84 |
| 0x08071fd0 | `__libc_csu_fini` | 80 | 80/80 |

This inventory is a dated verification snapshot. The historical source
compiler and original vendor reference are obtained separately and
are not committed. To regenerate: run the exact matcher, then the
partial checkpoint matcher, then this report renderer.
