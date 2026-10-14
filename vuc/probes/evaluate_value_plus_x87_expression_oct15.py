#!/usr/bin/env python3
"""VUC original 219-byte Value+ x87 source-level operator idioms.

Default genuine +,-,* body is 219 bytes at j4 with 172 original-offset
matches; j1 yields wrong-sized 216 bytes but overlaps 205 original bytes.
Probe real scalar/lane expression and source ordering to recover original
219-byte mixed alignment without fabricated pads or x86 instructions.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,collections,json
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/value_addition_partial_oct14_probe.json").read_text())
raw,elf=m.decomp.load();sec=next(x for x in elf["sections"] if x["name"]==".text")
addr=int(cfg["address"],0);orig=raw[sec["offset"]+addr-sec["address"]:sec["offset"]+addr-sec["address"]+219]
source=(R/"vuc/src/ValueArithmeticUnit.cpp").read_text()
start=source.index("Value Value::operator+(const Value &other) const");end=source.index("Value Value::operator-(",start)
segment=source[start:end]
intold="case 1: result.bits += temporary.bits; break;"
scalarold="case 3: result.values[0] += temporary.values[0]; break;"
vectorold="""case 4:
            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];
            break;"""
assert all(s in segment for s in (intold,scalarold,vectorold))
ints={
 "compound":"case 1: result.bits += temporary.bits; break;",
 "assign":"case 1: result.bits = result.bits + temporary.bits; break;",
 "reverse":"case 1: result.bits = temporary.bits + result.bits; break;",
 "locala":"case 1: { unsigned a = result.bits; result.bits = a + temporary.bits; break; }",
 "localb":"case 1: { unsigned b = temporary.bits; result.bits += b; break; }",
}
scalars={
 "compound":"case 3: result.values[0] += temporary.values[0]; break;",
 "assign":"case 3: result.values[0] = result.values[0] + temporary.values[0]; break;",
 "reverse":"case 3: result.values[0] = temporary.values[0] + result.values[0]; break;",
 "floatlocals":"case 3: { float lhs = result.values[0]; float rhs = temporary.values[0]; result.values[0] = lhs + rhs; break; }",
 "doublelocals":"case 3: { double lhs = result.values[0]; double rhs = temporary.values[0]; result.values[0] = (float)(lhs + rhs); break; }",
}
vectors={
 "baseline":vectorold,
 "reverse-lane":'case 4:\n'+'\n'.join(f'            result.values[{i}] += temporary.values[{i}];' for i in (3,2,1,0))+'\n            break;',
 "alternate-lane":'case 4:\n'+'\n'.join(f'            result.values[{i}] += temporary.values[{i}];' for i in (0,2,1,3))+'\n            break;',
 "left-to-right":'case 4:\n'+'\n'.join(f'            result.values[{i}] = result.values[{i}] + temporary.values[{i}];' for i in range(4))+'\n            break;',
 "right-to-left":'case 4:\n'+'\n'.join(f'            result.values[{i}] = temporary.values[{i}] + result.values[{i}];' for i in range(4))+'\n            break;',
 "floatpointer":'case 4: { float *values = result.values; const float *otherValues = temporary.values;\n'+'\n'.join(f'            values[{i}] += otherValues[{i}];' for i in range(4))+'\n            break; }',
 "floatloop":'case 4: for(int i=0; i<4; ++i) result.values[i] += temporary.values[i]; break;',
}
flags={
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j2":["-O2","-frename-registers","-falign-jumps=2"],
 "j4":["-O2","-frename-registers","-falign-jumps=4"],
 "j4l1":["-O2","-frename-registers","-falign-jumps=4","-falign-labels=1"],
 "j1l4":["-O2","-frename-registers","-falign-jumps=1","-falign-labels=4"],
}
T=Path("/tmp/vuc-binutils215/value-plus-scalar-vector-idioms-oct15");T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for (ik,iv),(sk,sv),(vk,vv),(fk,fs) in itertools.product(ints.items(),scalars.items(),vectors.items(),flags.items()):
 tag=f'{ik}-{sk}-{vk}-{fk}'
 cpp_text=source.replace(segment,segment.replace(intold,iv).replace(scalarold,sv).replace(vectorold,vv))
 cpp=T/(tag+".cpp");obj=T/(tag+".o");cpp.write_text(cpp_text)
 cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*fs,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 if cp.returncode:rows.append(dict(tag=tag,error=cp.stderr[-140:]));continue
 try:code,rel=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],addr)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 score=sum(x==y for x,y in zip(code,orig))
 prefix=next((i for i,(x,y) in enumerate(zip(code,orig)) if x!=y),min(len(code),len(orig)))
 row=dict(tag=tag,length=len(code),relocations=rel,match=score,prefix=prefix)
 rows.append(row);sizes[len(code)]+=1
 if len(code)==219 and rel==1 and (best is None or (score,prefix)>(best["match"],best["prefix"])):
  best=row;(T/"best.cpp").write_text(cpp_text);(T/"best.o").write_bytes(obj.read_bytes());(T/"best.flags").write_text(" ".join(fs)+"\n")
(T/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if "match" in r),key=lambda r:(r["match"],r["prefix"]),reverse=True)[:14]),indent=2))
