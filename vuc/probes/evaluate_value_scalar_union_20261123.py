#!/usr/bin/env python3
"""Test distinct scalar vs vector union views in VUC Value::operator+.

Original 219-byte function has 203 authenticated original-offset bytes,
with remaining 16 in x87 scalar/vector case scheduling. Current source
uses the identical values[0] expression for scalar and lane 0, which may
cause GCC 3.3's cross-jumping to share their computations. Test authentic
C++03 unions/float aliases and scalar expression forms instead of synthetic
register dependencies. Preserve the original TypeConvert relocation.
"""
from pathlib import Path
import importlib.util, json, subprocess
V=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-value-union-scalar-20261123");O.mkdir(exist_ok=True)
original=(V/"probes/ValueAdditionLaneOrderOct15.cpp").read_text()
header=original.split("Value Value::operator+(const Value &other) const\n{")[0]
plus=original[original.index("Value Value::operator+(const Value &other) const\n{"):original.index("Value Value::operator-(const Value &other) const")]
assert plus.count("case 3: result.values[0] += temporary.values[0]; break;")==1
original_union="union { unsigned bits; float values[4]; };"
assert original_union in header
source_variants={
 "baseline":(original_union,"result.values[0] += temporary.values[0]"),
 "float-scalar":("union { unsigned bits; float scalar; float values[4]; };","result.scalar += temporary.scalar"),
 "float-scalar-lhs":("union { unsigned bits; float scalar; float values[4]; };","result.scalar += temporary.values[0]"),
 "float-scalar-rhs":("union { unsigned bits; float scalar; float values[4]; };","result.values[0] += temporary.scalar"),
 "float-scalar-array":("union { unsigned bits; float scalar[1]; float values[4]; };","result.scalar[0] += temporary.scalar[0]"),
 "float-scalar-struct":("union { unsigned bits; struct { float scalar; } value; float values[4]; };","result.value.scalar += temporary.value.scalar"),
 "float-scalar-struct-lhs":("union { unsigned bits; struct { float scalar; } value; float values[4]; };","result.value.scalar += temporary.values[0]"),
 "float-scalar-struct-rhs":("union { unsigned bits; struct { float scalar; } value; float values[4]; };","result.values[0] += temporary.value.scalar"),
 "float-x":("union { unsigned bits; struct {float x,y,z,w;} scalar4; float values[4]; };","result.scalar4.x += temporary.scalar4.x"),
 "float-x-lhs":("union { unsigned bits; struct {float x,y,z,w;} scalar4; float values[4]; };","result.scalar4.x += temporary.values[0]"),
 "float-x-rhs":("union { unsigned bits; struct {float x,y,z,w;} scalar4; float values[4]; };","result.values[0] += temporary.scalar4.x"),
 "float-pointer-scalar":(original_union,"*result.values += *temporary.values"),
 "float-pointer-const":(original_union,"result.values[0] = *temporary.values + *result.values"),
 "float-int-cast":(original_union,"*reinterpret_cast<float*>(&result.bits) += *reinterpret_cast<const float*>(&temporary.bits)"),
 "float-int-cast-simple":(original_union,"*(float*)&result.bits += *(float*)&temporary.bits"),
 "float-obj-pointer":(original_union,"float *dst=&result.values[0]; const float *src=&temporary.values[0]; *dst += *src"),
 "float-obj-ref":(original_union,"float &dst=result.values[0]; const float &src=temporary.values[0]; dst += src"),
 "float-obj-temporary":(original_union,"float scalar = result.values[0]; scalar += temporary.values[0]; result.values[0] = scalar"),
 "float-obj-formula":(original_union,"result.values[0] = result.values[0] + temporary.values[0]"),
 "float-obj-reversed":(original_union,"result.values[0] = temporary.values[0] + result.values[0]"),
}
spec=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
cfg=V/"config/value_addition_partial_oct14_probe.json"
rows=[]
for name,(union, scalar) in source_variants.items():
  code=header.replace(original_union,union)+"\n"+plus.replace(
       "result.values[0] += temporary.values[0]; break;",
       scalar+"; break;",1)
  p=O/(name+".cpp");p.write_text(code)
  for snap in ("jan25","dec06"):
    obj=O/(name+"-"+snap+".o")
    cmd=["/tmp/vuc-binutils215/g++-"+snap+"-as215",
         "-O2","-frename-registers","-falign-jumps=1","-c",str(p),"-o",str(obj)]
    cp=subprocess.run(cmd,capture_output=True,text=True)
    row={"source":name,"compiler":snap}
    if cp.returncode:row["compilation_error"]=cp.stderr[-350:]
    else:
      try:
        s=m.run(obj,cfg)
        row.update(size=s["length"],matched=s["exact_bytes"],
                   relocs=s["relocations"],diffs=s["different_offsets"],exact=s["exact"])
      except ValueError as e:row["unverified"]=str(e)
    print(json.dumps(row,sort_keys=True),flush=True)
    rows.append(row)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
