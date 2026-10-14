#!/usr/bin/env python3
"""VUC AST statement-list Translate() original length and branch layout.

Natural 116-byte candidate is near original 119-byte loop/heap sequence;
test historical GCC 3.3 branch/loop/label choices and actual node ownership
without synthetic register or alignment instructions.
"""
from pathlib import Path
from itertools import product
import importlib.util,subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/stmtlist-layout-oct14");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/"vuc/probes/translate_stmt_list.cpp").read_text()
base=base.replace('#include "../src/ASTNodeList.h"','#include "'+str(R/'vuc/src/ASTNodeList.h')+'"')
forms={"baseline":base}
forms["early-continue"]=base.replace(
 "        if (item) {\n            if (result)\n                result=(IR::Stmt_Data *)new IR::Seq_Data(result, item);\n            else\n                result=item;\n        }",
 "        if (!item) continue;\n        if (!result) result=item;\n        else result=(IR::Stmt_Data *)new IR::Seq_Data(result,item);")
forms["local-seq"]=base.replace(
 "result=(IR::Stmt_Data *)new IR::Seq_Data(result, item);",
 "result=(IR::Stmt_Data *)(IR::Seq_Data *)new IR::Seq_Data(result, item);")
forms["loop-snapshot"]=base.replace(
 "    for (int i=0; i<statements.length; ++i) {",
 "    int count=statements.length;\n    for (int i=0; i<count; ++i) {")
forms["while-index"]=base.replace(
 "    for (int i=0; i<statements.length; ++i) {",
 "    int i=0;\n    while (i<statements.length) {").replace(
 "        }\n    }\n    return result;","        }\n        ++i;\n    }\n    return result;")
forms["first-assign"]=base.replace(
 "        if (item) {\n            if (result)\n                result=(IR::Stmt_Data *)new IR::Seq_Data(result, item);\n            else\n                result=item;\n        }",
 "        if (!item) continue;\n        if (result) result=(IR::Stmt_Data *)new IR::Seq_Data(result,item);\n"
 "        else result=item;")
rows=[]
flags=[(j,l,k,rename) for j,l,k,rename in product((1,2,3,4,8),(1,2,4,8),(1,4,16),(True,False))]
for name,code in forms.items():
 cpp=T/(name+".cpp");cpp.write_text(code)
 for j,l,k,rename in flags:
  opts=["-O2",f"-falign-jumps={j}",f"-falign-labels={l}",f"-falign-loops={k}"]
  if rename:opts.append("-frename-registers")
  key=f"j{j}-label{l}-loop{k}-"+("r" if rename else "p")
  obj=T/(name+"-"+key+".o")
  cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  if cp.returncode:
   print("ERROR",name,key,cp.stderr[-95:],flush=True);continue
  nm=subprocess.check_output(["nm","-S",str(obj)],text=True)
  sym=next((x for x in nm.splitlines() if x.endswith("_Z9TranslateR11ASTNodeListIP7ASTStmtE")),"")
  size=int(sym.split()[1],16) if sym else -1
  if size!=119:
   rows.append((0,size,name,key,"length"));continue
  try:
   result=m.run(obj,R/"vuc/config/translate_stmt_list_probe.json")
   rows.append((result["exact_bytes"],size,name,key,result["different_offsets"][:15]))
   if result["exact"]:
    print("NEW_EXACT",name,key,cpp,flush=True);raise SystemExit(0)
  except ValueError as e:rows.append((0,size,name,key,str(e)[:100]))
 print("DONE",name,flush=True)
print("TOP")
for x in sorted(rows,reverse=True)[:35]:print(x,flush=True)
