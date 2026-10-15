#!/usr/bin/env python3
"""Restore Type's real C++03 default constructor within its accepted template TU.

The existing exact TypeUninitializedCopy.cpp reconstructs *two* original
GCC 3.3.3 uninitialized-copy specializations. Co-instantiating the new
_M_insert_aux without the original user-provided default constructor
shortens the 525-byte function to 440. Recover plausible Type::Type()
constructor ownership, and check the two original exact specializations
in the *same* ELF32 object, not merely candidate function length.
"""
from pathlib import Path
import subprocess,importlib.util,json
R=Path(__file__).resolve().parents[1]
OUT=Path("/tmp/vuc-binutils215/type-vector-real-ctor-oct15s")
OUT.mkdir(exist_ok=True)
original=(R/"src/TypeUninitializedCopy.cpp").read_text()
marks='    Type(const Type &other)'
assert marks in original
variants={
 "original":original,
 "empty-default":original.replace(marks,'    Type() {}\n'+marks),
 "zero-default":original.replace(marks,'    Type():a(0),b(0),c(0),d(0) {}\n'+marks),
 "copy-from-default":original.replace(marks,'    Type():a(0),b(0),c(0),d(0) {}\n'+marks),
 "private-default":original.replace(marks,'    Type() {}\n'+marks),
 "member-init-a":original.replace(marks,'    Type():a(0) {}\n'+marks),
 "default-members":original.replace(marks,'    Type() { a=b=c=d=0; }\n'+marks),
}
assert len(set(variants.values()))>=4
spec=importlib.util.spec_from_file_location("m",R/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
candidates={
 "insert":"type_vector_insert_aux_partial_oct15s.json",
 "accepted_const":"uninitialized_type_const_probe.json",
 "accepted_mut":"uninitialized_type_mut_probe.json"
}
results=[]
for label,source in variants.items():
 if label=="private-default":source=source.replace('    Type() {}','    Type() { a=0; }')
 cpp=OUT/(label+".cpp")
 cpp.write_text(source+'\n// Genuine third C++03 template specialization\ntemplate void TypeVector::_M_insert_aux(TypeIterator,const Type&);\n')
 for compiler in ("jan25","dec06"):
  obj=OUT/(label+"-"+compiler+".o")
  cp=subprocess.run(["/tmp/vuc-binutils215/g++-"+compiler+"-as215","-O2","-frename-registers","-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  row={"case":label,"compiler":compiler}
  if cp.returncode:row["error"]=cp.stderr[-200:]
  else:
   for name,manifest in candidates.items():
    try:
     scored=m.run(obj,R/"config"/manifest)
     row[name]={"length":scored["length"],"score":scored["exact_bytes"],"relocations":scored["relocations"]}
    except ValueError as e:row[name]={"error":str(e)}
  results.append(row);print(row,flush=True)
(OUT/"results.json").write_text(json.dumps(results,indent=2)+"\n")
