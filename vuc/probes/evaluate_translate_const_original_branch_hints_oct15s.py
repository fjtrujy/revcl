#!/usr/bin/env python3
"""Original VUC TranslateConst source-shape controls with named type/boolean guards.

Prior compiler-option and Boolean-result sweeps leave the function at
291 bytes instead of 290. This narrower test varies whether the original
string-to-bool branch and/or kind dispatch might have an actual historical
branch-likelihood annotation. No artificial bytes/alignment patching.
"""
from pathlib import Path
import importlib.util,subprocess,json
R=Path(__file__).resolve().parents[1]
OUT=Path("/tmp/vuc-binutils215/translate-const-guard-owner-oct15s")
OUT.mkdir(exist_ok=True)
base=(R/"probes/translate_const_glibc_oct14.cpp").read_text().replace('#include "../src/MLGameString.h"','#include "'+str(R/"src/MLGameString.h")+'"')
spec=importlib.util.spec_from_file_location("match",R/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
manifest=R/"config/translate_const_oct14_probe.json"
forms={
 "original":base,
 "true-unlikely":base.replace("if (value == truth)","if (__builtin_expect(value == truth, 0))"),
 "true-likely":base.replace("if (value == truth)","if (__builtin_expect(value == truth, 1))"),
 "kind-unlikely-2":base.replace("switch(type.kind) {","switch(type.kind) {").replace("case 2: {","case 2: {"),
 "const-bool":base.replace('MLGameString truth("true",false);','const MLGameString truth("true",false);'),
 "bool-comparison":base.replace("if (value == truth)","bool yes=(value==truth); if (yes)"),
 "bool-comparison-unlikely":base.replace("if (value == truth)","bool yes=(value==truth); if (__builtin_expect(yes,0))"),
 "false-arm-primary":base.replace('if (value == truth)\n            text = MLGameString("1",false);\n        else\n            text = MLGameString("0",false);','if (value != truth)\n            text = MLGameString("0",false);\n        else\n            text = MLGameString("1",false);'),
 "false-arm-unlikely":base.replace('if (value == truth)\n            text = MLGameString("1",false);\n        else\n            text = MLGameString("0",false);','if (__builtin_expect(!(value == truth),0))\n            text = MLGameString("0",false);\n        else\n            text = MLGameString("1",false);'),
}
# Additional genuine top-level guard form, preserving all constructor branches:
forms["early-float"]=base.replace('switch(type.kind) {\n    case 2:', 'switch(type.kind) {\n    case 2:') # explicit baseline control
rows=[]
for tag,source in forms.items():
 cpp=OUT/(tag+".cpp");cpp.write_text(source)
 for compiler in ("jan25","dec06"):
  obj=OUT/(tag+"-"+compiler+".o")
  cp=subprocess.run(["/tmp/vuc-binutils215/g++-"+compiler+"-as215","-O2","-frename-registers","-falign-jumps=1","-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  row={"case":tag,"compiler":compiler}
  if cp.returncode:row["error"]=cp.stderr[-140:]
  else:
   sy=subprocess.run(["nm","-S","--defined-only",str(obj)],capture_output=True,text=True)
   matches=[x.split() for x in sy.stdout.splitlines() if x.endswith("_Z14TranslateConstRK12MLGameStringRK4Type") and len(x.split())>=4]
   row["length"]=int(matches[0][1],16) if matches else -1
   if row["length"]==290:
    try:
     ans=m.run(obj,manifest)
     row.update(matched=ans["exact_bytes"],relocations=ans["relocations"],literals=ans["verified_literal_bytes"])
    except ValueError as e:row["error"]=str(e)
  rows.append(row)
  print(row,flush=True)
(OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
