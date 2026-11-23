#!/usr/bin/env python3
"""Original InstructionList TU ordering versus 45/46 Instruction arena cleanup.

Unlike earlier inline experiments, move the real Instruction arena global
lifecycle definition among already reconstructed InstructionList methods.
No source-byte patching, fabricated functions, force-register directives,
or assembly. A new exact cleanup must retain original 265/265 STL vector.
"""
from pathlib import Path
import importlib.util,json,subprocess
V=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-instruction-arena-real-definition-order-20261123");O.mkdir(exist_ok=True)
list_source=(V/"src/InstructionListLineCommentUnit.cpp").read_text()
list_source=list_source.replace('#include "MLGameString.h"',f'#include "{V/"src/MLGameString.h"}"')
list_source="#include <iostream>\n#include <new>\n"+list_source
list_source=list_source.replace("    static void *operator new(std::size_t);",
                               "    static char *instrData;\n    static void *operator new(std::size_t);",1)
methodnames=[
"void InstructionList::Add(Instruction *instruction)",
"void InstructionList::Print(std::ostream &output)",
"void InstructionList::AddL(const MLGameString &name)",
"MLGameString InstructionList::GenLabel(const char *name)",
"MLGameString InstructionList::GenTmp(const char *name)",
"void InstructionList::Add(const char *format, ...)",
"void InstructionList::AddB(const char *format, ...)",
"void InstructionList::AddLineComment(int line)"
]
positions=[list_source.index(n) for n in methodnames]
head=list_source[:positions[0]]
methods=[list_source[positions[i]:positions[i+1] if i+1<len(positions) else len(list_source)] for i in range(len(positions))]
original=(V/"probes/instruction_full_source_unit_oct13.cpp").read_text()
rest=original[original.index("char *Instruction::instrData;"):]
rest=rest.replace("return text_.c_str();","return text.c_str();")
arena_pos=rest.index("struct InstructionArenaCleanup")
definitions=rest[:arena_pos]
lifecycle=rest[arena_pos:]
spec=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
targets={"cleanup":"instruction_arena_cleanup_unit_oct14_probe.json",
         "vector":"instruction_vector_insert_exact_oct15i.json"}
rows=[]
for position in range(len(methods)+1):
 for relation in ("definitions-first","definitions-last"):
  unit=head+(definitions if relation=="definitions-first" else "")
  unit+="\n".join(methods[:position])+"\n"+lifecycle+"\n"
  unit+="\n".join(methods[position:])+"\n"
  if relation=="definitions-last":unit+=definitions
  tag=f"index{position}-{relation}"
  src=O/(tag+".cpp");src.write_text(unit)
  for flags,opts in {
     "baseline":["-O2","-frename-registers","-fuse-cxa-atexit","-falign-jumps=1"],
     "jump4":["-O2","-frename-registers","-fuse-cxa-atexit","-falign-jumps=4"],
   }.items():
   for snap in ("jan25","dec06"):
    obj=O/(tag+"-"+flags+"-"+snap+".o")
    cp=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215",*opts,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
    row={"position":position,"relation":relation,"flags":flags,"compiler":snap}
    if cp.returncode:row["compile_error"]=cp.stderr[-250:]
    else:
     for key,conf in targets.items():
      try:
       v=m.run(obj,V/"config"/conf)
       row[key]=[v["exact_bytes"],v["length"],v["exact"]]
      except ValueError as ex:row[key]={"unverified":str(ex)}
    print(json.dumps(row,sort_keys=True),flush=True);rows.append(row)
    if isinstance(row.get("cleanup"),list) and row["cleanup"][2]:
     (O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
     print("NEW_EXACT",row,flush=True)
     raise SystemExit(0)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
