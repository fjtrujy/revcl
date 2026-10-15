#!/usr/bin/env python3
"""Compare original-size Type stream operator with plausible Type flags.

The original 16-byte Type layout is grounded by code and accepted
Type::IsAssignable; its four high-end bytes may be bool/char/bitfield
declarations. Use authentic period-C++ field declarations, not compiler
flag sweeps, and keep original type-name table, relocation provenance
and type parsing semantics.
"""
from pathlib import Path
import importlib.util
import subprocess
import json

R=Path(__file__).resolve().parents[2]/"vuc"
O=Path("/tmp/vuc-binutils215/type-input-declared-flags-oct15n")
O.mkdir(parents=True,exist_ok=True)
s=(R/"probes/TypeStreamInputOriginalUnitOct15n.cpp").read_text()
needle="    unsigned char parsed, is_const, zero_array, auxiliary;"
assert needle in s
styles={
 "uchar":needle,
 "bool-all":"    bool parsed, is_const, zero_array, auxiliary;",
 "char-all":"    char parsed, is_const, zero_array, auxiliary;",
 "bool-separate":"    bool parsed; bool is_const; bool zero_array; unsigned char auxiliary;",
 "mixed-bools":"    unsigned char parsed; bool is_const; bool zero_array; unsigned char auxiliary;",
 "mixed-aux-bool":"    unsigned char parsed, is_const, zero_array; bool auxiliary;",
 "bool-first":"    bool parsed; unsigned char is_const, zero_array, auxiliary;",
 "char-first":"    char parsed; unsigned char is_const, zero_array, auxiliary;",
 "bitfields":"    unsigned parsed:8, is_const:8, zero_array:8, auxiliary:8;",
}
spec=importlib.util.spec_from_file_location("matcher",R/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
conf=R/"config/type_stream_input_partial_oct15n.json"
results=[]
for label,decl in styles.items():
 cpp=O/(label+".cpp")
 cpp.write_text(s.replace(needle,decl))
 obj=O/(label+".o")
 p=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215","-O2","-frename-registers","-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 row={"declaration":label}
 if p.returncode:row["error"]=p.stderr[-165:]
 else:
  try:
   score=m.run(obj,conf)
   row.update(length=score["length"],match=score["exact_bytes"],
              relocations=score["relocations"],literals=score["verified_literal_bytes"],
              different_offsets=score["different_offsets"][:100])
  except ValueError as e:row["error"]=str(e)
 results.append(row)
 print(label,row.get("length"),row.get("match"),row.get("error",""),flush=True)
(O/"results.json").write_text(json.dumps(results,indent=2)+"\n")
