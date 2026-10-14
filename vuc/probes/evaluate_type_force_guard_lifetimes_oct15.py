#!/usr/bin/env python3
"""Genuine VUC Value::TypeConvertForce C++03 source owner/copy/early-exit recipes.

The original 257-byte function has no external relocation. Historical GCC
3.3.3 emits an authentic 193-byte original-address overlap for baseline.
Test ordinary boolean source and copies with the real TypeConvert predecessor,
never hand-crafted x86 bytes, fake symbols or alignment-only source fillers.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/value_type_convert_force_oct13_probe.json").read_text())
raw,elf=m.decomp.load();sec=next(s for s in elf["sections"] if s["name"]==".text")
addr=int(cfg["address"],0);ref=raw[sec["offset"]+addr-sec["address"]:sec["offset"]+addr-sec["address"]+257]
source=(R/cfg["source"]).read_text()
start=source.index("bool Value::TypeConvertForce(")
body=source[start:]
begin=body.index("    left=first;")
predicate=body.index("    switch(left.type)")
switches=body[predicate:]
assert switches.endswith("}\n")
prefix=body[:begin]
copies={
 "base":"    left=first;\n    right=second;\n",
 "refs":"    Value &outLeft = left;\n    Value &outRight = right;\n    outLeft=first;\n    outRight=second;\n",
 "constrefs":"    const Value &a = first;\n    const Value &b = second;\n    left=a;\n    right=b;\n",
 "alias-pointers":"    Value *outLeft = &left;\n    Value *outRight = &right;\n    *outLeft=first;\n    *outRight=second;\n",
 "copy-members":"    left=first;\n    right=second;\n    const int firstKind = first.type;\n    const int secondKind = second.type;\n",
 "copy-trivial":"    left=first;\n    right=second;\n    const TypeID type=(TypeID)desired;\n",
 "assign-right-first":"    right=second;\n    left=first;\n",
}
guards={
 "base":"    if(first.type == desired && second.type == desired) return true;\n    if(desired != ValueInteger) return false;\n",
 "nested":"    if (first.type == desired) { if (second.type == desired) return true; }\n    if (desired != ValueInteger) return false;\n",
 "neg-or":"    if (!(first.type != desired || second.type != desired)) return true;\n    if (desired != ValueInteger) return false;\n",
 "neg-and":"    if (!(first.type == desired && second.type == desired)) { if (desired != ValueInteger) return false; } else return true;\n",
 "desired-first":"    if (desired == ValueInteger) { if (first.type == desired && second.type == desired) return true; } else if (first.type == desired && second.type == desired) return true; else return false;\n",
 "return-int":"    int both = first.type == desired && second.type == desired;\n    if (both) return true;\n    if (desired != ValueInteger) return false;\n",
 "local-neg":"    const bool mismatch = (first.type != desired || second.type != desired);\n    if (!mismatch) return true;\n    if (desired != ValueInteger) return false;\n",
}
# Avoid mechanically rewriting lexical switches; test useful genuine unsigned/typed representations.
types={
 "baseline":switches,
 "signed-kind":switches.replace("switch(left.type)","switch((int)left.type)").replace("switch(right.type)","switch((int)right.type)"),
 "unsigned-kind":switches.replace("switch(left.type)","switch((unsigned)left.type)").replace("switch(right.type)","switch((unsigned)right.type)"),
 "int-conversion":switches.replace("(int)left.values[0]","static_cast<int>(left.values[0])").replace("(int)right.values[0]","static_cast<int>(right.values[0])"),
 "bool-local":switches.replace("    switch(right.type)","    bool leftConverted = true;\n    if(!leftConverted) return false;\n    switch(right.type)"),
}
flags={
 "saved":["-O2","-falign-jumps=1","-falign-labels=1","-falign-loops=2"],
 "rename":["-O2","-frename-registers","-falign-jumps=1","-falign-labels=1","-falign-loops=2"],
 "j1l1":["-O2","-falign-jumps=1","-falign-labels=1","-falign-loops=1"],
 "j1l4":["-O2","-falign-jumps=1","-falign-labels=1","-falign-loops=4"],
 "j4l1":["-O2","-falign-jumps=4","-falign-labels=1","-falign-loops=2"],
}
T=Path("/tmp/vuc-binutils215/force-source-guards-next-oct15");T.mkdir(parents=True,exist_ok=True)
rows=[];lengths=collections.Counter();best=None
for (ck,cp),(gk,gd),(tk,sw),(fk,opts) in itertools.product(copies.items(),guards.items(),types.items(),flags.items()):
 tag=f"{ck}-{gk}-{tk}-{fk}"
 if ck=="copy-members":gd=gd.replace("first.type","firstKind").replace("second.type","secondKind")
 if ck=="copy-trivial":gd=gd.replace("desired","type")
 code=source[:start]+prefix+cp+gd+sw
 cpp=T/(tag+".cpp");obj=T/(tag+".o");cpp.write_text(code)
 result=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 if result.returncode:rows.append(dict(tag=tag,error=result.stderr[-130:]));continue
 try:machine,rels=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],addr)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 equal=sum(a==b for a,b in zip(ref,machine))
 prefixlen=next((i for i,(a,b) in enumerate(zip(ref,machine)) if a!=b),min(len(ref),len(machine)))
 record=dict(tag=tag,length=len(machine),relocations=rels,match=equal,prefix=prefixlen)
 rows.append(record);lengths[len(machine)]+=1
 if len(machine)==257 and rels==0 and (best is None or (equal,prefixlen)>(best["match"],best["prefix"])):
  best=record;(T/"best.cpp").write_text(code);(T/"best.o").write_bytes(obj.read_bytes());(T/"best.flags").write_text(" ".join(opts)+"\n")
(T/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rows),sizes=dict(lengths),best=best,top=sorted((x for x in rows if "match" in x),key=lambda r:(r["match"],r["prefix"]),reverse=True)[:12]),indent=2))
