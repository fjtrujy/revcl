#!/usr/bin/env python3
"""Test authentic GCC3.3-era vector::_M_insert_aux member-store order.

The current VUC 733/746 partial differs only in a late three-member
vector success epilogue. This checks whether historical, semantically
equivalent source ordering in the C++03 vector.tcc header affects GCC
RTL allocation and scheduling, without changing any production header,
compiler, assembler, code bytes, or accepted source.
"""
from pathlib import Path
import importlib.util, itertools, json, subprocess
ROOT=Path(__file__).resolve().parents[1]
OUT=Path("/tmp/vuc-vector-stores-oct22")
OUT.mkdir(parents=True,exist_ok=True)
HEADER=Path("/home/fjtrujy.guest/.cache/vuc-toolchain/gcc335/root/usr/include/c++/3.3/bits/vector.tcc")
source=HEADER.read_text()
begin=source.index("    _M_insert_aux(iterator __position, const _Tp& __x)")
end=source.index("  #ifdef _GLIBCPP_DEPRECATED",begin)
function=source[begin:end]
stores=[
  "_M_start = __new_start.base();",
  "_M_finish = __new_finish.base();",
  "_M_end_of_storage = __new_start.base() + __len;"
]
chunk="\n        ".join(stores)
assert function.count(chunk)==1
sp=importlib.util.spec_from_file_location("vuc_match_oct22",ROOT/"scripts/match_probe.py")
match=importlib.util.module_from_spec(sp);sp.loader.exec_module(match)
results=[]
for order in itertools.permutations(range(3)):
 tag="order"+"".join(map(str,order))
 overlay=OUT/tag/"bits";overlay.mkdir(parents=True,exist_ok=True)
 changed=function.replace(chunk,"\n        ".join(stores[i] for i in order),1)
 assert changed!=function or order==(0,1,2)
 (overlay/"vector.tcc").write_text(source[:begin]+changed+source[end:])
 row={"order":order}
 for snapshot in ("jan25","dec06"):
  obj=OUT/tag/(snapshot+".o")
  cmd=["/tmp/vuc-binutils215/g++-"+snapshot+"-as215",
       "-O2","-frename-registers","-falign-jumps=1",
       "-I"+str(OUT/tag),"-c",str(ROOT/"probes/ScopeVectorInsertionOriginalUnitOct15i.cpp"),"-o",str(obj)]
  compiled=subprocess.run(cmd,text=True,capture_output=True)
  if compiled.returncode:row[snapshot]={"error":compiled.stderr[-500:]};continue
  scores={}
  for label,cfg in (("vector","scope_vector_insert_partial_oct15i.json"),
                    ("begin","symbol_begin_scope_probe.json")):
   try:scores[label]=match.run(obj,ROOT/"config"/cfg)
   except Exception as e:scores[label]={"error":str(e)}
  row[snapshot]=scores
  print(tag,snapshot,"vector",scores["vector"].get("exact_bytes"),"/",scores["vector"].get("length"),
        "begin",scores["begin"].get("exact_bytes"),"/",scores["begin"].get("length"),flush=True)
 results.append(row)
(OUT/"results.json").write_text(json.dumps(results,indent=2)+"\n")
for snapshot in ("jan25","dec06"):
 eligible=[r for r in results if r.get(snapshot,{}).get("vector",{}).get("length")==746
         and r.get(snapshot,{}).get("begin",{}).get("exact")]
 print("ELIGIBLE",snapshot,[(r["order"],r[snapshot]["vector"].get("exact_bytes"),
                              r[snapshot]["vector"].get("different_offsets")) for r in eligible],flush=True)
