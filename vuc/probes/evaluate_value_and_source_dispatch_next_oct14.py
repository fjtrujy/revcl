#!/usr/bin/env python3
"""Recover Value::operator&& original dispatch order with authentic C++03 branches.

Disassemble the original 325-byte function separately: the boolean result and
the x87 float pair comparisons appear authentic, but case dispatch and
epilogue layout differ. Probe real if/else source control flow and type
lifetime using both period compiler options and relocation-aware comparison.
"""
from pathlib import Path
from itertools import product,permutations
import collections,importlib.util,json,subprocess
R=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/value_logical_and_partial_oct14_probe.json").read_text())
raw,elf=m.decomp.load();section=next(s for s in elf["sections"] if s["name"]==".text")
address=int(cfg["address"],0)
original=raw[section["offset"]+address-section["address"]:section["offset"]+address-section["address"]+cfg["size"]]
base=(R/cfg["source"]).read_text()
start=base.index("  switch (result.type) {")
end=base.index("\n  }\n }\n return result;",start)+len("\n  }")
region=base[start:end]
cases={
 "int":('type == 1 || type == 2','result.bits=(result.bits && temporary.bits);'),
 "float":('type == 3','result.bits=(result.values[0] != 0.0f && temporary.values[0] != 0.0f);'),
 "vector":('type == 4','result.bits='+region.split("case 4: result.bits=",1)[1].split(";break;",1)[0]+";"),
}
modes=("ifelse","if-continue","if-independent","switch-derived")
types=("direct","local","const-local")
flags={
 "saved":["-O2","-frename-registers","-ffast-math","-falign-jumps=16","-falign-labels=1","-falign-loops=1"],
 "old":["-O2","-frename-registers","-ffast-math","-falign-jumps=1","-falign-labels=8","-falign-loops=1"],
 "j8l16":["-O2","-frename-registers","-ffast-math","-falign-jumps=8","-falign-labels=16","-falign-loops=1"],
 "j16l16":["-O2","-frename-registers","-ffast-math","-falign-jumps=16","-falign-labels=16","-falign-loops=1"],
 "nojump":["-O2","-frename-registers","-ffast-math","-falign-jumps=1","-falign-labels=1","-falign-loops=1"],
}
T=Path("/tmp/vuc-binutils215/value-and-if-dispatch-oct14");T.mkdir(parents=True,exist_ok=True)
rank=[];lengths=collections.Counter();best=None
for order,mode,typ,flag in product(tuple(permutations(cases)),modes,types,flags):
 key="-".join(order)+"-"+mode+"-"+typ+"-"+flag
 decl="" if typ=="direct" else "    "+("const int" if typ=="const-local" else "int")+" type = result.type;\n"
 def cond(expr):return expr.replace("type","result.type") if typ=="direct" else expr
 if mode=="ifelse":
  region=decl+"    "+"\n    else ".join(("if ("+cond(cases[k][0])+") { "+cases[k][1]+" }") for k in order)
 elif mode=="if-independent":
  region=decl+"    "+"\n    ".join(("if ("+cond(cases[k][0])+") { "+cases[k][1]+" }") for k in order)
 elif mode=="if-continue":
  region=decl+"    "+"\n    else ".join(("if ("+cond(cases[k][0])+") "+cases[k][1]) for k in order)
 else:
  region=decl+"    switch ("+("result.type" if typ=="direct" else "type")+") {\n"+ "\n".join(("case "+("1: case 2:" if k=="int" else "3:" if k=="float" else "4:")+" "+cases[k][1]+" break;") for k in order)+"\n    }"
 code=base[:start]+region+base[end:]
 cpp=T/(key+".cpp");obj=T/(key+".o");cpp.write_text(code)
 build=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flags[flag],"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 if build.returncode:
  rank.append(dict(key=key,error=build.stderr[-170:]));continue
 try:
  body,rel=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],address)
 except ValueError as e:
  rank.append(dict(key=key,error=str(e)));continue
 score=sum(a==b for a,b in zip(original,body))
 prefix=next((i for i,(a,b) in enumerate(zip(original,body)) if a!=b),min(len(original),len(body)))
 row=dict(key=key,length=len(body),relocations=rel,overlap=score,prefix=prefix)
 rank.append(row);lengths[len(body)]+=1
 if len(body)==325 and rel==1 and (best is None or (score,prefix)>(best["overlap"],best["prefix"])):
  best=row;(T/"best.cpp").write_text(code);(T/"best.flags").write_text(" ".join(flags[flag])+"\n");(T/"best.o").write_bytes(obj.read_bytes())
(T/"results.json").write_text(json.dumps(rank,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rank),original_lengths=lengths.get(325,0),lengths=dict(lengths),best=best,top=sorted((r for r in rank if "overlap" in r),key=lambda r:(r["overlap"],r["prefix"]),reverse=True)[:14]),indent=2))
