#!/usr/bin/env python3
"""Bounded GCC 3.3 historical CFG alignment study for AssembleBinOp.

First pass found 131/193 relocation-verified bytes for four independent
if arms ordered 3,4,2,1. Test branch/label/loop alignment and genuine
condition lifetime, without padding, inline asm, fake predecessor functions.
"""
from itertools import product
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/assemble-dispatch-layout-oct14");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/"vuc/probes/assemble_binop_dispatch_oct14.cpp").read_text()
prefix=base[:base.index("IR::Register AssembleBinOp(")].replace(
 '#include "../src/MLGameString.h"', '#include "'+str(R/'vuc/src/MLGameString.h')+'"')
sig="""IR::Register AssembleBinOp(const IR::Register &target,
                            IR::Expr_Data *expr,
                            InstructionList &instructions)
{
"""
conds={
"int-code": "    int code = GetOpType(expr)->kind;\n",
"unsigned-code":"    unsigned code = GetOpType(expr)->kind;\n",
"const-int-code":"    const int code = GetOpType(expr)->kind;\n",
"type-pointer":"    IR::Type_Data *type = GetOpType(expr);\n    int code = type->kind;\n",
}
cases=("    if (code == 3) return AssembleBinOpFloat4(target, expr, instructions);\n"
       "    if (code == 4) return AssembleBinOpFloat4(target, expr, instructions);\n"
       "    if (code == 2) return AssembleBinOpInt(target, expr, instructions);\n"
       "    if (code == 1) return AssembleBinOpInt(target, expr, instructions);\n"
       "    return target;\n")
variants={}
for name,local in conds.items():
 variants[name]=local+cases
variants[name+"-else"]=local+cases.replace("    if (code == 4)", "    else if (code == 4)").replace(
 "    if (code == 2)", "    else if (code == 2)").replace(
 "    if (code == 1)", "    else if (code == 1)")
variants["early-default"]=conds["int-code"]+cases.replace("    return target;", "    return IR::Register(target);")
# Equivalent type-kind checks with period GCC builtin expectation; prediction
# can change alignment only when original predicates justify it.
for value in (0,1):
 for arm in (1,2,3,4):
  variants[f"expect-{arm}-{value}"]=conds["int-code"]+cases.replace(
   f"code == {arm}",f"__builtin_expect(code == {arm}, {value})")
flags={}
for j,l,loop in product((1,2,3,4,8),(1,2,4,8),(1,4,16)):
 flags[f"j{j}-label{l}-loop{loop}"]=[
  "-O2","-frename-registers",f"-falign-jumps={j}",
  f"-falign-labels={l}",f"-falign-loops={loop}"
 ]
scores=[]
for name,body in variants.items():
 cpp=T/(name+".cpp");cpp.write_text(prefix+sig+body+"}\n")
 for fl,opts in flags.items():
  obj=T/(name+"-"+fl+".o")
  cc=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,
    "-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  if cc.returncode:
   print("ERROR",name,fl,cc.stderr[-80:],flush=True);continue
  line=next((x for x in subprocess.check_output(["nm","-S",str(obj)],text=True).splitlines()
    if x.endswith("_Z13AssembleBinOpRKN2IR8RegisterEPNS_9Expr_DataER15InstructionList")),"")
  size=int(line.split()[1],16) if line else -1
  if size!=193:
   scores.append((0,size,name,fl,"length"));continue
  try:
   r=m.run(obj,R/"vuc/config/assemble_binop_dispatch_oct14_probe.json")
   scores.append((r["exact_bytes"],size,name,fl,r["different_offsets"][:10]))
   if r["exact"]:
    print("NEW_EXACT",name,fl,cpp,flush=True);raise SystemExit(0)
  except ValueError as e:scores.append((0,size,name,fl,"reject "+str(e)[:80]))
 print("CHECKED",name,flush=True)
print("TOP")
for x in sorted(scores,reverse=True)[:45]:print(x,flush=True)
