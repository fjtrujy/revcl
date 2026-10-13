#!/usr/bin/env python3
"""Test whether the authentic preceding addition/subtraction methods suffice for exact multiply.

This intentionally removes unrelated Value members from the earlier larger
source-unit experiment, preserving exactly the genuine consecutive arithmetic
members found in original ELF symbol order. All 222/221 bytes and authentic
TypeConvert relocations must still match.
"""
from pathlib import Path
import importlib.util, subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/value-arithmetic-trio-oct13");T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
source=(R/"vuc/probes/value_ml.cpp").read_text()
source=source.replace("    Value operator*(const Value &other) const;", "    Value operator*(const Value &other) const;\n    Value operator-(const Value &other) const;")
minus=(R/"vuc/src/ValueMinus.cpp").read_text()
minus=minus[minus.index("Value Value::operator-("):]
spot=source.index("Value Value::operator*(")
source=source[:spot]+minus+"\n"+source[spot:]
path=T/"arithmetic-trio.cpp"
path.write_text(source)
for label,fl in {
 "r-j1":["-O2","-frename-registers","-falign-jumps=1"],
 "r":["-O2","-frename-registers"],
 "plain":["-O2","-falign-jumps=1"],
 "r-j4":["-O2","-frename-registers","-falign-jumps=4"],
}.items():
 obj=T/(label+".o")
 cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*fl,"-c",str(path),"-o",str(obj)],capture_output=True,text=True)
 if cp.returncode:print("COMPILE_ERROR",label,cp.stderr[-230:],flush=True);continue
 for symbol,config in (
  ("plus","value_arithmetic_plus_probe.json"),
  ("minus","value_arithmetic_minus_probe.json"),
  ("multiply","value_ml_probe.json"),
 ):
  try:
   score=m.run(obj,R/"vuc/config"/config)
   print(label,symbol,score["exact_bytes"],"/",score["length"],
     "relocs",score["relocations"],"diff",score["different_offsets"][:15],flush=True)
  except ValueError as e:
   print(label,symbol,"REJECT",str(e)[:100],flush=True)
