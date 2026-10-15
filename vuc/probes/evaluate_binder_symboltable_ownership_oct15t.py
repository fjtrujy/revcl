#!/usr/bin/env python3
"""Original 552-byte Binder_Bind C++03 SymbolTable constructor ownership.

ELF proves begin/end scopes and 0x804-byte vector allocator reserve.
Test plausible class ownership forms and member destructor declarations
while preserving the canonical global 12-byte vector layout and original
calls. Scratch-only controls; never weaken exact manifest.
"""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[1]
OUT=Path("/tmp/vuc-binutils215/binder-symboltable-owner-oct15t");OUT.mkdir(exist_ok=True)
original=(R/"probes/BinderBindASTModuleFreshOct15s.cpp").read_text()
original=original.replace('#include "../src/MLGameString.h"','#include "'+str(R/"src/MLGameString.h")+'"')
original=original.replace('#include "../src/MLHashFunGameString.h"','#include "'+str(R/"src/MLHashFunGameString.h")+'"')
original=original.replace('#include "../src/MLHashTable.h"','#include "'+str(R/"src/MLHashTable.h")+'"')
original=original.replace('#include "../src/ASTNodeList.h"','#include "'+str(R/"src/ASTNodeList.h")+'"')
start=original.index('struct SymbolTable {')
end=original.index('struct ASTModule {')
method='''    void BeginScope();
    void EndScope();
    SymbolTableEntry *Find(const MLGameString&, bool);
'''
variants={
 "baseline":original[start:end],
 "member-explicit-default":"""struct SymbolTable {
 std::vector<Scope> scopes;
 SymbolTable():scopes() { scopes.reserve(100); }
"""+method+"};\n",
 "member-empty-dtor":"""struct SymbolTable {
 std::vector<Scope> scopes;
 SymbolTable() {scopes.reserve(100);}
 ~SymbolTable() {}
"""+method+"};\n",
 "member-nothrow-dtor":"""struct SymbolTable {
 std::vector<Scope> scopes;
 SymbolTable() {scopes.reserve(100);}
 ~SymbolTable() throw() {}
"""+method+"};\n",
 "member-copy-dtor":"""struct SymbolTable {
 std::vector<Scope> scopes;
 SymbolTable() {scopes.reserve(100);}
 ~SymbolTable() { (void)scopes.size(); }
"""+method+"};\n",
 "member-local-reserve":"""struct SymbolTable {
 std::vector<Scope> scopes;
 SymbolTable() { std::vector<Scope>& v=scopes; v.reserve(100); }
"""+method+"};\n",
 "inherited":"""struct SymbolTable: public std::vector<Scope> {
 SymbolTable() { reserve(100); }
"""+method+"};\n",
 "inherited-explicit-dtor":"""struct SymbolTable: public std::vector<Scope> {
 SymbolTable() { reserve(100); }
 ~SymbolTable() {}
"""+method+"};\n",
 "inherited-noexcept-dtor":"""struct SymbolTable: public std::vector<Scope> {
 SymbolTable() { reserve(100); }
 ~SymbolTable() throw() {}
"""+method+"};\n",
 "member-array":"""struct SymbolTable {
 std::vector<Scope> scopes;
 SymbolTable() { this->scopes.reserve(100u); }
"""+method+"};\n",
 "member-template-reserve":"""struct SymbolTable {
 std::vector<Scope> scopes;
 SymbolTable():scopes() {this->scopes.reserve(100);}
"""+method+"};\n",
 "member-empty-try-dtor":"""struct SymbolTable {
 std::vector<Scope> scopes;
 SymbolTable() {scopes.reserve(100);}
 ~SymbolTable() { try {} catch(...) {} }
"""+method+"};\n",
}
results=[]
for tag,code in variants.items():
 cpp=OUT/(tag+".cpp")
 cpp.write_text(original[:start]+code+original[end:])
 for compiler in ("jan25","dec06"):
  obj=OUT/(tag+"-"+compiler+".o")
  p=subprocess.run(["/tmp/vuc-binutils215/g++-"+compiler+"-as215","-O2","-frename-registers","-falign-jumps=8","-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  result={"case":tag,"compiler":compiler}
  if p.returncode:result["error"]=p.stderr[-140:]
  else:
   nm=subprocess.run(["nm","-S","--defined-only",str(obj)],capture_output=True,text=True)
   syms=[s.split() for s in nm.stdout.splitlines() if s.endswith("_Z11Binder_BindP9ASTModule") and len(s.split())>=4]
   result["length"]=int(syms[0][1],16) if syms else None
  results.append(result);print(result,flush=True)
(OUT/"results.json").write_text(json.dumps(results,indent=2)+"\n")
