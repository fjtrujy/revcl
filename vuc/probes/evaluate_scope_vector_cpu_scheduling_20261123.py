#!/usr/bin/env python3
"""Historical CPU scheduling provenance for the 733/746 SymbolScope vector helper.

GCC 3.3 sched2 currently schedules _M_finish before _M_start in the
growth commit whereas the original VUC 2004.1 reverses them. Use genuine
SymbolTable::BeginScope and the original STL template body. Vary only
historically plausible i386 processor models to test real GCC scheduler
dependencies; preserve the independently exact BeginScope sibling.
"""
from pathlib import Path
import json,importlib.util,subprocess
V=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-scope-vector-cpu-20261123");O.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
source=V/"probes/ScopeVectorInsertionOriginalUnitOct15i.cpp"
config=V/"config/scope_vector_insert_partial_oct15i.json"
begin=V/"config/symbol_begin_scope_probe.json"
cpu={
 "baseline":[],
 "march-i386":["-march=i386"],
 "march-i486":["-march=i486"],
 "march-i586":["-march=i586"],
 "march-i686":["-march=i686"],
 "march-pentium":["-march=pentium"],
 "march-pro":["-march=pentiumpro"],
 "march-p2":["-march=pentium2"],
 "march-p3":["-march=pentium3"],
 "march-p4":["-march=pentium4"],
 "march-athlon":["-march=athlon"],
 "mcpu-i386":["-mcpu=i386"],
 "mcpu-i486":["-mcpu=i486"],
 "mcpu-i586":["-mcpu=i586"],
 "mcpu-i686":["-mcpu=i686"],
 "mcpu-pentium":["-mcpu=pentium"],
 "mcpu-pro":["-mcpu=pentiumpro"],
 "mcpu-p2":["-mcpu=pentium2"],
 "mcpu-p3":["-mcpu=pentium3"],
 "mcpu-p4":["-mcpu=pentium4"],
 "mcpu-athlon":["-mcpu=athlon"],
 "mcpu-pro-march386":["-mcpu=pentiumpro","-march=i386"],
 "mcpu-p3-march386":["-mcpu=pentium3","-march=i386"],
}
rows=[]
for tag,cpuopts in cpu.items():
    for snap in ("jan25","dec06"):
      obj=O/(tag+"-"+snap+".o")
      cmd=["/tmp/vuc-binutils215/g++-"+snap+"-as215",
           "-O2","-frename-registers","-falign-jumps=1",*cpuopts,
           "-c",str(source),"-o",str(obj)]
      cp=subprocess.run(cmd,capture_output=True,text=True)
      row={"cpu":tag,"compiler":snap}
      if cp.returncode:row["compile_error"]=cp.stderr[-300:]
      else:
        for target,p in (("vector",config),("begin",begin)):
          try:
            s=m.run(obj,p)
            row[target]={"length":s["length"],"matched":s["exact_bytes"],
                         "exact":s["exact"],"diffs":s["different_offsets"],
                         "relocs":s["relocations"]}
          except ValueError as e:row[target]={"unverified":str(e)}
      rows.append(row);print(json.dumps(row,sort_keys=True),flush=True)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
