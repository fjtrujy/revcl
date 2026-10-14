#!/usr/bin/env python3
"""Probe original 221-byte Value::ToString using real GCC alignment/source order.

Preserve genuine +,-,* original predecessor definitions and real sprintf,
MLGameString constructor and authenticated original literal data ranges.
Only rank candidates of original length with all relocation/literal checks.
"""
import importlib.util,itertools,json,subprocess,collections
from pathlib import Path
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/"vuc/config/value_to_string_oct13_probe.json"
base=(R/"vuc/probes/ValueToStringUnit.cpp").read_text()
T=Path("/tmp/vuc-binutils215/value-tostring-original-literal-alignment-next");T.mkdir(parents=True,exist_ok=True)
flags={}
for jump,label,loop in itertools.product((1,2,4,8,16),(1,2,4,8,16),(1,4)):
 flags[f"j{jump}-l{label}-loop{loop}"]=["-O2","-frename-registers",f"-falign-jumps={jump}",f"-falign-labels={label}",f"-falign-loops={loop}"]
rows=[];best=None;sizes=collections.Counter()
for tag,opts in flags.items():
 obj=T/(tag+".o")
 cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(R/"vuc/probes/ValueToStringUnit.cpp"),"-o",str(obj)],capture_output=True,text=True)
 if cp.returncode:
  rows.append(dict(tag=tag,error=cp.stderr[-130:]));continue
 try:res=m.run(obj,cfg)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 row=dict(tag=tag,length=res["length"],overlap=res["exact_bytes"],relocations=res["relocations"],verified_literal_bytes=res["verified_literal_bytes"],prefix=next((i for i in range(221) if i in res["different_offsets"]),221))
 rows.append(row);sizes[res["length"]]+=1
 if res["length"]==221 and res["relocations"]==res.get("relocations",0) and (best is None or (row["overlap"],row["prefix"])>(best["overlap"],best["prefix"])):
  best=row;(T/"best.o").write_bytes(obj.read_bytes());(T/"best.flags").write_text(" ".join(opts)+"\n")
(T/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best_original_length=best,top=sorted((r for r in rows if "overlap" in r),key=lambda r:(r["overlap"],r["prefix"]),reverse=True)[:8]),indent=2))
