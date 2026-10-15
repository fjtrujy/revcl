#!/usr/bin/env python3
"""Recover weak MLHashTable::FindCopy in the genuine SymbolTable TU.

Original SymbolTable.cpp owns Enter, Find, BeginScope and EndScope, then
vector insertion, iterator uninitialized-copy, and MLHashTable::FindCopy.
The existing SymbolTableUnit.cpp calls FindCopy but lacked its original
template definition, which historical GCC might take into account when
allocating the real vector/iterator helper registers. Introduce only the
already-recovered exact FindCopy source in original neighboring positions;
evaluate all six original addresses and full relocation/literal checks.
"""
from pathlib import Path
import json
import subprocess
import importlib.util
ROOT=Path(__file__).resolve().parents[2]
R=ROOT/"vuc"
O=Path("/tmp/vuc-binutils215/symboltable-findcopy-true-tunit-oct15n")
O.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("match",R/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
original=(R/"src/SymbolTableUnit.cpp").read_text()
for inc in ("MLGameString.h","MLHashTable.h","MLHashFunGameString.h"):
 original=original.replace('#include "'+inc+'"','#include "'+str(R/"src"/inc)+'"')
# The original recognized source body is exact. Do not infer the original
# compiler owner solely from this probe until the whole TU also co-matches.
findsrc=(R/"src/MLHashTableFindCopy.cpp").read_text()
body=findsrc[findsrc.index("template <class Key, class Value, class Hasher, int Buckets>"):]
both=body
one=body[:body.index("template MLGameStringData *")]
one += body[body.index("struct SymbolTableEntry;"):]
cases={
  "baseline":original,
  "findcopy-late-only":original+"\n"+one,
  "findcopy-late-both":original+"\n"+both,
  "findcopy-prior-begin":original.replace("void SymbolTable::BeginScope()",one+"\nvoid SymbolTable::BeginScope()"),
}
target={
 "Find":"symboltable_find_probe.json",
 "BeginScope":"symbol_begin_scope_probe.json",
 "EndScope":"symboltable_end_exact_probe.json",
 "ScopeInsert":"scope_vector_insert_partial_oct15i.json",
 "FindCopy":"hash_symbol_findcopy_probe.json",
}
# An authenticated scratch manifest maps the original-sized iterator
# template's original calls; it is not selected until all 149 bytes match.
from_json=O/"scope-uninitialized-strict.json"
from_json.write_text(json.dumps({
  "function":"_ZSt24__uninitialized_copy_auxIN9__gnu_cxx17__normal_iteratorIP11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS3_ELi513EESt6vectorIS8_SaIS8_EEEESD_ET0_T_SF_SE_12__false_type",
  "address":"0x0806a840","size":149,"source":"vuc/probes/symboltable_unit_oct15n.cpp",
  "flags":["-O2","-frename-registers","-falign-jumps=1"],
  "relocations":{
   "_ZN11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS0_ELi513EEC1ERKS5_":"0x0804f420",
   "_ZN11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS0_ELi513EE5clearEv":"0x0804f020",
   "__cxa_begin_catch":"0x0804995c","__cxa_rethrow":"0x0804979c",
   "__cxa_end_catch":"0x0804983c","_Unwind_Resume":"0x0804998c"
  }
},indent=2))
rows=[]
for tag,content in cases.items():
 cpp=O/(tag+".cpp");cpp.write_text(content)
 for snap in ("jan25","dec06"):
  obj=O/(tag+"-"+snap+".o")
  cp=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215","-O2","-frename-registers","-falign-jumps=1","-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  row={"owner":tag,"snapshot":snap}
  if cp.returncode:row["error"]=cp.stderr[-225:]
  else:
   for key,cfg in {**target,"ScopeUninitialized":"scope-uninitialized-strict.json"}.items():
    try:
     c=(R/"config"/cfg) if key!="ScopeUninitialized" else from_json
     v=m.run(obj,c)
     row[key]={'length':v['length'],'matched':v['exact_bytes'],'reloc':v['relocations'],'diff':v['different_offsets'][:21]}
    except ValueError as e:row[key]={'error':str(e)}
  rows.append(row)
  print(tag,snap,{k:(v.get('matched'),v.get('length'),v.get('error')) for k,v in row.items() if isinstance(v,dict)},row.get('error',''),flush=True)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
