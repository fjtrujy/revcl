#!/usr/bin/env python3
"""Original Type vector 525-byte body: try authentic C++03 copy/assign spellings.

The old STL algorithm copies 16-byte Type fields during growth and
in-place backward shift. The residual 44 bytes cluster in scalar copy
scheduling and branch layout. Distinguish copy ctor from copy assignment
using genuinely plausible historical user-written declarations.
All candidates compile in the original 3-instantiation order, and must
preserve 79/79 and 86/86 exact predecessors on both compiler snapshots.
"""
from pathlib import Path
import subprocess,importlib.util,json
R=Path(__file__).resolve().parents[1]
OUT=Path("/tmp/vuc-binutils215/type-vector-copyassign-oct15s")
OUT.mkdir(exist_ok=True)
base=(R/"probes/TypeVectorInsertAuxRealOrderOct15s.cpp").read_text()
constructor='    Type(const Type &other):a(other.a),b(other.b),c(other.c),d(other.d) {}'
assert constructor in base
forms={
 "selected":base,
 "body-forward":base.replace(constructor,'    Type(const Type &other) {a=other.a;b=other.b;c=other.c;d=other.d;}'),
 "body-reverse":base.replace(constructor,'    Type(const Type &other) {d=other.d;c=other.c;b=other.b;a=other.a;}'),
 "assignment-forward":base.replace(constructor,constructor+'\n    Type &operator=(const Type &other) {a=other.a;b=other.b;c=other.c;d=other.d;return *this;}'),
 "assignment-reverse":base.replace(constructor,constructor+'\n    Type &operator=(const Type &other) {d=other.d;c=other.c;b=other.b;a=other.a;return *this;}'),
 "assignment-pairs":base.replace(constructor,constructor+'\n    Type &operator=(const Type &other) {a=other.a;c=other.c;b=other.b;d=other.d;return *this;}'),
 "assignment-second-first":base.replace(constructor,constructor+'\n    Type &operator=(const Type &other) {b=other.b;a=other.a;d=other.d;c=other.c;return *this;}'),
 "copy-body-assignment-forward":base.replace(constructor,'    Type(const Type &other) {a=other.a;b=other.b;c=other.c;d=other.d;}\n    Type &operator=(const Type &other) {a=other.a;b=other.b;c=other.c;d=other.d;return *this;}'),
 "copy-body-assignment-reverse":base.replace(constructor,'    Type(const Type &other) {a=other.a;b=other.b;c=other.c;d=other.d;}\n    Type &operator=(const Type &other) {d=other.d;c=other.c;b=other.b;a=other.a;return *this;}'),
 "copy-assign-memcpy":base.replace(constructor,constructor+'\n    Type &operator=(const Type &other) { __builtin_memcpy(this,&other,16);return *this;}'),
}
msp=importlib.util.spec_from_file_location("match",R/"scripts/match_probe.py")
m=importlib.util.module_from_spec(msp);msp.loader.exec_module(m)
configs={
 "insert":"type_vector_insert_aux_partial_oct15s.json",
 "copy_const":"uninitialized_type_const_probe.json",
 "copy_mut":"uninitialized_type_mut_probe.json",
}
results=[]
for label,source in forms.items():
 cpp=OUT/(label+".cpp");cpp.write_text(source)
 for snap in ("jan25","dec06"):
  obj=OUT/(label+"-"+snap+".o")
  p=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215","-O2","-frename-registers","-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  item={"case":label,"compiler":snap}
  if p.returncode:item["error"]=p.stderr[-160:]
  else:
   for tag,cfg in configs.items():
    try:
     v=m.run(obj,R/"config"/cfg)
     item[tag]=(v["length"],v["exact_bytes"],v["relocations"])
    except ValueError as ex:item[tag]=str(ex)
  results.append(item);print(item,flush=True)
(OUT/"results.json").write_text(json.dumps(results,indent=2)+"\n")
