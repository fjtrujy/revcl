#!/usr/bin/env python3
"""Explore source-order variants for VUC Print<ASTVarDecl*>; strict exact only."""
from pathlib import Path
import importlib.util,subprocess
ROOT=Path(__file__).resolve().parents[2]
TEMP=Path("/tmp/vuc-binutils215")
spec=importlib.util.spec_from_file_location("vucmatch",ROOT/"vuc/scripts/match_probe.py")
match=importlib.util.module_from_spec(spec);spec.loader.exec_module(match)
raw=(ROOT/"vuc/src/PrintASTVarList.cpp").read_text()
needle="        T item = list.entries[i];\n        Print(item, output);"
assert needle in raw
variants={
 "direct":needle,
 "ptr-first":"        T *items = list.entries;\n        Print(items[i], output);",
 "value-first":"        T item = list.entries[i];\n        Print(item, output);",
 "typed-before":"        ASTDecl *item = list.entries[i];\n        Print(item, output);",
 "typed-after":"        T *items = list.entries;\n        ASTDecl *item = items[i];\n        Print(item, output);",
 "index-next":"        Print(list.entries[i++], output);",
 "offset":"        Print(*(list.entries + i), output);",
 "temporary-reference":"        T &item = list.entries[i];\n        Print(item, output);",
 "printer-ref":"        std::ostream &out = output;\n        Print(list.entries[i], out);",
 "printer-pointer":"        std::ostream *out = &output;\n        Print(list.entries[i], *out);",
}
flags={
 "ren":["-O2","-frename-registers"],
 "plain":["-O2"],
 "ren-j1":["-O2","-frename-registers","-falign-jumps=1"],
 "ren-no-sched":["-O2","-frename-registers","-fno-schedule-insns2"],
 "ren-no-gcse":["-O2","-frename-registers","-fno-gcse"],
 "ren-no-cse":["-O2","-frename-registers","-fno-cse-follow-jumps"],
}
res=[]
for name,expression in variants.items():
 cpp=TEMP/("print-var-"+name+".cpp");cpp.write_text(raw.replace(needle,expression))
 for fl,opts in flags.items():
  obj=TEMP/("print-var-"+name+"-"+fl+".o")
  cp=subprocess.run([str(TEMP/"g++-jan25-as215"),*opts,"-I",str(ROOT/"vuc/src"),"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  if cp.returncode:res.append((0,name,fl,cp.stderr[-120:]));continue
  try:
   d=match.run(obj,ROOT/"vuc/config/print_list_var_exact_probe.json")
   res.append((d["exact_bytes"],name,fl,str(d["length"])))
  except Exception as exc: res.append((0,name,fl,str(exc)[:80]))
res.sort(reverse=True)
for row in res[:25]: print(*row)
