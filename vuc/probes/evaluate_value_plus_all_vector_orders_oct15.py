#!/usr/bin/env python3
"""Find genuine vector lane source order for original VUC Value addition.

Each of the four independent float lanes is modified exactly once. Reordering
lane statements is semantically unchanged for distinct result and temporary
Value objects; historical GCC 3.3.3 may generate different branch alignment.
Full original 219-byte size and original TypeConvert relocation required.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/value_addition_partial_oct14_probe.json").read_text())
raw,elf=m.decomp.load();sec=next(s for s in elf["sections"] if s["name"]==".text")
addr=int(cfg["address"],0);original=raw[sec["offset"]+addr-sec["address"]:sec["offset"]+addr-sec["address"]+219]
base=(R/"vuc/src/ValueArithmeticUnit.cpp").read_text()
start=base.index("Value Value::operator+(const Value &other) const");end=base.index("Value Value::operator-(",start)
segment=base[start:end];old="""case 4:
            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];
            break;"""
assert old in segment
flags={
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j2":["-O2","-frename-registers","-falign-jumps=2"],
 "j4":["-O2","-frename-registers","-falign-jumps=4"],
 "j1l2":["-O2","-frename-registers","-falign-jumps=1","-falign-labels=2"],
 "j1l4":["-O2","-frename-registers","-falign-jumps=1","-falign-labels=4"],
 "j1l8":["-O2","-frename-registers","-falign-jumps=1","-falign-labels=8"],
}
T=Path("/tmp/vuc-binutils215/value-plus-vector-permutations-oct15");T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for order,(label,opts) in itertools.product(itertools.permutations(range(4)),flags.items()):
 tag="".join(str(i) for i in order)+"-"+label
 new="case 4:\n"+"\n".join(f"            result.values[{i}] += temporary.values[{i}];" for i in order)+"\n            break;"
 code=base.replace(segment,segment.replace(old,new))
 cpp=T/(tag+".cpp");obj=T/(tag+".o");cpp.write_text(code)
 run=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 if run.returncode:rows.append(dict(tag=tag,error=run.stderr[-140:]));continue
 try:b,r=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],addr)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 match=sum(a==v for a,v in zip(b,original))
 prefix=next((i for i,(a,v) in enumerate(zip(b,original)) if a!=v),min(len(b),len(original)))
 row=dict(tag=tag,length=len(b),relocations=r,match=match,prefix=prefix)
 rows.append(row);sizes[len(b)]+=1
 if len(b)==219 and r==1 and (best is None or (match,prefix)>(best["match"],best["prefix"])):
  best=row;(T/"best.cpp").write_text(code);(T/"best.flags").write_text(" ".join(opts)+"\n");(T/"best.o").write_bytes(obj.read_bytes())
(T/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((x for x in rows if "match" in x),key=lambda x:(x["match"],x["prefix"]),reverse=True)[:16]),indent=2))
