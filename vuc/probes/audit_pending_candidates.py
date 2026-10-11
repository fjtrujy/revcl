#!/usr/bin/env python3
"""Audit pending original-ELF VUC hypotheses with their recorded GCC recipes.

Never promote partial matches. Do not alter any sources or manifests.
"""
from pathlib import Path
import hashlib,importlib.util,json,subprocess
R=Path(__file__).resolve().parents[2]
TMP=Path("/tmp/vuc-binutils215/pending-audit")
TMP.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
accepted=set(json.loads((R/"vuc/config/exact_candidates.json").read_text()))
compiler="/tmp/vuc-binutils215/g++-jan25-as215"
results=[]
compiled={}
for config in sorted((R/"vuc/config").glob("*_probe.json")):
 rel="vuc/config/"+config.name
 if rel in accepted:continue
 try:
  c=json.loads(config.read_text())
  if not isinstance(c,dict) or not all(k in c for k in ("function","source","address","size")):continue
  source=R/c["source"]
  if not source.is_file():results.append((-1,rel,"MISSING SOURCE"));continue
  flags=tuple(c.get("flags",["-O2","-frename-registers"]))
  key=(source,flags)
  if key not in compiled:
   out=TMP/("p-"+hashlib.sha256((str(source)+"\x00"+" ".join(flags)).encode()).hexdigest()[:12]+".o")
   p=subprocess.run([compiler,*flags,"-c",str(source),"-o",str(out)],capture_output=True,text=True)
   compiled[key]=(out,p.returncode,p.stderr[-180:])
  obj,code,err=compiled[key]
  if code:results.append((-1,rel,"COMPILE "+err));continue
  try:
   item=m.run(obj,config)
   results.append((item["exact_bytes"] if item["exact"] else item["exact_bytes"],rel,"%d/%d%s"%(item["exact_bytes"],item["length"]," EXACT" if item["exact"] else "")))
  except Exception as e:results.append((-1,rel,str(e)[:145]))
 except Exception as e:results.append((-1,rel,"BAD CONFIG "+str(e)[:110]))
results.sort(key=lambda x:x[0],reverse=True)
for row in results:print("%4s %-70s %s"%(row[0],row[1],row[2]),flush=True)
print("SUMMARY",len(results),"fully-exact",sum(" EXACT" in r[2] for r in results),flush=True)
