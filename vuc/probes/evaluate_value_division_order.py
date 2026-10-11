#!/usr/bin/env python3
"""Explore normal C++ switch case ordering for original Value division."""
from pathlib import Path
import itertools, subprocess, importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215")
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
raw=(R/"vuc/probes/value_division_reciprocal.cpp").read_text()
original=raw.index("        case 1: result.bits")
start=original
end=raw.index("\n        }\n    }\n    return result;", start)
prefix=raw[:start];suffix=raw[end:]
cases={
 "1": "        case 1: result.bits = (int)result.bits / (int)temporary.bits; break;",
 "3": "        case 3: result.values[0] /= temporary.values[0]; break;",
 "4": "        case 4:\n"+"\n".join("            result.values[%d] *= 1.0f / temporary.values[%d];"%(i,i) for i in range(4))+"\n            break;",
}
flags={
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j2":["-O2","-frename-registers","-falign-jumps=2"],
 "j3":["-O2","-frename-registers","-falign-jumps=3"],
 "j4":["-O2","-frename-registers","-falign-jumps=4"],
 "j8":["-O2","-frename-registers","-falign-jumps=8"],
 "j16":["-O2","-frename-registers","-falign-jumps=16"],
 "j1l4":["-O2","-frename-registers","-falign-jumps=1","-falign-labels=4"],
 "j1l8":["-O2","-frename-registers","-falign-jumps=1","-falign-labels=8"],
 "j1l16":["-O2","-frename-registers","-falign-jumps=1","-falign-labels=16"],
}
results=[]
for perm in itertools.permutations("134"):
 s=prefix+"\n".join(cases[c] for c in perm)+suffix
 name="".join(perm);p=T/("vdiv-case-"+name+".cpp");p.write_text(s)
 for f,opts in flags.items():
  obj=T/("vdiv-case-"+name+"-"+f+".o")
  cp=subprocess.run([str(T/"g++-jan25-as215"),*opts,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
  if cp.returncode:results.append((0,name,f,cp.stderr[-70:]));continue
  try:
   d=m.run(obj,R/"vuc/config/value_dv_probe.json")
   results.append((d["exact_bytes"],name,f,str(d["length"])))
  except Exception as e:results.append((0,name,f,str(e)[:70]))
  if results[-1][0]==235:print("EXACT",name,f,flush=True); break
results.sort(reverse=True)
for x in results[:26]:print(*x)
