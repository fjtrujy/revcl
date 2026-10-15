#!/usr/bin/env python3
"""Reconstruct original GCC Type TU symbol order from original VUC ELF.

ELF order at 0x804ee10: _M_insert_aux first (525), then
uninitialized Type const copy (79) at 0x804f0e0, then mutable copy
(86) at 0x804f1c0. Test ordering of legitimate explicit instantiations
to preserve correct historical compiler source-unit template ownership.
"""
from pathlib import Path
import subprocess,importlib.util,json
R=Path(__file__).resolve().parents[1]
OUT=Path("/tmp/vuc-binutils215/type-vector-original-inst-order-oct15s")
OUT.mkdir(exist_ok=True)
base=(R/"src/TypeUninitializedCopy.cpp").read_text()
first=base.index("template Type *std::__uninitialized_copy_aux")
header=base[:first]
copy_instances=base[first:]
insert="template void TypeVector::_M_insert_aux(TypeIterator,const Type&);\n"
assert copy_instances.count("template")>=2
spec=importlib.util.spec_from_file_location("m",R/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
candidates={
 "insert":"type_vector_insert_aux_partial_oct15s.json",
 "const-copy":"uninitialized_type_const_probe.json",
 "mut-copy":"uninitialized_type_mut_probe.json"
}
rows=[]
for order,source in {
 "actual-original":header+insert+copy_instances,
 "actual-original-default":header.replace('    Type(const Type &other)','    Type() {}\n    Type(const Type &other)')+insert+copy_instances,
 "copies-first":header+copy_instances+insert,
 "copies-first-default":header.replace('    Type(const Type &other)','    Type() {}\n    Type(const Type &other)')+copy_instances+insert,
}.items():
 cpp=OUT/(order+".cpp");cpp.write_text(source)
 for snap in ("jan25","dec06"):
  obj=OUT/(order+"-"+snap+".o")
  p=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215","-O2","-frename-registers","-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  row={"order":order,"compiler":snap}
  if p.returncode:row["error"]=p.stderr[-170:]
  else:
   for tag,filename in candidates.items():
    try:
     v=m.run(obj,R/"config"/filename)
     row[tag]=(v["length"],v["exact_bytes"],v["relocations"])
    except ValueError as e:row[tag]=str(e)
  rows.append(row);print(row,flush=True)
(OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
