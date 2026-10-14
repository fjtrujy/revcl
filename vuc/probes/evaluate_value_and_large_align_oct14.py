#!/usr/bin/env python3
"""Bounded GCC 3.3.3 original AND alignment beyond old 1/2/4/8 grid."""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
manifest=json.loads((R/"vuc/config/value_logical_and_partial_oct14_probe.json").read_text())
raw,elf=m.decomp.load()
sec=next(s for s in elf["sections"] if s["name"]==".text")
addr=int(manifest["address"],0)
ref=raw[sec["offset"]+addr-sec["address"]:sec["offset"]+addr-sec["address"]+325]
source=R/manifest["source"]
T=Path("/tmp/vuc-binutils215/value-and-larger-align-batch");T.mkdir(parents=True,exist_ok=True)
recs=[];best=None;sizes=collections.Counter()
for jump,label,loop,reorder in itertools.product((1,4,8,16,32),(1,8,16,32),(1,4),(True,False)):
 tag=f"j{jump}-l{label}-loop{loop}-r{int(reorder)}"
 opts=["-O2","-frename-registers","-ffast-math",f"-falign-jumps={jump}",f"-falign-labels={label}",f"-falign-loops={loop}"]
 if not reorder:opts.append("-fno-reorder-blocks")
 obj=T/(tag+".o")
 cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(source),"-o",str(obj)],capture_output=True,text=True)
 if cp.returncode:
  recs.append(dict(tag=tag,error=cp.stderr[-160:]));continue
 try:body,nr=m.prepare_named_function(obj.read_bytes(),manifest["function"],manifest["relocations"],addr)
 except ValueError as e:
  recs.append(dict(tag=tag,error=str(e)));continue
 overlap=sum(a==b for a,b in zip(body,ref))
 prefix=next((i for i,(a,b) in enumerate(zip(body,ref)) if a!=b),min(len(body),len(ref)))
 row=dict(tag=tag,length=len(body),relocations=nr,overlap=overlap,prefix=prefix)
 recs.append(row);sizes[len(body)]+=1
 if len(body)==325 and nr==1 and (best is None or (overlap,prefix)>(best["overlap"],best["prefix"])):
  best=row;(T/"best.o").write_bytes(obj.read_bytes());(T/"best.flags").write_text(" ".join(opts)+"\n")
(T/"results.json").write_text(json.dumps(recs,indent=2)+"\n")
print(json.dumps(dict(attempted=len(recs),sizes=dict(sizes),best=best,top=sorted((x for x in recs if "overlap" in x),key=lambda r:(r["overlap"],r["prefix"]),reverse=True)[:12]),indent=2))
