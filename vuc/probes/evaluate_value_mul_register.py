#!/usr/bin/env python3
"""Investigate natural C++ register selection for original Value multiplication."""
from pathlib import Path
import importlib.util, subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215")
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
original=(R/"vuc/probes/value_ml.cpp").read_text()
old="case 1: result.bits *= temporary.bits; break;"
assert old in original
forms={
  "compound": "result.bits *= temporary.bits;",
  "result-first": "result.bits = result.bits * temporary.bits;",
  "temp-first": "result.bits = temporary.bits * result.bits;",
  "signed-compound": "*(int*)&result.bits *= *(int*)&temporary.bits;",
  "signed-result": "result.bits = (int)result.bits * (int)temporary.bits;",
  "tmp-copy": "unsigned tmp = temporary.bits; result.bits *= tmp;",
  "value-copy": "unsigned val = result.bits; result.bits = val * temporary.bits;",
  "value-copy-reversed": "unsigned val = result.bits; result.bits = temporary.bits * val;",
  "tmp-and-value": "unsigned temp = temporary.bits; unsigned val = result.bits; result.bits = val * temp;",
  "tmp-and-value-reversed": "unsigned temp = temporary.bits; unsigned val = result.bits; result.bits = temp * val;",
  "pointer": "unsigned *out = &result.bits; *out *= temporary.bits;",
  "pointer-reversed": "unsigned *out = &result.bits; *out = temporary.bits * *out;",
  "int-copy": "int tmp = temporary.bits; result.bits = (int)result.bits * tmp;",
  "volatile-cast": "result.bits = (unsigned)((int)result.bits * (int)temporary.bits);"
}
flags={
  "renj1": ["-O2","-frename-registers","-falign-jumps=1"],
  "renj3": ["-O2","-frename-registers","-falign-jumps=3"],
  "renj4": ["-O2","-frename-registers","-falign-jumps=4"],
  "plainj1": ["-O2","-falign-jumps=1"],
  "ren-no-gcse":["-O2","-frename-registers","-falign-jumps=1","-fno-gcse"],
  "ren-no-sched":["-O2","-frename-registers","-falign-jumps=1","-fno-schedule-insns2"],
}
results=[]
for name,expr in forms.items():
 cpp=T/("vmul-"+name+".cpp")
 cpp.write_text(original.replace(old, "case 1: "+expr+" break;"))
 for key,opts in flags.items():
  out=T/("vmul-"+name+"-"+key+".o")
  cp=subprocess.run([str(T/"g++-jan25-as215"),*opts,"-c",str(cpp),"-o",str(out)],capture_output=True,text=True)
  if cp.returncode:results.append((0,name,key,cp.stderr[-70:])); continue
  try:
   r=m.run(out,R/"vuc/config/value_ml_probe.json")
   results.append((r["exact_bytes"],name,key,str(r["length"])))
  except Exception as e:results.append((0,name,key,str(e)[:70]))
  if results[-1][0]==222:
   print("EXACT",name,key,flush=True)
   break
results.sort(reverse=True)
for r in results[:30]:print(*r)
