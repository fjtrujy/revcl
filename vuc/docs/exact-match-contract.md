# VUC reconstruction — mandatory byte-exact acceptance

## Contract

The requested objective is a **byte-for-byte reconstruction of the complete
canonical VUC 2004.1 Linux i386 executable**, not behavioral equivalence and
not a best-effort percentage. Final acceptance requires all original bytes,
including code, read-only/writable data, section/linker layout, startup/runtime
objects, and any significant ELF metadata, to compare equal to the SHA-256
verified original. The complete executable comparison is **not implemented or
validated yet**; no overall score or completion claim is available.

For each function submitted for reconstruction, the intermediate acceptance
gate requires **every instruction byte to match** after resolving recorded
ELF32/i386 relocations to the evidence-backed original addresses. Every
literal mapped by the function manifest must match too. A single different
byte, different function length, unsupported relocation, wrong symbol,
unmapped target, or failed compilation is a **failure**.

Partial comparisons are strictly diagnostic. They must never be entered into
the accepted source tree `vuc/src/`, the exact regression manifest, or be
reported as successful reconstruction. Do not substitute hand-written
machine-byte blobs, fabricated addresses, linker padding, or hardcoded
assembled instructions for plausible C++ source.

## Original-source inventory

`vuc/scripts/source_units.py` reads the immutable ZIP-backed ELF symbol table
and regenerates `vuc/config/source_units.tsv`. Results:

- 57 `STT_FILE` records including historical libc and compiler startup files
- **22 original VUC `.cpp` translation-unit filenames** in symbol order
- **79 locally bound text-function records** attributable to those filenames

Local symbol records between `STT_FILE` markers give useful provenance.
Non-local/global symbols appear in a separate portion of the ELF symbol table:
the inventory **does not invent global-function ownership** from that
ordering. This is an inventory, not a recovered link map or source tree.

## Current exact-only regression baseline

Compiler: source-date GCC `3.3.3 20040125 (prerelease)`, built in the
existing `revcl-oracle` Linux VM. The original Debian `0pre3` patch set,
2004 libstdc++ headers and final linker toolchain remain unauthenticated.
The matching pipeline uses the separately recovered GNU **as 2.15** because
modern binutils 2.40 emits different alignment padding; the exact original
assembler version and linker remain unauthenticated.
Using `-frename-registers` on selected candidates gives exact bytes; this
is a **candidate flag hypothesis**, not proof of the original per-TU flags.

