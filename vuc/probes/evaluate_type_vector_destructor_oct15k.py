#!/usr/bin/env python3
"""Test whether VUC Type's 525-byte vector helper implies a nontrivial destructor.

The 2004 original has explicit no-op loops and exception cleanup around
Type vector growth; the current reconstructed 16-byte Type's implicit trivial
destructor emits a 440-byte helper. Probe ordinary C++03 empty destructors
against the original original-length gate; do not manufacture byte padding.
"""
from pathlib import Path
import subprocess
import importlib.util
import json

ROOT=Path(__file__).resolve().parents[2]
VUC=ROOT/"vuc"
OUT=Path("/tmp/vuc-binutils215/type-vector-dtor-oct15k")
OUT.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("m",VUC/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
source=(VUC/"probes/TypeVectorInsertionRealTypeOct15k.cpp").read_text()
s_type=(VUC/"src/TypeUninitializedCopy.cpp").read_text()
# Localize include with the real accepted Type source, not a competing invented
# type model; change only its destructor declaration/definition.
s_type=s_type.replace("struct Type {", "struct Type {")
types={
 "implicit":s_type,
 "inline-empty":s_type.replace("    Type(const Type &other)", "    ~Type() {}\n    Type(const Type &other)"),
 "out-of-line-empty":s_type.replace("    Type(const Type &other)", "    ~Type();\n    Type(const Type &other)").replace("typedef std::vector<Type> TypeVector;", "Type::~Type() {}\ntypedef std::vector<Type> TypeVector;"),
}
symbol="_ZNSt6vectorI4TypeSaIS0_EE13_M_insert_auxEN9__gnu_cxx17__normal_iteratorIPS0_S2_EERKS0_"
config={
"function":symbol, "address":"0x0804ee10","size":525,
"source":"vuc/probes/TypeVectorInsertionRealTypeOct15k.cpp",
"flags":["-O2","-frename-registers"],
"relocations":{
"_ZSt24__uninitialized_copy_auxIN9__gnu_cxx17__normal_iteratorIP4TypeSt6vectorIS2_SaIS2_EEEES7_ET0_T_S9_S8_12__false_type":"0x0804f1c0",
"_ZNSt24__default_alloc_templateILb1ELi0EE10deallocateEPvj":"0x0804997c",
"_ZNSt24__default_alloc_templateILb1ELi0EE8allocateEj":"0x0804968c",
"__cxa_begin_catch":"0x0804995c", "__cxa_rethrow":"0x0804979c",
"__cxa_end_catch":"0x0804983c","_Unwind_Resume":"0x0804998c"}}
cfg=OUT/"type-vector-original.json"
cfg.write_text(json.dumps(config,indent=2)+"\n")
opts={
 "current":["-O2","-frename-registers"],
 "jump1":["-O2","-frename-registers","-falign-jumps=1"],
 "jump8-loop1":["-O2","-frename-registers","-falign-jumps=8","-falign-loops=1","-falign-labels=1"],
}
rows=[]
for name,part in types.items():
 cpp=OUT/(name+".cpp")
 cpp.write_text(part+"\n"+
    "template void std::vector<Type>::_M_insert_aux(TypeIterator, const Type &);\n")
 for flag,option in opts.items():
  obj=OUT/(name+"-"+flag+".o")
  cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*option,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  row={"source":name,"flags":flag}
  if cp.returncode:
   row["error"]=cp.stderr[-200:]
  else:
   try:
    s=m.run(obj,cfg)
    row.update(length=s["length"],matching=s["exact_bytes"],
               relocations=s["relocations"],diff=s["different_offsets"][:45])
   except ValueError as e:
    row["error"]=str(e)
  rows.append(row)
  print(row,flush=True)
(OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
