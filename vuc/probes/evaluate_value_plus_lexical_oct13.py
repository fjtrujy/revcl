#!/usr/bin/env python3
"""All genuine Value addition switch orders after VCL bbe5056.

The three intact (1,3,4) branches preserve addition semantics, with
original STL/ABI relocation checked by the strict matcher.
"""
from itertools import permutations
from pathlib import Path
import subprocess, importlib.util
R = Path(__file__).resolve().parents[2]
T = Path("/tmp/vuc-binutils215/value-plus-order-oct13")
T.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
raws={"isolated":(R/"vuc/probes/value_arithmetic.cpp").read_text(),
      "real":(R/"vuc/src/Value.cpp").read_text()}
raws["real"]=raws["real"].replace("    Value();","    Value() : type(0), a(0), b(0), flag0(0), flag1(0), flag2(0), flag3(0) {}").replace(
    "Value::Value() : type(0),a(0),b(0),flag0(0),flag1(0),flag2(0),flag3(0) {}","")
flags={"renj1":["-O2","-frename-registers","-falign-jumps=1"],
       "renj4":["-O2","-frename-registers","-falign-jumps=4"],
       "plainj1":["-O2","-falign-jumps=1"],
       "no-gcse":["-O2","-frename-registers","-falign-jumps=1","-fno-gcse"]}
ranks=[]
for ctx,code in raws.items():
    at=code.index("Value Value::operator+(")
    sw=code.index("switch (result.type)",at)
    end=code.index("\n        }\n",sw)
    first=code.index("\n        case 1:",sw)
    parts=code[first:end].split("\n        case ")
    assert len(parts)==4,parts
    cases={int(x.split(":",1)[0]):x for x in parts[1:]}
    assert set(cases)=={1,3,4}
    for order in permutations((1,3,4)):
        p=T/(ctx+"-"+"".join(map(str,order))+".cpp")
        p.write_text(code[:first]+"".join("\n        case "+cases[n] for n in order)+code[end:])
        for name,fl in flags.items():
            obj=T/(p.stem+"-"+name+".o")
            cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*fl,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
            if cp.returncode:
                print("ERROR",ctx,order,name,cp.stderr[-90:],flush=True);continue
            try:
                score=m.run(obj,R/"vuc/config/value_arithmetic_plus_probe.json")
                ranks.append((score["exact_bytes"],ctx,order,name,score["length"],score["different_offsets"]))
                if score["exact"]:
                    print("NEW_EXACT",ctx,order,name,p,flush=True);raise SystemExit(0)
            except ValueError as exc:
                ranks.append((0,ctx,order,name,str(exc)[:75],[]))
        print("tested",ctx,order,flush=True)
print("BEST")
for x in sorted(ranks,key=lambda z:z[0],reverse=True)[:23]:print(x)
