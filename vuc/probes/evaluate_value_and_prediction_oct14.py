#!/usr/bin/env python3
"""GCC 3.3.3 real TypeConvert and switch prediction controls for Value &&.

Only tested valid source syntax and verified original-length instruction bytes
with the legitimate TypeConvert relocation, never manual alignment padding.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
config=json.loads((R/"vuc/config/value_logical_and_partial_oct14_probe.json").read_text())
raw,elf=m.decomp.load();s=next(x for x in elf["sections"] if x["name"]==".text")
addr=int(config["address"],0);original=raw[s["offset"]+addr-s["address"]:s["offset"]+addr-s["address"]+325]
src=(R/config["source"]).read_text()
assert "if (TypeConvert(*this,other,result,temporary))" in src
assert "switch (result.type)" in src
T=Path("/tmp/vuc-binutils215/value-and-prediction-branch-batch");T.mkdir(parents=True,exist_ok=True)
rec=[];sizes=collections.Counter();best=None
for predicate,switch,jump,label in itertools.product(("plain","likely","unlikely"),("plain","type1","type3","type4"),(1,4,8),(1,8,16)):
 key=f"{predicate}-{switch}-j{jump}-l{label}"
 text=src
 if predicate!="plain":
  text=text.replace("if (TypeConvert(*this,other,result,temporary))","if (__builtin_expect(TypeConvert(*this,other,result,temporary),"+("1" if predicate=="likely" else "0")+"))")
 if switch!="plain":text=text.replace("switch (result.type)","switch (__builtin_expect(result.type,"+switch[-1]+"))")
 cpp=T/(key+".cpp");o=T/(key+".o");cpp.write_text(text)
 flags=["-O2","-frename-registers","-ffast-math",f"-falign-jumps={jump}",f"-falign-labels={label}","-falign-loops=1"]
 cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flags,"-c",str(cpp),"-o",str(o)],capture_output=True,text=True)
 if cp.returncode:rec.append(dict(key=key,error=cp.stderr[-120:]));continue
 try:body,rels=m.prepare_named_function(o.read_bytes(),config["function"],config["relocations"],addr)
 except ValueError as e:rec.append(dict(key=key,error=str(e)));continue
 score=sum(a==b for a,b in zip(body,original))
 prefix=next((i for i,(a,b) in enumerate(zip(body,original)) if a!=b),min(len(body),len(original)))
 r=dict(key=key,length=len(body),relocations=rels,overlap=score,prefix=prefix)
 rec.append(r);sizes[len(body)]+=1
 if len(body)==325 and rels==1 and (best is None or (score,prefix)>(best["overlap"],best["prefix"])):
  best=r;(T/"best.cpp").write_text(text);(T/"best.flags").write_text(" ".join(flags)+"\n");(T/"best.o").write_bytes(o.read_bytes())
(T/"results.json").write_text(json.dumps(rec,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rec),sizes=dict(sizes),best_original_length=best,top=sorted((x for x in rec if "overlap"in x),key=lambda x:(x["overlap"],x["prefix"]),reverse=True)[:10]),indent=2))