| Original VUC function | Byte match | Candidate flags |
| --- | ---: | --- |
| `Instruction::ToString()` | 9/9 | `-O2` |
| `ListFile::GetNumLines()` | 18/18 | `-O2` |
| `MLGameString::c_str() const` | 13/13 | `-O2 -frename-registers` |
| `AsmOutput_WriteEpilogue()` | 30/30 | `-O2 -frename-registers` |
| `MLGameString` default ctor C1 / C2 | 14/14 each | `-O2 -frename-registers` |
| `MLGameString` copy ctor C1 / C2 | 15/15 each | `-O2 -frename-registers` |
| `MLGameString(char const*, bool)` ctor C1 / C2 | 52/52 each | `-O2 -frename-registers` |
| `MLGameString(MLTempString const&)` ctor C1 / C2 | 49/49 each | `-O2 -frename-registers` |
| `MLHashFun<MLGameString>::Hash()` | 20/20 | `-O2 -frename-registers` |
| `Value` default ctor C1 / C2 | 44/44 each | `-O2 -frename-registers` |
| `MLGameStringData(char const*, bool)` ctor C1 / C2 | 111/111 each | `-O2 -frename-registers` |
| `MLHashFun<char const*>::IsEqual()` | 40/40 | `-O2 -frename-registers -falign-jumps=1` |
| `MLHashTable<int,int,...>::clear()` | 64/64 | `-O2 -frename-registers -falign-jumps=1` |
| `MLHashTable<char const*,MLGameStringData*,...>::clear()` | 64/64 | `-O2 -frename-registers -falign-jumps=1` |
| `MLHashFun<char const*>::Hash()` | 86/86 | `-O2 -frename-registers`, GNU as 2.15 |
| `MLHashFun<MLGameString>::IsEqual()` | 24/24 | `-O2 -frename-registers` |
| `MLTempStringData::FormatString()` | 35/35 | `-O2 -frename-registers` |
| `MLTempString::operator+=()` | 53/53 | `-O2 -frename-registers` |
| `MLTempString::operator+()` | 58/58 | `-O2 -frename-registers` |
| `MLHashTable<char const*,MLGameStringData*,...>::FindCopy()` | 114/114 | `-O2 -frename-registers -falign-jumps=1` |
| `MLTempStringData::Get()` | 77/77 | `-O2 -falign-jumps=1`, ELF object rebasing |
| `SymbolTable::EndScope()` | 27/27 | `-O2 -frename-registers` |
| `yyFlexLexer::LexerOutput()` | 18/18 | `-O2 -frename-registers` |
| `Value::Set(ASTExpr const*)` | 42/42 | `-O2 -falign-jumps=1` |
| `Semant_Check(ASTModule*)` | 40/40 | `-O2 -frename-registers` |
| `Check(ASTNodeList<ASTVarDecl*>&)` | 45/45 | `-O2 -frename-registers -falign-jumps=1` |
| `Check(ASTNodeList<ASTStmt*>&)` | 45/45 | `-O2 -frename-registers -falign-jumps=1` |
| `Check(ASTNodeList<ASTDecl*>&)` | 45/45 | `-O2 -frename-registers -falign-jumps=1` |
| `FlexLexer::~FlexLexer()` D1 | 14/14 | `-O2 -frename-registers`, original vtable |
| `FlexLexer::~FlexLexer()` D0 | 18/18 | `-O2 -frename-registers`, original vtable |
| `Translate(ASTNodeList<ASTVarDecl*>&)` | 45/45 | `-O2 -frename-registers -falign-jumps=1` |
| `Bind(ASTNodeList<ASTVarDecl*>&, SymbolTable&)` | 57/57 | `-O2 -frename-registers` |
| `Bind(ASTNodeList<ASTExpr*>&, SymbolTable&)` | 57/57 | `-O2 -frename-registers` |
| `Bind(ASTNodeList<ASTStmt*>&, SymbolTable&)` | 57/57 | `-O2 -frename-registers` |
| `Bind(ASTNodeList<ASTDecl*>&, SymbolTable&)` | 57/57 | `-O2 -frename-registers` |
| `MLHashTable<MLGameString,SymbolTableEntry*,...>::clear()` | 64/64 | `-O2 -frename-registers -falign-jumps=1` |
| `Bind(ASTNodeList<MLGameString>&, SymbolTable&)` | 57/57 | `-O2 -frename-registers` |
| `script_lex()` | 39/39 | `-O2 -frename-registers` |
| `IsFoldable(IR::Operator_Data*)` | 41/41 | `-O2 -frename-registers -falign-jumps=1` |
| `__tcf_0` (20 original iostream cleanup functions) | 20 × 16/16 | `-O2 -frename-registers -fuse-cxa-atexit`, named BSS objects |
| `__static_initialization_and_destruction_0(int,int)` (14 66-byte original iostream helpers) | 14 × 66/66 | `-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections -falign-jumps=1` |
| `_GLOBAL__I_constantsTabStop` | 31/31 | `-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections` |
| `MLHashTable<MLGameString,MLGameString,...>::clear()` | 64/64 | `-O2 -frename-registers -falign-jumps=1` |
| `IsCommutative(IR::Operator_Data*)` | 66/66 | `-O2 -frename-registers` |
| `InstructionList::Add(Instruction*)` | 55/55 | `-O2 -frename-registers -falign-jumps=1` |
| `Print(ASTNodeList<ASTExpr*>&,...)` | 80/80 | `-O2 -frename-registers` |
| `Print(ASTNodeList<ASTStmt*>&,...)` | 80/80 | `-O2 -frename-registers` |
| `Print(ASTNodeList<ASTVarDecl*>&,...)` | 80/80 | `-O2 -frename-registers` |
| `Print(ASTNodeList<ASTDecl*>&,...)` | 80/80 | `-O2 -frename-registers` |
| `ConvVal(...,float&)` | 51/51 | `-O2 -frename-registers` |
| `ConvVal(...,int&)` | 60/60 | `-O2 -frename-registers` |
| `ConvVal(...,unsigned&)` | 60/60 | `-O2 -frename-registers` |
| `yyFlexLexer::LexerError()` | 54/54 | `-O2 -frename-registers` |
| `MLHashTable<MLGameString,SymbolTableEntry*,...>::FindCopy()` | 114/114 | `-O2 -frename-registers -falign-jumps=1` |
| `MLHashTable<MLGameString,MLGameString,...>::FindPtr()` | 114/114 | `-O2 -frename-registers -falign-jumps=1` |
| `AlignConstant(char const*)` | 69/69 | `-O2 -frename-registers`, original string literal |
| `yyFlexLexer::yy_switch_to_buffer()` | 75/75 | `-O2 -frename-registers` |
| `yyFlexLexer::yy_init_buffer()` | 53/53 | `-O2 -frename-registers` |
| `yyFlexLexer::yy_flush_buffer()` | 80/80 | `-O2 -frename-registers -falign-jumps=1` |
| `yyFlexLexer::yyrestart(std::istream*)` | 93/93 | `-O2 -frename-registers` |
| `std::__uninitialized_copy_aux<ConstTypeIterator,Type*>()` | 79/79 | `-O2 -frename-registers` |
| `std::__uninitialized_copy_aux<TypeIterator,TypeIterator>()` | 86/86 | `-O2 -frename-registers` |
| `yyFlexLexer::switch_streams()` | 96/96 | `-O2 -frename-registers`, three virtual calls |
| `yyFlexLexer::LexerInput()` | 97/97 | `-O2 -frename-registers`, historical istream state checks |
| `yyFlexLexer::~yyFlexLexer()` D2 | 78/78 | `-O2 -frename-registers`, original vtables |
| `yyFlexLexer::~yyFlexLexer()` D1 | 78/78 | `-O2 -frename-registers`, original vtables |
| `yyFlexLexer::~yyFlexLexer()` D0 | 78/78 | `-O2 -frename-registers`, original vtables |
| `MLTempString::MLTempString()` C1 | 102/102 | `-O2 -frename-registers`, exact singleton guard and empty literal |
| `_GLOBAL__I__ZN7ASTNode3memE` | 31/31 | `-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections`, original helper call |
| `SymbolTable::Find(MLGameString const&,bool)` | 115/115 | `-O2 -frename-registers` |
| `ListFile::Print(int,int,std::ofstream&)` | 88/88 | `-O2 -frename-registers -falign-jumps=1` |
| `ListFile::PrintLine(int,std::ofstream&)` | 133/133 | `-O2 -frename-registers`, `std::string::_Rep::_S_terminal` |
| `__tcf_1` (original `globalEnv` cleanup) | 16/16 | `-O2 -frename-registers -fuse-cxa-atexit` |
| `__tcf_2` (original `localEnv` cleanup) | 16/16 | `-O2 -frename-registers -fuse-cxa-atexit` |
| `AssembleModule(IR::Module_Data*,InstructionList&)` | 89/89 | `-O2 -frename-registers` |
| `GetOpType(IR::Expr_Data*)` | 117/117 | `-O2 -frename-registers`, `IR::UInt_Data` vtable |
| `InstructionList::Print(std::ostream&)` | 87/87 | `-O2 -frename-registers` |
| `Check<ASTExpr*>(ASTNodeList<ASTExpr*>&)` | 66/66 | `-O2 -frename-registers`, 16-byte result ABI |
| `main(int,char**)` | 63/63 | `-O2 -frename-registers` |
| `__tcf_1` (AST arena teardown) | 46/46 | `-O2 -frename-registers -fuse-cxa-atexit` |
| `InstructionList::AddL(MLGameString const&)` | 107/107 | `-O2 -frename-registers -falign-jumps=1` |
| `TranslateFrag(MLGameString const&)` | 87/87 | `-O2 -frename-registers -falign-jumps=1` |

