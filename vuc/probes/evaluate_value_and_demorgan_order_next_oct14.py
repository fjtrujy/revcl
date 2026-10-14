#!/usr/bin/env python3
"""Authentic logical AND eight-term C++03 De Morgan evaluation order.

Reconstruction with no synthetic opcodes: float comparison expressions are
side-effect free and commutative, but lexical order changes historical
GCC 3.3.3 branch placement. Check exact original 325-byte length/relocation.
"""
from pathlib import Path
from itertools import permutations,product
import importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/value_logical_and_partial_oct14_probe.json").read_text())
raw,elf=m.decomp.load();s=next(x for x in elf["sections"] if x["name"]==".text")
address=int(cfg["address"],0);original=raw[s["offset"]+address-s["address"]:s["offset"]+address-s["address"]+325]
base=(R/cfg["source"]).read_text()
old=base.split("case 4: result.bits=",1)[1].split(";break;",1)[0]
T=Path("/tmp/vuc-binutils215/value-and-demorgan-order-next-oct14");T.mkdir(exist_ok=True,parents=True)
flags={
 "j16":["-O2","-frename-registers","-ffast-math","-falign-jumps=16","-falign-labels=1","-falign-loops=1"],
 "j1":["-O2","-frename-registers","-ffast-math","-falign-jumps=1","-falign-labels=8","-falign-loops=1"],
 "l16":["-O2","-frename-registers","-ffast-math","-falign-jumps=8","-falign-labels=16","-falign-loops=1"],
}
rows=[];sizes=collections.Counter();best=None
for laneorder,grouping,fl in product(permutations(range(4)),("by-owner","interleave","by-owner-reverse"),flags):
 name="".join(str(i) for i in laneorder)+"-"+grouping+"-"+fl
 r=[f"(result.values[{i}] == 0.0f)" for i in laneorder]
 t=[f"(temporary.values[{i}] == 0.0f)" for i in laneorder]
 if grouping=="by-owner":terms=r+t
 elif grouping=="by-owner-reverse":terms=t+r
 else:terms=[x for pair in zip(r,t) for x in pair]
 expr="!("+" || ".join(terms)+")"
 code=base.replace("case 4: result.bits="+old,"case 4: result.bits="+expr)
 cpp=T/(name+".cpp");obj=T/(name+".o");cpp.write_text(code)
 cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flags[fl],"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 if cp.returncode:rows.append(dict(name=name,error=cp.stderr[-130:]));continue
 try:body,rel=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],address)
 except ValueError as e:rows.append(dict(name=name,error=str(e)));continue
 score=sum(a==b for a,b in zip(body,original))
 prefix=next((i for i,(a,b) in enumerate(zip(body,original)) if a!=b),min(len(body),len(original)))
 row=dict(name=name,length=len(body),relocations=rel,overlap=score,prefix=prefix)
 rows.append(row);sizes[len(body)]+=1
 if len(body)==325 and rel==1 and (best is None or (score,prefix)>(best["overlap"],best["prefix"])):
  best=row;(T/"best.cpp").write_text(code);(T/"best.o").write_bytes(obj.read_bytes());(T/"best.flags").write_text(" ".join(flags[fl])+"\n")
(T/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if "overlap" in r),key=lambda r:(r["overlap"],r["prefix"]),reverse=True)[:10]),indent=2))
