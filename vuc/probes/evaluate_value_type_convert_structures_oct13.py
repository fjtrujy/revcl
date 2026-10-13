#!/usr/bin/env python3
"""Infer the authentic Value::TypeConvert nested type-dispatch CFG.

The first 116 bytes of a genuine trivial-struct-copy C++ candidate already
reproduce the original register and memory copy sequence. The original
remaining dispatch uses signed ranges and ordered branches; test period
C++03 nested switches and if chains, preserving all integer/float semantics.
Never hardcode NOPs, instruction bytes, or caller addresses.
"""
from pathlib import Path
import importlib.util,subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/value-type-convert-structures-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
src=(R/"vuc/probes/value_type_convert_oct13.cpp").read_text()
at=src.index("    if (left.type == 1")
prefix=src[:at]
variants={}
variants["baseline"]=src[at:src.rindex("}")].rstrip()
variants["nested-switch"]="""
    switch (left.type) {
    case 1: case 2:
        switch (right.type) {
        case 1: case 2: right.type = 1; return true;
        case 3: left.values[0] = (float)left.bits; left.type = 3; return true;
        }
        return false;
    case 3:
        switch (right.type) {
        case 1: case 2: right.values[0] = (float)right.bits; right.type = 3; return true;
        }
        return false;
    }
    return false;"""
variants["nested-switch-reverse"]="""
    switch (left.type) {
    case 3:
        switch (right.type) {
        case 1: case 2: right.values[0] = (float)right.bits; right.type = 3; return true;
        }
        return false;
    case 1: case 2:
        switch (right.type) {
        case 1: case 2: right.type = 1; return true;
        case 3: left.values[0] = (float)left.bits; left.type = 3; return true;
        }
        return false;
    }
    return false;"""
variants["range-ordered"]="""
    if (left.type < 1) return false;
    if (left.type <= 2) {
        if (right.type < 1) return false;
        if (right.type <= 2) { right.type = 1; return true; }
        if (right.type != 3) return false;
        left.values[0] = (float)left.bits;
        left.type = 3;
        return true;
    }
    if (left.type != 3) return false;
    if ((unsigned)(right.type - 1) > 1) return false;
    right.values[0] = (float)right.bits;
    right.type = 3;
    return true;"""
variants["range-ordered-reverse"]="""
    if (left.type < 1) return false;
    if (left.type <= 2) {
        if (right.type < 1) return false;
        if (right.type <= 2) { right.type = 1; return true; }
        if (right.type != 3) return false;
        left.type = 3;
        left.values[0] = (float)left.bits;
        return true;
    }
    if (left.type != 3) return false;
    if ((unsigned)(right.type - 1) > 1) return false;
    right.type = 3;
    right.values[0] = (float)right.bits;
    return true;"""
variants["range-conditional"]="""
    if (left.type >= 1 && left.type <= 2) {
        if (right.type >= 1 && right.type <= 2) {
            right.type = 1;
            return true;
        }
        if (right.type == 3) {
            left.values[0] = (float)left.bits;
            left.type = 3;
            return true;
        }
    } else if (left.type == 3 && right.type >= 1 && right.type <= 2) {
        right.values[0] = (float)right.bits;
        right.type = 3;
        return true;
    }
    return false;"""
variants["range-unconditional"]="""
    int firstType = left.type;
    int secondType = right.type;
    if (firstType < 1) return false;
    if (firstType <= 2) {
        if (secondType < 1) return false;
        if (secondType <= 2) { right.type = 1; return true; }
        if (secondType == 3) {
            left.values[0] = (float)left.bits;
            left.type = 3;
            return true;
        }
        return false;
    }
    if (firstType == 3 && (unsigned)(secondType - 1) <= 1) {
        right.values[0] = (float)right.bits;
        right.type = 3;
        return true;
    }
    return false;"""
variants["switch-and-range"]="""
    switch (left.type) {
    case 1: case 2:
        if (right.type >= 1 && right.type <= 2) {
            right.type = 1; return true;
        }
        if (right.type == 3) {
            left.values[0] = (float)left.bits; left.type = 3; return true;
        }
        return false;
    case 3:
        if ((unsigned)(right.type - 1) > 1) return false;
        right.values[0] = (float)right.bits; right.type = 3; return true;
    }
    return false;"""
variants["if-switch-inner"]="""
    if (left.type < 1 || left.type > 3) return false;
    if (left.type == 3) {
        if (right.type != 1 && right.type != 2) return false;
        right.values[0] = (float)right.bits; right.type = 3; return true;
    }
    switch (right.type) {
    case 1: case 2: right.type = 1; return true;
    case 3: left.values[0] = (float)left.bits; left.type = 3; return true;
    }
    return false;"""
flags={
    "r-j1":["-O2","-frename-registers","-falign-jumps=1"],
    "r-j2":["-O2","-frename-registers","-falign-jumps=2"],
    "r-j4":["-O2","-frename-registers","-falign-jumps=4"],
    "r":["-O2","-frename-registers"],
    "plain-j1":["-O2","-falign-jumps=1"],
}
rank=[]
for label,body in variants.items():
 p=T/(label+".cpp")
 p.write_text(prefix+body+"\n}\n")
 for tag,opt in flags.items():
  o=T/(label+"-"+tag+".o")
  b=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opt,"-c",str(p),"-o",str(o)],capture_output=True,text=True)
  if b.returncode:print("COMPILE_ERROR",label,tag,b.stderr[-130:],flush=True);continue
  output=subprocess.check_output(["nm","-S",str(o)],text=True)
  row=next((s for s in output.splitlines() if s.endswith("_ZN5Value11TypeConvertERKS_S1_RS_S2_")),"")
  size=int(row.split()[1],16) if row else -1
  if size==220:
   try:
    score=m.run(o,R/"vuc/config/value_type_convert_oct13_probe.json")
    rank.append((score["exact_bytes"],label,tag,size,score["different_offsets"][:22]))
    if score["exact"]:print("NEW_EXACT",label,tag,p,flush=True)
   except ValueError as e:rank.append((0,label,tag,size,str(e)[:100]))
  else:rank.append((0,label,tag,size,[]))
 print("checked",label,flush=True)
print("TOP")
for row in sorted(rank,key=lambda a:a[0],reverse=True)[:32]:print(row)
