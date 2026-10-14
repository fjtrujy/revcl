#!/usr/bin/env python3
"""Reconstruct VUC's 193-byte AssembleBinOp dispatcher as genuine C++03.

Recover source-level dispatch topology and ABI to explain the four distinct
original cases. Original function, MLGameString ctor, type and callee
relocations are checked by match_probe. All outputs are VM scratch files.
"""
from itertools import permutations,product
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/assemble-binop-dispatch-oct14");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
original=(R/"vuc/probes/assemble_binop_dispatch_oct14.cpp").read_text()
prefix=original[:original.index("IR::Register AssembleBinOp(")]
prefix=prefix.replace('#include "../src/MLGameString.h"',
                      '#include "'+str(R/'vuc/src/MLGameString.h')+'"')
signature="""IR::Register AssembleBinOp(const IR::Register &target,
                            IR::Expr_Data *expr,
                            InstructionList &instructions)
{
"""
def dispatch(kind):
    return "AssembleBinOpInt" if kind in (1,2) else "AssembleBinOpFloat4"
forms={}
for order in permutations((1,2,3,4)):
    name="".join(map(str,order))
    switch="    switch (GetOpType(expr)->kind) {\n"
    for case in order:
        switch+="    case %d: return %s(target, expr, instructions);\n"%(case,dispatch(case))
    forms["switch-"+name]=switch+"    default: return target;\n    }\n"
for order in permutations((1,2,3,4)):
    name="".join(map(str,order))
    blocks="    const int kind = GetOpType(expr)->kind;\n"
    for c in order:
        blocks+="    if (kind == %d) return %s(target, expr, instructions);\n"%(c,dispatch(c))
    forms["if-"+name]=blocks+"    return target;\n"
for name, grouping in {
    "switch-grouped":((1,2),(3,4)),
    "switch-alt-grouped":((3,4),(1,2)),
}.items():
    cases="    switch (GetOpType(expr)->kind) {\n"
    for group in grouping:
        for c in group: cases+="    case %d:\n"%c
        cases+="        return %s(target, expr, instructions);\n"%dispatch(group[0])
    forms[name]=cases+"    default: return target;\n    }\n"
for name, style in {
    "switch-local-result":"""    IR::Register result(target);
    switch (GetOpType(expr)->kind) {
    case 1: case 2: result=AssembleBinOpInt(target,expr,instructions);break;
    case 3: case 4: result=AssembleBinOpFloat4(target,expr,instructions);break;
    }
    return result;
""",
    "if-range-branches":"""    int kind=GetOpType(expr)->kind;
    if (kind==3 || kind==4) return AssembleBinOpFloat4(target,expr,instructions);
    if (kind==2 || kind==1) return AssembleBinOpInt(target,expr,instructions);
    return target;
""",
    "if-groups-inverse":"""    int kind=GetOpType(expr)->kind;
    if (kind==1 || kind==2) return AssembleBinOpInt(target,expr,instructions);
    if (kind==3 || kind==4) return AssembleBinOpFloat4(target,expr,instructions);
    return target;
""",
    "switch-break":"""    int kind=GetOpType(expr)->kind;
    switch(kind) {
    case 1: {IR::Register tmp=AssembleBinOpInt(target,expr,instructions); return tmp;}
    case 2: {IR::Register tmp=AssembleBinOpInt(target,expr,instructions); return tmp;}
    case 3: {IR::Register tmp=AssembleBinOpFloat4(target,expr,instructions); return tmp;}
    case 4: {IR::Register tmp=AssembleBinOpFloat4(target,expr,instructions); return tmp;}
    default: return target;
    }
""",
}.items():forms[name]=style
flags={
"rn":["-O2","-frename-registers"],
"j1":["-O2","-frename-registers","-falign-jumps=1"],
"j4":["-O2","-frename-registers","-falign-jumps=4"],
"nr":["-O2"],
"noreorder":["-O2","-frename-registers","-fno-reorder-blocks"],
"nocross":["-O2","-frename-registers","-fno-crossjumping"],
}
scores=[]
for name,body in forms.items():
    cpp=T/(name+".cpp");cpp.write_text(prefix+signature+body+"}\n")
    for flag,opts in flags.items():
        obj=T/(name+"-"+flag+".o")
        cc=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
        if cc.returncode:
            print("ERROR",name,flag,cc.stderr[-100:],flush=True);continue
        nm=subprocess.check_output(["nm","-S",str(obj)],text=True)
        rec=next((x for x in nm.splitlines() if x.endswith("_Z13AssembleBinOpRKN2IR8RegisterEPNS_9Expr_DataER15InstructionList")),"")
        size=int(rec.split()[1],16) if rec else -1
        if size!=193:
            scores.append((0,size,name,flag,"length"));continue
        try:
            score=m.run(obj,R/"vuc/config/assemble_binop_dispatch_oct14_probe.json")
            scores.append((score["exact_bytes"],size,name,flag,score["different_offsets"][:14]))
            if score["exact"]:
                print("NEW_EXACT",name,flag,cpp,flush=True)
                raise SystemExit(0)
        except ValueError as e:
            scores.append((0,size,name,flag,"reject "+str(e)[:85]))
    if name.startswith("if-") and name.endswith("4321"):print("DONE IF",flush=True)
    if name.startswith("switch-") and name.endswith("4321"):print("DONE SWITCH",flush=True)
print("TOP")
for item in sorted(scores,reverse=True)[:35]:print(item,flush=True)
