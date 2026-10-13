#!/usr/bin/env python3
"""Test original Value division after verified +, -, * historical predecessors.

The original VUC symbol sequence has division at 0x0806ec50 immediately
after newly exact multiplication. Keep preceding source authentic and
vary only normal lexical orders of three real division arms.
"""
import importlib.util, subprocess
from itertools import permutations
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/value-div-real-prefix-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
prefix=(R/"vuc/src/ValueArithmeticUnit.cpp").read_text()
assert "Value Value::operator*(" in prefix
prefix=prefix.replace("    Value operator*(const Value &other) const;","    Value operator*(const Value &other) const;\n    Value operator/(const Value &other) const;")
div=(R/"vuc/probes/value_division_reciprocal.cpp").read_text()
div=div[div.index("Value Value::operator/("):]
cases_start=div.index("\n        case 1:")
cases_end=div.index("\n        }\n",cases_start)
parts=div[cases_start:cases_end].split("\n        case ")
assert len(parts)==4
arms={int(x.split(":",1)[0]):x for x in parts[1:]}
assert set(arms)=={1,3,4}
flags={
 "r-j1":["-O2","-frename-registers","-falign-jumps=1"],
 "r-j4":["-O2","-frename-registers","-falign-jumps=4"],
 "r-j8":["-O2","-frename-registers","-falign-jumps=8"],
 "plain-j1":["-O2","-falign-jumps=1"],
}
rank=[]
for order in permutations((1,3,4)):
 changed=div[:cases_start]+"".join("\n        case "+arms[x] for x in order)+div[cases_end:]
 path=T/("division-"+''.join(map(str,order))+".cpp")
 path.write_text(prefix+"\n"+changed)
 for name,opts in flags.items():
  obj=T/(path.stem+"-"+name+".o")
  proc=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(path),"-o",str(obj)],capture_output=True,text=True)
  if proc.returncode:print("COMPILE_FAIL",order,name,proc.stderr[-160:],flush=True);continue
  try:
   score=m.run(obj,R/"vuc/config/value_dv_probe.json")
   rank.append((score["exact_bytes"],order,name,score["length"],score["different_offsets"][:12]))
   if score["exact"]:
    print("NEW_EXACT_DIVISION",order,name,path,flush=True)
    raise SystemExit(0)
  except ValueError as e:rank.append((0,order,name,str(e)[:100],[]))
 print("checked",order,flush=True)
print("TOP")
for row in sorted(rank,reverse=True)[:24]:print(row)
