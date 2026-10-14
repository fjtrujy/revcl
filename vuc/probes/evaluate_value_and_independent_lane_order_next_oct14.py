#!/usr/bin/env python3
"""VUC Value::operator&& - genuine independent lane evaluation order.

All eight float comparisons have no C++ side effects. Earlier De Morgan
C++03 reconstruction reached a verified original-length 192/325 partial
when source order was 1,0,2,3. Test distinct left-hand and right-hand
value lane orders to recover more original GCC branch topology.
"""
from pathlib import Path
from itertools import permutations,product
import importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/value_logical_and_partial_oct14_probe.json").read_text())
raw,elf=m.decomp.load();sec=next(s for s in elf["sections"] if s["name"]==".text")
address=int(cfg["address"],0)
original=raw[sec["offset"]+address-sec["address"]:sec["offset"]+address-sec["address"]+325]
base=(R/cfg["source"]).read_text()
old=base.split("case 4: result.bits=",1)[1].split(";break;",1)[0]
flags=["-O2","-frename-registers","-ffast-math","-falign-jumps=16","-falign-labels=1","-falign-loops=1"]
T=Path("/tmp/vuc-binutils215/value-and-independent-lane-owners-next-oct14");T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for left,right in product(permutations(range(4)),permutations(range(4))):
 tag="".join(map(str,left))+"-"+"".join(map(str,right))
 first=[f"(result.values[{i}] == 0.0f)" for i in left]
 second=[f"(temporary.values[{i}] == 0.0f)" for i in right]
 expr="!("+" || ".join(first+second)+")"
 code=base.replace("case 4: result.bits="+old,"case 4: result.bits="+expr)
 cpp=T/(tag+".cpp");obj=T/(tag+".o");cpp.write_text(code)
 cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flags,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 if cp.returncode:rows.append(dict(tag=tag,error=cp.stderr[-150:]));continue
 try:body,rel=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],address)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 match=sum(a==b for a,b in zip(body,original))
 prefix=next((i for i,(a,b) in enumerate(zip(body,original)) if a!=b),min(len(body),len(original)))
 row=dict(tag=tag,length=len(body),relocations=rel,overlap=match,prefix=prefix)
 rows.append(row);sizes[len(body)]+=1
 if len(body)==325 and rel==1 and (best is None or (match,prefix)>(best["overlap"],best["prefix"])):
  best=row
  (T/"best.cpp").write_text(code);(T/"best.o").write_bytes(obj.read_bytes());(T/"best.flags").write_text(" ".join(flags)+"\n")
(T/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if "overlap" in r),key=lambda r:(r["overlap"],r["prefix"]),reverse=True)[:16]),indent=2))
