#!/usr/bin/env python3
"""Historical VUC Value division operand-order reconstruction.

The original 235-byte function's case-3 scalar operation loads the destination
before performing fdivs on the source. GCC transforms an ordinary '/=' into
source-first fdivr; compare normal C++ expressions and lvalue lifetimes while
retaining the genuine plus/minus/multiply predecessors and their ABI.
"""
from pathlib import Path
from itertools import product
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/divide-expression-oct13")
T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
unit=(R/"vuc/src/ValueArithmeticUnit.cpp").read_text()
unit=unit.replace("    Value operator*(const Value &other) const;","    Value operator*(const Value &other) const;\n    Value operator/(const Value &other) const;")
other=(R/"vuc/probes/value_division_reciprocal.cpp").read_text()
division=other[other.index("Value Value::operator/("):]
scalar="result.values[0] /= temporary.values[0];"
forms={
 "compound": scalar,
 "assignment": "result.values[0] = result.values[0] / temporary.values[0];",
 "reciprocal": "result.values[0] *= 1.0f / temporary.values[0];",
 "denom-local": "float divisor = temporary.values[0]; result.values[0] /= divisor;",
 "numer-local": "float numerator = result.values[0]; result.values[0] = numerator / temporary.values[0];",
 "both-local": "float numerator = result.values[0]; float divisor = temporary.values[0]; result.values[0] = numerator / divisor;",
 "float-pointer": "float *left = &result.values[0]; *left = *left / temporary.values[0];",
 "division-pointer": "const float *right = &temporary.values[0]; result.values[0] /= *right;",
 "divide-temp": "const float quotient = result.values[0] / temporary.values[0]; result.values[0] = quotient;",
 "reciprocal-temp": "const float reciprocal = 1.0f / temporary.values[0]; result.values[0] *= reciprocal;",
}
flags={
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j4":["-O2","-frename-registers","-falign-jumps=4"],
 "j8":["-O2","-frename-registers","-falign-jumps=8"],
 "j1-label4":["-O2","-frename-registers","-falign-jumps=1","-falign-labels=4"],
 "plain":["-O2","-falign-jumps=1"],
}
rank=[]
for name,scalar_src in forms.items():
 body=division.replace(scalar,scalar_src)
 assert body!=division or name=="compound"
 if name not in ("compound", "assignment", "reciprocal"):
  # C++ case labels cannot jump across initialized locals. Keep each
  # genuine temporary scoped to the scalar case, not to the switch.
  body=body.replace("case 3: "+scalar_src+" break;", "case 3: { "+scalar_src+" break; }")
 src=T/(name+".cpp");src.write_text(unit+"\n"+body)
 for flag,options in flags.items():
  obj=T/(name+"-"+flag+".o")
  p=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*options,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
  if p.returncode:
   print("ERROR",name,flag,p.stderr[-140:],flush=True);continue
  output=subprocess.check_output(["nm","-S",str(obj)],text=True)
  symbol=next((line for line in output.splitlines() if line.endswith("_ZNK5ValuedvERKS_")),"")
  size=int(symbol.split()[1],16) if symbol else -1
  try:
   score=m.run(obj,R/"vuc/config/value_dv_probe.json")
   rank.append((score["exact_bytes"],size,name,flag,score["different_offsets"][:12]))
   if score["exact"]:
    print("NEW_EXACT",name,flag,str(src),flush=True)
    raise SystemExit(0)
  except ValueError as e:rank.append((0,size,name,flag,str(e)[:100]))
 print("DONE",name,flush=True)
print("TOP")
for item in sorted(rank,reverse=True)[:35]:print(item)
