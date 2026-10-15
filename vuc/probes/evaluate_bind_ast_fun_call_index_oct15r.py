#!/usr/bin/env python3
"""Investigate historical GCC signed array-index lowering for Bind(ASTFunCall*).

Original 304-byte code differs at exactly +178..+181 in the recovered
source, original emits 'decl %eax; mov (%esi,%eax,4),%edi'; candidate
emits one '-4' indexed mov. Independently validate all referenced
literals and 23 genuine relocations. The standalone object has
unowned compiler-generated .rodata padding, so code-byte results
here are DIAGNOSTIC ONLY, NOT strict promotion scores.
"""
from pathlib import Path
import importlib.util,subprocess,json
R=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-binutils215/bind-ast-index-oct15r")
O.mkdir(exist_ok=True)
src=(R/"probes/BindASTFunCallFreshOct15r.cpp").read_text()
src=src.replace('#include "../src/MLGameString.h"','#include "'+str(R/"src/MLGameString.h")+'"')
src=src.replace('#include "../src/ASTNodeList.h"','#include "'+str(R/"src/ASTNodeList.h")+'"')
needle="matchStack.entries[matchStack.length-1]->line"
assert needle in src
variants={
 "baseline":"matchStack.entries[matchStack.length-1]->line",
 "prefix-local-index":"get_index(matchStack)->line",
 "pointer-arithmetic":"(*(matchStack.entries + matchStack.length - 1))->line",
 "pointer-reverse":"(*(matchStack.entries - 1 + matchStack.length))->line",
 "unsigned-index":"matchStack.entries[(unsigned)(matchStack.length-1)]->line",
 "signed-index":"matchStack.entries[(int)(matchStack.length-1)]->line",
 "cast-uint-first":"matchStack.entries[(unsigned)matchStack.length-1]->line",
 "temporary-index":"(last_element(matchStack))->line",
 "inline-operator":"matchStack.last()->line",
 "negative-displacement":"matchStack.entries[matchStack.length + -1]->line",
 "number-index":"matchStack.entries[(unsigned)matchStack.length + 0xffffffffu]->line",
 "explicit-local":"(matchStack.entries[last_index(matchStack.length)])->line",
}
inline="""static int last_index(int n){ --n; return n; }
static ASTNode *last_element(ASTNodeList<ASTNode*> &stack) {
  int index=stack.length; --index; return stack.entries[index];
}
static ASTNode *get_index(ASTNodeList<ASTNode*> &stack) {
  int index=stack.length-1; ASTNode **table=stack.entries;
  return table[index];
}
"""
# ASTNodeList comes from original header; no new ABI state is invented.
# Add optional method to template in scratch *after* copying header.
spec=importlib.util.spec_from_file_location("matcher",R/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
cfg=json.loads((R/"config/bind_ast_fun_call_partial_oct15r.json").read_text())
data,elf=m.decomp.load()
code_section=next(s for s in elf["sections"] if s["name"]==".text")
address=int(cfg["address"],16)
original=data[code_section["offset"]+address-code_section["address"]:code_section["offset"]+address-code_section["address"]+cfg["size"]]
ro=next(s for s in elf["sections"] if s["name"]==".rodata")
rows=[]
for tag,expr in variants.items():
 cpp=O/(tag+".cpp")
 p=src.replace(needle,expr)
 if tag=="inline-operator":
  p=p.replace("ASTNodeList<ASTNode *> matchStack","ASTNodeList<ASTNode *> matchStack")
  p=p.replace("matchStack.last()->line","last_element(matchStack)->line")
 p=p.replace("void Bind(ASTFunCall *call, SymbolTable &symbols)",inline+"\nvoid Bind(ASTFunCall *call, SymbolTable &symbols)")
 cpp.write_text(p)
 for snap in ("jan25","dec06"):
  obj=O/(tag+"-"+snap+".o")
  comp=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215","-O2","-frename-registers","-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  row={"source":tag,"compiler":snap}
  if comp.returncode:row["error"]=comp.stderr[-240:]
  else:
   binary=obj.read_bytes()
   secs,_=m.elf_sections(binary)
   lit=m.section_bytes(binary,secs[".rodata"][1])
   fullvalid=True
   for literal in cfg["relocation_literals"][".rodata"]:
    n=literal["size"];o=literal["offset"];a=int(literal["address"],16)
    expected=data[ro["offset"]+a-ro["address"]:ro["offset"]+a-ro["address"]+n]
    if lit[o:o+n]!=expected:fullvalid=False
   row["referenced_literals_authentic"]=fullvalid
   try:
    normalized,count=m.prepare_named_function(binary,cfg["function"],cfg["relocations"],address,{},cfg["relocation_literals"])
    row["length"]=len(normalized)
    row["relocations"]=count
    row["code_match_diagnostic"]=m.compare(normalized,original)["exact_bytes"] if len(normalized)==cfg["size"] else None
    row["differing_offsets"]=m.compare(normalized,original)["different_offsets"] if len(normalized)==cfg["size"] else []
    row["strict_literal_coverage"]=False # standalone .rodata alignment remains unowned
   except ValueError as ex:row["error"]=str(ex)
  rows.append(row)
  print({k:v for k,v in row.items() if k!="differing_offsets"},flush=True)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
