#!/usr/bin/env python3
"""Test GCC-3.3 block-frequency hypotheses for VUC dispatch (experimental).

Original 193-byte dispatch and best 162/193 reconstruction differ at branch
target to case4 and GCC-generated case4 padding (0x91..0x93). This tests
real historical __builtin_expect arm likelihood, never byte edits or NOPs.
Likelihood is NOT authenticated original source; matching would be only a
diagnostic until source-level provenance is recovered.
"""
from pathlib import Path
from itertools import product
import importlib.util,subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/dispatch-probability-oct14");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/"vuc/probes/evaluate_assemble_binop_alignment_oct14.py").read_text()
# Keep the header and source-level straightforward 3,4,2,1 if dispatch.
src=(R/"vuc/probes/assemble_binop_dispatch_oct14.cpp").read_text()
head=src[:src.index("IR::Register AssembleBinOp(")].replace(
 '#include "../src/MLGameString.h"', '#include "'+str(R/'vuc/src/MLGameString.h')+'"')
sig="""IR::Register AssembleBinOp(const IR::Register &target,
                            IR::Expr_Data *expr,
                            InstructionList &instructions)
{
    int code = GetOpType(expr)->kind;
"""
checks={
3:"    if (code == 3) return AssembleBinOpFloat4(target, expr, instructions);\n",
4:"    if (code == 4) return AssembleBinOpFloat4(target, expr, instructions);\n",
2:"    if (code == 2) return AssembleBinOpInt(target, expr, instructions);\n",
1:"    if (code == 1) return AssembleBinOpInt(target, expr, instructions);\n",
}
scores=[]
for p3,p4,p2,p1 in product((-1,0,1),repeat=4):
 if p1 != 0:continue  # match known case1 unlikely edge
 key="p%d-%d-%d-%d"%(p3,p4,p2,p1)
 parts=[]
 for k,p in zip((3,4,2,1),(p3,p4,p2,p1)):
  line=checks[k]
  if p>=0:line=line.replace("code == %d"%k,"__builtin_expect(code == %d, %d)"%(k,p))
  parts.append(line)
 cpp=T/(key+".cpp")
 cpp.write_text(head+sig+"".join(parts)+"    return target;\n}\n")
 for j in (3,4):
  opts=["-O2","-frename-registers",f"-falign-jumps={j}","-falign-labels=1"]
  obj=T/(key+f"-j{j}.o")
  cc=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,
   "-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  if cc.returncode:
   print("ERROR",key,j,cc.stderr[-100:],flush=True);continue
  sym=next((x for x in subprocess.check_output(["nm","-S",str(obj)],text=True).splitlines()
    if x.endswith("_Z13AssembleBinOpRKN2IR8RegisterEPNS_9Expr_DataER15InstructionList")),"")
  size=int(sym.split()[1],16) if sym else -1
  if size!=193:
   scores.append((0,size,key,j,"length"));continue
  try:
   result=m.run(obj,R/"vuc/config/assemble_binop_dispatch_oct14_probe.json")
   scores.append((result["exact_bytes"],size,key,j,result["different_offsets"][:12]))
   if result["exact"]:
    print("DIAGNOSTIC_EXACT_NOT_ACCEPTED",key,j,cpp,flush=True);raise SystemExit(0)
  except ValueError as e:scores.append((0,size,key,j,str(e)[:90]))
 print("CHECKED",key,flush=True)
print("TOP")
for row in sorted(scores,reverse=True)[:24]:print(row,flush=True)
