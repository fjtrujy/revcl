#!/usr/bin/env python3
"""Instruction-led original C++03 Type stream input local lifetime variants.

The authentic original 369-byte operator>>(istream&,Type&) initializes
Type's 16-byte local in order array_length, reserve, byte flags, kind,
and then copies its fields to the result via real loads. The prior
369-byte candidate differs mainly in that initialization/copy prefix;
the rest of its code (+145..+343) is original-offset matching. Search
plausible original Type constructor and copy-assignment shapes only,
not compiler flag grids. Never mutate accepted application code.
"""
from pathlib import Path
import importlib.util
import json
import itertools
import subprocess
R=Path(__file__).resolve().parents[2]/"vuc"
O=Path("/tmp/vuc-binutils215/type-input-local-ctor-oct15n")
O.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("match",R/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
source=(R/"probes/type_stream_input_reconstruction_oct12.cpp").read_text()
anchor="""    Type() : kind(0), array_length(0), reserve(0),
             parsed(0), is_const(0), zero_array(0), auxiliary(0) {}"""
assert anchor in source
copy="""    Type& operator=(const Type &rhs) {
        kind = rhs.kind; array_length = rhs.array_length;
        reserve = rhs.reserve; parsed = rhs.parsed;
        is_const = rhs.is_const; zero_array = rhs.zero_array;
        auxiliary = rhs.auxiliary; return *this;
    }"""
ctors={
 "init-list":anchor,
 "kind-body":"""    Type() : array_length(0), reserve(0),
             parsed(0), is_const(0), zero_array(0), auxiliary(0) { kind=0; }""",
 "kind-first-body":"""    Type() : array_length(0), reserve(0),
             parsed(0), is_const(0), zero_array(0), auxiliary(0) { kind=0; }""",
 "all-body":"""    Type() {
        kind=0; array_length=0; reserve=0;
        parsed=0; is_const=0; zero_array=0; auxiliary=0;
    }""",
 "reverse-body":"""    Type() {
        array_length=0; reserve=0; parsed=0; is_const=0;
        zero_array=0; auxiliary=0; kind=0;
    }""",
 "all-body-flag-first":"""    Type() {
        parsed=0; is_const=0; zero_array=0; auxiliary=0;
        kind=0; array_length=0; reserve=0;
    }""",
 "partial-init":"""    Type() : kind(0), array_length(0), reserve(0) {
        parsed=0; is_const=0; zero_array=0; auxiliary=0;
    }""",
 "flags-init-first":"""    Type() : parsed(0), is_const(0), zero_array(0), auxiliary(0) {
        array_length=0; reserve=0; kind=0;
    }""",
 "kind-overwrite":"""    Type() : kind(0), array_length(0), reserve(0),
             parsed(0), is_const(0), zero_array(0), auxiliary(0) { kind=0; }""",
}
assign={"implicit":"","custom":copy,
        "custom-grouped":"""    Type& operator=(const Type &rhs) {
         kind=rhs.kind; array_length=rhs.array_length; reserve=rhs.reserve;
         unsigned *word=(unsigned *)&parsed;
         *word=*(const unsigned *)&rhs.parsed;
         return *this;
    }"""}
cfg=R/"config/type_stream_input_reconstruction_oct12_probe.json"
out=[]
for (name,ctor),(choice,copybody) in itertools.product(ctors.items(),assign.items()):
 tag=name+"-"+choice
 cpp=O/(tag+".cpp")
 cpp.write_text(source.replace(anchor,ctor+"\n"+copybody))
 obj=O/(tag+".o")
 cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215","-O2","-frename-registers","-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 row={"name":tag}
 if cp.returncode:
  row["error"]=cp.stderr[-150:]
 else:
  try:
   x=m.run(obj,cfg)
   row.update(length=x["length"],matched=x["exact_bytes"],relocs=x["relocations"],
        literal_bytes=x["verified_literal_bytes"],diff=x["different_offsets"][:110])
  except ValueError as e:row["error"]=str(e)
 out.append(row)
 print(tag,row.get("length"),row.get("matched"),row.get("error",""),flush=True)
(O/"results.json").write_text(json.dumps(out,indent=2)+"\n")
