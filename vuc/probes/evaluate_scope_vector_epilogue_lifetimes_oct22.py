#!/usr/bin/env python3
"""GCC3.3 vector::_M_insert_aux three-field data-dependency hypotheses.

The original output commits vector start, finish and capacity in that order.
Historical GCC 3.3.5 headers already spell that order, but its register
scheduler emits the first two in reverse. Evaluate C++03 source-dependent
member reads and real value lifetimes, without modifying the toolchain.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess
ROOT=Path(__file__).resolve().parents[1]
WORK=Path("/tmp/vuc-vector-epilogue-dataflow-oct22")
WORK.mkdir(parents=True,exist_ok=True)
HEADER=Path("/home/fjtrujy.guest/.cache/vuc-toolchain/gcc335/root/usr/include/c++/3.3/bits/vector.tcc")
source=HEADER.read_text()
begin=source.index("    _M_insert_aux(iterator __position, const _Tp& __x)")
end=source.index("  #ifdef _GLIBCPP_DEPRECATED",begin)
func=source[begin:end]
original="""        _M_start = __new_start.base();
        _M_finish = __new_finish.base();
        _M_end_of_storage = __new_start.base() + __len;"""
assert func.count(original)==1
modsp=importlib.util.spec_from_file_location("vuc_match_probe_oct22_epilogue",ROOT/"scripts/match_probe.py")
match=importlib.util.module_from_spec(modsp);modsp.loader.exec_module(match)
forms={
 "member":("_M_start + __len",""),
 "this-member":("this->_M_start + __len",""),
 "new-base":("__new_start.base() + __len",""),
 "new-start-local":("newStartForEnd + __len","pointer newStartForEnd = __new_start.base();"),
 "member-local":("newStartForEnd + __len","pointer newStartForEnd = _M_start;"),
 "member-const-local":("newStartForEnd + __len","pointer const newStartForEnd = _M_start;"),
 "new-start-const-local":("newStartForEnd + __len","pointer const newStartForEnd = __new_start.base();"),
 "member-ref":("newStartForEnd + __len","pointer &newStartForEnd = _M_start;"),
}
layouts={
 "end-inline":("_M_start = __new_start.base();", "_M_finish = __new_finish.base();"),
 "cached-start":("_M_start = cachedStart;", "_M_finish = __new_finish.base();"),
 "cached-both":("_M_start = cachedStart;", "_M_finish = cachedFinish;"),
}
results=[]
for (name,(end_expr,local)),(layout,(start_stmt,finish_stmt)),local_when in itertools.product(forms.items(),layouts.items(),("before","after")):
 if name in ("member-local","member-const-local") and local_when=="before":
  continue  # Capturing the *old* start would change vector growth semantics.
 tag=f"{name}__{layout}__{local_when}"
 localstart="pointer cachedStart = __new_start.base();" if layout!="end-inline" else ""
 localfinish="pointer cachedFinish = __new_finish.base();" if layout=="cached-both" else ""
 declarations="\n        ".join(x for x in (localstart,localfinish) if x)
 if local and local_when=="before": declarations="\n        ".join(x for x in (declarations,local) if x)
 # Local aliases of _M_start must be taken only after the member store.
 if local and local_when=="after": after=local
 else: after=""
 newbody="\n        ".join(x for x in (declarations,start_stmt,finish_stmt,after,
                             "_M_end_of_storage = "+end_expr+";") if x)
 variant=source[:begin]+func.replace(original,"        "+newbody,1)+source[end:]
 overlay=WORK/tag/"bits";overlay.mkdir(parents=True,exist_ok=True)
 (overlay/"vector.tcc").write_text(variant)
 record={"case":tag}
 for snapshot in ("jan25","dec06"):
  obj=WORK/tag/(snapshot+".o")
  cmd=["/tmp/vuc-binutils215/g++-"+snapshot+"-as215",
       "-O2","-frename-registers","-falign-jumps=1",
       "-I"+str(WORK/tag),"-c",str(ROOT/"probes/ScopeVectorInsertionOriginalUnitOct15i.cpp"),"-o",str(obj)]
  cp=subprocess.run(cmd,capture_output=True,text=True)
  if cp.returncode:record[snapshot]={"compile_error":cp.stderr[-320:]};continue
  scores={}
  for label,cfg in [("vector","scope_vector_insert_partial_oct15i.json"),
                    ("begin","symbol_begin_scope_probe.json")]:
   try:scores[label]=match.run(obj,ROOT/"config"/cfg)
   except Exception as e:scores[label]={"error":str(e)}
  record[snapshot]=scores
  if scores["vector"].get("exact"):print("EXACT",tag,snapshot,flush=True)
 results.append(record)
 if len(results)%12==0:print("completed",len(results),flush=True)
(WORK/"results.json").write_text(json.dumps(results,indent=2)+"\n")
for snapshot in ("jan25","dec06"):
 eligible=[(r["case"],r[snapshot]["vector"]["exact_bytes"],
            r[snapshot]["vector"]["different_offsets"])
           for r in results if r.get(snapshot,{}).get("vector",{}).get("length")==746 and
           r.get(snapshot,{}).get("begin",{}).get("exact")]
 print("ELIGIBLE",snapshot,len(eligible),"best",sorted(eligible,key=lambda row:-row[1])[:12],flush=True)
print("CHECKPOINT",len(results),"real-source variants",flush=True)
