#!/usr/bin/env python3
"""Probe real boolean algebra of Value::operator&& for original x87 CFG.

Vector and scalar x87 compare expressions are side-effect-free. Test actual
source-level De Morgan, short-circuit, and equivalent boolean forms rather
than alignment-only or injected instruction workarounds.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/value_logical_and_partial_oct14_probe.json").read_text())
raw,elf=m.decomp.load();sec=next(s for s in elf["sections"] if s["name"]==".text")
addr=int(cfg["address"],0);orig=raw[sec["offset"]+addr-sec["address"]:sec["offset"]+addr-sec["address"]+325]
base=(R/cfg["source"]).read_text()
scalar="(result.values[0] != 0.0f && temporary.values[0] != 0.0f)"
vector=base.split("case 4: result.bits=",1)[1].split(";break;",1)[0]
assert scalar in base and vector
terms=[f"(result.values[{i}] != 0.0f && temporary.values[{i}] != 0.0f)" for i in range(4)]
negterms=[f"(result.values[{i}] == 0.0f || temporary.values[{i}] == 0.0f)" for i in range(4)]
bareterms=[f"(result.values[{i}] && temporary.values[{i}])" for i in range(4)]
negs=[f"(result.values[{i}] == 0.0f)" for i in range(4)]+[f"(temporary.values[{i}] == 0.0f)" for i in range(4)]
vectors={
 "baseline":vector,
 "demorgan-total":"!("+" || ".join(negterms)+")",
 "demorgan-eight":"!("+" || ".join(negs)+")",
 "demorgan-pairs":" && ".join("(!"+part+")" for part in negterms),
 "bool-short":" && ".join(bareterms),
 "double-negated":"!!("+" && ".join(terms)+")",
 "integer-bools":" && ".join(f"(bool(result.values[{i}]) && bool(temporary.values[{i}]))" for i in range(4)),
 "last-true-predicate":" && ".join(terms[:3]+["!"+negterms[3]]),
 "first-true-predicate":" && ".join(["!"+negterms[0]]+terms[1:]),
 "ternary":"("+" && ".join(terms)+") ? 1 : 0",
 "alternate-pair-order":" && ".join([terms[i] for i in (0,2,1,3)]),
 "reverse-pairs":" && ".join(reversed(terms)),
 "bitwise-pairs":" && ".join(f"(bool(result.values[{i}]) & bool(temporary.values[{i}]))" for i in range(4)),
}
scalars={
 "original":scalar,
 "demorgan":"!(result.values[0] == 0.0f || temporary.values[0] == 0.0f)",
 "bool":"(bool(result.values[0]) && bool(temporary.values[0]))",
 "bare":"(result.values[0] && temporary.values[0])",
 "ternary":"("+scalar+" ? 1 : 0)",
}
flags={
 "selected":["-O2","-frename-registers","-ffast-math","-falign-jumps=16","-falign-labels=1","-falign-loops=1"],
 "prior":["-O2","-frename-registers","-ffast-math","-falign-jumps=1","-falign-labels=8","-falign-loops=1"],
 "labels16":["-O2","-frename-registers","-ffast-math","-falign-jumps=8","-falign-labels=16","-falign-loops=1"],
 "labels1":["-O2","-frename-registers","-ffast-math","-falign-jumps=1","-falign-labels=1","-falign-loops=1"],
}
T=Path("/tmp/vuc-binutils215/value-and-demorgan-next-oct14");T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for (vn,v),(sn,s),(fn,fl) in itertools.product(vectors.items(),scalars.items(),flags.items()):
 key=f"{vn}-{sn}-{fn}"
 code=base.replace("case 3: result.bits="+scalar,"case 3: result.bits="+s).replace("case 4: result.bits="+vector,"case 4: result.bits="+v)
 cpp=T/(key+".cpp");obj=T/(key+".o");cpp.write_text(code)
 cc=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*fl,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 if cc.returncode:rows.append(dict(key=key,error=cc.stderr[-150:]));continue
 try:body,rel=m.prepare_named_function(obj.read_bytes(),cfg["function"],cfg["relocations"],addr)
 except ValueError as e:rows.append(dict(key=key,error=str(e)));continue
 overlap=sum(a==b for a,b in zip(orig,body))
 prefix=next((i for i,(a,b) in enumerate(zip(orig,body)) if a!=b),min(len(orig),len(body)))
 row=dict(key=key,length=len(body),relocations=rel,overlap=overlap,prefix=prefix)
 rows.append(row);sizes[len(body)]+=1
 if len(body)==325 and rel==1 and (best is None or (overlap,prefix)>(best["overlap"],best["prefix"])):
  best=row;(T/"best.cpp").write_text(code);(T/"best.flags").write_text(" ".join(fl)+"\n");(T/"best.o").write_bytes(obj.read_bytes())
(T/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if "overlap" in r),key=lambda r:(r["overlap"],r["prefix"]),reverse=True)[:12]),indent=2))
