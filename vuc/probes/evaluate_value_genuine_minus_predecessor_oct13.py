#!/usr/bin/env python3
"""Test genuine Value::operator- predecessor context for 222-byte multiplication.

The original VUC ELF orders Value::operator+ (0x0806e9b0),
Value::operator- (0x0806ea90) and Value::operator* (0x0806eb70).
Prior near-exact multiply probes omitted the genuine, already exact
subtraction predecessor. VCL bbe5056 shows genuine lexical source
neighbors can affect GCC peephole2/renaming. All sources are C++03.
"""
from pathlib import Path
from itertools import permutations
import importlib.util, subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/value-plus-minus-mul-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/"vuc/src/Value.cpp").read_text()
base=base.replace("    Value();","    Value() : type(0), a(0), b(0), flag0(0), flag1(0), flag2(0), flag3(0) {}").replace(
"Value::Value() : type(0),a(0),b(0),flag0(0),flag1(0),flag2(0),flag3(0) {}","")
base=base.replace("    Value operator*(const Value &other) const;", "    Value operator*(const Value &other) const;\n    Value operator-(const Value &other) const;")
minus=(R/"vuc/src/ValueMinus.cpp").read_text()
minus=minus[minus.index("Value Value::operator-("):]
def reordered_cpp(body, symbol, order):
    start=body.index(symbol)
    block=body.index("switch (result.type)",start)
    end=body.index("\n        }\n",block)
    first=body.index("\n        case 1:",block)
    raw=body[first:end].split("\n        case ")
    assert len(raw)==4,raw
    arms={int(x.split(":",1)[0]):x for x in raw[1:]}
    assert set(arms)=={1,3,4}
    return body[:first]+"".join("\n        case "+arms[x] for x in order)+body[end:]
opts={"base":["-O2","-frename-registers","-falign-jumps=1"],
      "no-gcse":["-O2","-frename-registers","-falign-jumps=1","-fno-gcse"],
      "j4":["-O2","-frename-registers","-falign-jumps=4"]}
rank=[]
for mode in ("minus-cases","plus-cases","mul-cases"):
 for order in permutations((1,3,4)):
    mb=minus
    bb=base
    if mode=="minus-cases":
      mb=reordered_cpp(minus,"Value Value::operator-(",order)
    elif mode=="plus-cases":
      bb=reordered_cpp(base,"Value Value::operator+(",order)
    else:
      bb=reordered_cpp(base,"Value Value::operator*(",order)
    # Preserve authentic original function definition order.
    insertion=bb.index("Value Value::operator*(")
    src=T/(mode+"-"+"".join(map(str,order))+".cpp")
    src.write_text(bb[:insertion]+mb+"\n"+bb[insertion:])
    for key,flags in opts.items():
      obj=T/(src.stem+"-"+key+".o")
      cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flags,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
      if cp.returncode:print("COMPILE_ERROR",src.stem,key,cp.stderr[-160:],flush=True);continue
      try:
       mul=m.run(obj,R/"vuc/config/value_ml_probe.json")
       minus_result=m.run(obj,R/"vuc/config/value_arithmetic_minus_probe.json")
       rank.append((mul["exact_bytes"],minus_result["exact_bytes"],src.stem,key,mul["different_offsets"]))
       if mul["exact"]:
        print("MUL_BYTE_EXACT",src,key,"minus:",minus_result["exact"],flush=True)
      except ValueError as e:
       print("REJECT",src.stem,key,str(e)[:130],flush=True)
    print("checked",src.stem,flush=True)
print("TOP")
for row in sorted(rank,reverse=True)[:25]:print(row)
