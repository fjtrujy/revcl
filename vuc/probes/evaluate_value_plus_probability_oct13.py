#!/usr/bin/env python3
"""Use original C++ branch-likelihood expressions for Value addition.

The original and reconstructed addition share all arithmetic instructions,
but the original has a 3-byte branch-target alignment before its
single-component float case. __builtin_expect is a genuine period GCC
extension: test whether probable type or conversion outcomes change
the relevant layout without changing arithmetic semantics.
Keep already exact subtraction/multiplication as neighbor guards.
"""
from pathlib import Path
from itertools import product
import importlib.util,subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/value-plus-probability-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("matcher",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/"vuc/src/ValueArithmeticUnit.cpp").read_text()
assert base.count("if (TypeConvert(*this, other, result, temporary))")==3
assert base.count("switch (result.type)")==3
flags={
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j2":["-O2","-frename-registers","-falign-jumps=2"],
 "j3":["-O2","-frename-registers","-falign-jumps=3"],
 "j4":["-O2","-frename-registers","-falign-jumps=4"],
}
results=[]
for conversion,typemode in product(("none","likely","unlikely"),("none","one","three","four","zero","unlikely3")):
 source=base
 before=source.index("Value Value::operator+(")
 after=source.index("Value Value::operator-(",before)
 prefix=source[:before];body=source[before:after];suffix=source[after:]
 if conversion!="none":
  arg=1 if conversion=="likely" else 0
  body=body.replace("if (TypeConvert(*this, other, result, temporary))",
   "if (__builtin_expect(TypeConvert(*this, other, result, temporary), "+str(arg)+"))")
 if typemode!="none":
  expected={"one":1,"three":3,"four":4,"zero":0,"unlikely3":0}[typemode]
  if typemode=="unlikely3":
   body=body.replace("switch (result.type)", "switch (__builtin_expect(result.type == 3, 0) ? 3 : result.type)")
  else:
   body=body.replace("switch (result.type)", "switch (__builtin_expect(result.type, "+str(expected)+"))")
 candidate=prefix+body+suffix
 key=conversion+"-"+typemode
 p=T/(key+".cpp");p.write_text(candidate)
 for fl,opts in flags.items():
  o=T/(key+"-"+fl+".o")
  cc=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(p),"-o",str(o)],capture_output=True,text=True)
  if cc.returncode:print("COMPILER",key,fl,cc.stderr[-120:],flush=True);continue
  try:
   r=m.run(o,R/"vuc/config/value_arithmetic_plus_probe.json")
   results.append((r["exact_bytes"],key,fl,r["length"],r["different_offsets"][:12]))
   if r["exact"]:
    print("PLUS_EXACT",key,fl,p,flush=True)
    for other,conf in {"minus":"value_arithmetic_minus_probe.json","multiply":"value_multiply_exact_oct13.json"}.items():
     try:
      t=m.run(o,R/"vuc/config"/conf)
      print(other,t["exact_bytes"],t["length"],t["exact"],flush=True)
     except ValueError as e:print("OTHER_REJECT",other,str(e),flush=True)
  except ValueError as e:results.append((0,key,fl,str(e)[:75],[]))
 print("tested",key,flush=True)
print("TOP")
for result in sorted(results,reverse=True)[:25]:print(result)