**133/133 exact function-symbol records, 6761/6761 checked code bytes.** This counts only the
explicitly accepted candidates, not all original functions. The end of the
`AsmOutput_WriteEpilogue` string literal is also checked from the original
ELF. All 289 supported relocations across the accepted candidate functions
are resolved and verified. The **nine** original `MLGameString` member functions
are now exact as a class-method cluster (273/273 bytes). This does **not**
establish that the entire original `MLGameString.cpp` translation unit is
complete or byte-identical.

`vuc/config/function_status.tsv` provides a symbol-by-symbol reconstruction
queue for all 298 identified text function symbols. It currently contains
133 EXACT, 10 EXPERIMENTAL and 155 UNRESOLVED entries. Generate it using
`python3 vuc/scripts/coverage.py --write` against the authenticated original
ELF and the two candidate manifests. It is **not** an executable byte score.

`AsmOutput_WritePrologue` remains **non-exact** (22/30 matching code bytes
under the tested `-O2` hypothesis). Its matching 92 literal bytes do not
make the function acceptable. It remains exclusively in `vuc/probes/`.

## Reproduction and failure rules

On the host with the existing Lima VM and January compiler bootstrap:

```sh
# Accepted VUC source fragments: command must exit zero
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate

# All experimental candidates: deliberately exits nonzero until all are exact
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --manifest /path/to/revcl/vuc/config/selected_candidates.json \
  --out /tmp/vuc-binutils215/diagnostic-gate

python3 -m unittest discover -s vuc/tests -v
python3 vuc/scripts/source_units.py --write
```

The first command checks `vuc/config/exact_candidates.json` by default.
It returns nonzero if any exact regression deteriorates. The second
intentionally includes the still-unmatched prologue, so **failure is correct**.
A generated `candidate_summary.json` includes
`acceptance: PASS/FAIL`, `all_candidates_exact: true/false` and
`whole_binary_score: null`. It must never silently accept a partial
candidate.

## Next steps

Recover the remaining original source and object boundaries; expand exact
matches without sacrificing the current regression set; obtain the complete
historical Debian compiler/binutils/linker/libstdc++ inputs; and construct
a fully validated executable-level byte comparator. The final original
binary SHA-256 remains the ultimate objective, but comparison must use the
original ZIP-backed bytes, not inferred filenames or addresses.
