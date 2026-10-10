#!/usr/bin/env python3
"""Explore VUC IsFoldable source idioms with preserved original ELF acceptance."""
from pathlib import Path
import subprocess
import importlib.util

ROOT=Path(__file__).resolve().parents[2]
TOOL=Path("/tmp/vuc-binutils215")
s=importlib.util.spec_from_file_location("vuc_match",ROOT/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
base=(ROOT/"vuc/probes/is_foldable.cpp").read_text()
start=base.index("bool IsFoldable")
samples={
"or-chain":"""bool IsFoldable(IR::Operator_Data *op) {
    return op->kind == 13 || op->kind == 12 || op->kind == 11 || op->kind == 10;
}""",
"bool-var":"""bool IsFoldable(IR::Operator_Data *op) {
    bool found = false;
    if (op->kind == 13) found=true;
    if (op->kind == 12) found=true;
    if (op->kind == 11) found=true;
    if (op->kind == 10) found=true;
    return found;
}""",
"nested":"""bool IsFoldable(IR::Operator_Data *op) {
    bool found=false;
    if (op->kind == 13) found = true;
    else if (op->kind == 12) found = true;
    else if (op->kind == 11) found = true;
    else if (op->kind == 10) found = true;
    return found;
}""",
"switch":"""bool IsFoldable(IR::Operator_Data *op) {
    switch (op->kind) {
    case 13: case 12: case 11: case 10: return true;
    default: return false;
    }
}""",
"switch-break":"""bool IsFoldable(IR::Operator_Data *op) {
    bool fold=false;
    switch (op->kind) {
    case 13: case 12: case 11: case 10: fold=true; break;
    default: break;
    }
    return fold;
}""",
"separate-switch":"""bool IsFoldable(IR::Operator_Data *op) {
    bool fold=false;
    switch (op->kind) {
    case 13: fold=true; break;
    case 12: fold=true; break;
    case 11: fold=true; break;
    case 10: fold=true; break;
    }
    return fold;
}""",
"equal-bitwise":"""bool IsFoldable(IR::Operator_Data *op) {
    return (op->kind==13) | (op->kind==12) | (op->kind==11) | (op->kind==10);
}""",
"array-static":"""bool IsFoldable(IR::Operator_Data *op) {
    const int candidates[]={13,12,11,10};
    for (int i=0;i<4;i++) if (candidates[i]==op->kind) return true;
    return false;
}""",
"if-return":"""bool IsFoldable(IR::Operator_Data *op) {
    if (op->kind==13) return true;
    if (op->kind==12) return true;
    if (op->kind==11) return true;
    if (op->kind==10) return true;
    return false;
}""",
"if-inverted":"""bool IsFoldable(IR::Operator_Data *op) {
    if (op->kind!=13)
        if (op->kind!=12)
            if (op->kind!=11)
                if (op->kind!=10)
                    return false;
    return true;
}""",
"volatile-local":"""bool IsFoldable(IR::Operator_Data *op) {
    volatile int kind=op->kind;
    bool found=false;
    if(kind==13 || kind==12 || kind==11 || kind==10) found=true;
    return found;
}""",
"switch-bool":"""bool IsFoldable(IR::Operator_Data *op) {
    bool ret=false;
    switch(op->kind) {
      case 10: ret=true; break;
      case 11: ret=true; break;
      case 12: ret=true; break;
      case 13: ret=true; break;
      default: ret=false; break;
    }
    return ret;
}"""
}
opts={
 "ren":["-O2","-frename-registers"],
 "ren-j1":["-O2","-frename-registers","-falign-jumps=1"],
 "plain":["-O2"],
 "plain-j1":["-O2","-falign-jumps=1"],
 "ren-os":["-Os","-frename-registers"],
}
results=[]
for name,body in samples.items():
 p=TOOL/("isfoldable-"+name+".cpp")
 p.write_text(base[:start]+body+"\n")
 for fkey,flags in opts.items():
  obj=TOOL/("isfoldable-"+name+"-"+fkey+".o")
  cp=subprocess.run([str(TOOL/"g++-jan25-as215"),*flags,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
  if cp.returncode:results.append((0,name,fkey,cp.stderr[-120:]));continue
  try:
   res=m.run(obj,ROOT/"vuc/config/is_foldable_probe.json")
   results.append((res["exact_bytes"],name,fkey,str(res["length"])))
  except Exception as e:results.append((0,name,fkey,str(e)[:75]))
results.sort(reverse=True)
for x in results[:28]: print(*x)
