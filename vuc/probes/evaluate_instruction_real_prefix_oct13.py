#!/usr/bin/env python3
"""Rebuild real InstructionList methods before Instruction arena cleanup.

The isolated arena already reproduces 45/46 bytes; the original
Instruction.cpp translation unit also defines instruction-list operations.
Only authentic reconstructed methods are inserted before ToString,
allocator, and static arena lifecycle to test GCC 3.3.3 peephole2 context.
"""
from pathlib import Path
import importlib.util, subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/instruction-real-prefix-oct13")
T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("matcher",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
full=(R/"vuc/probes/instruction_full_source_unit_oct13.cpp").read_text()
line=(R/"vuc/probes/instruction_line_with_unit_oct13.cpp").read_text()
pos=line.index("void InstructionList::Add(Instruction *instruction)")
header=line[:pos].replace('"../src/MLGameString.h"','"'+str(R/"vuc/src/MLGameString.h")+'"')
header=header.replace("#include <vector>","#include <vector>\n#include <iostream>\n#include <new>")
header=header.replace("struct Instruction {","struct Instruction {\n    static char *instrData;")
assert "    static void *operator new(std::size_t);" in header
line_methods=line[pos:]
unit_text=full[full.index("const char *Instruction::ToString()"):]
unit_text=unit_text.replace("text_.c_str()", "text.c_str()")
choices={
 "none":"",
 "list-add":line_methods[:line_methods.index("void InstructionList::Print(")],
 "list-all":line_methods,
 "list-before-linecomment":line_methods[:line_methods.index("void InstructionList::AddLineComment")],
 "list-linecomment":line_methods[line_methods.index("void InstructionList::AddLineComment"):],
}
opts={
 "ren":["-O2","-frename-registers","-fuse-cxa-atexit","-ffunction-sections"],
 "plain":["-O2","-fuse-cxa-atexit","-ffunction-sections"],
 "no-regmove":["-O2","-frename-registers","-fuse-cxa-atexit","-ffunction-sections","-fno-regmove"],
 "no-sched2":["-O2","-frename-registers","-fuse-cxa-atexit","-ffunction-sections","-fno-schedule-insns2"],
}
for name,methods in choices.items():
 src=T/(name+".cpp")
 src.write_text(header+"\nchar *Instruction::instrData;\n"+methods+"\n"+unit_text)
 for tag,flag in opts.items():
  obj=T/(name+"-"+tag+".o")
  p=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flag,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
  if p.returncode:
   print("COMPILE_FAIL",name,tag,p.stderr[-150:],flush=True);continue
  outcome={}
  for label,manifest in {"cleanup":"instruction_arena_cleanup_probe.json",
                         "text":"instruction_to_string_probe.json",
                         "line":"instruction_add_line_comment_context_oct13_probe.json"}.items():
   try:
    result=m.run(obj,R/"vuc/config"/manifest)
    outcome[label]=str(result["exact_bytes"])+"/"+str(result["length"])
    if label=="cleanup" and result["exact"]:print("NEW_EXACT",src,tag,outcome,flush=True)
   except (OSError,ValueError) as err:outcome[label]=str(err)[:80]
  print(name,tag,outcome,flush=True)
