#!/usr/bin/env python3
"""Near-exact VUC user-function CPU codegen / i686 scheduling provenance.

Probe authentic original-length 96/98 Print(ASTFunCall*) and 155/161
script_error candidates against 2004-era GCC3.3 i386 CPU targets, where
scheduler/resource assumptions and allocator order may differ. Preserve
relocations, literal-section provenance, accepted functions and original ELF.
"""
from pathlib import Path
import importlib.util,json,subprocess
V=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-near-exact-cpu-20261123");O.mkdir(exist_ok=True)
sp=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
targets={
 "printer":("vuc/probes/PrintASTFunCallRealUnitOct15m.cpp",
   ["-O2"],"print_ast_fun_context_oct14_probe.json",96),
 "script_error":("vuc/probes/ScriptErrorRealUnitOct15k.cpp",
   ["-O2","-frename-registers","-falign-jumps=1"],"script_error_probe.json",155),
}
cpu={
 "baseline":[],
 "i386":["-march=i386"],
 "i486":["-march=i486"],
 "i586":["-march=i586"],
 "i686":["-march=i686"],
 "pentiumpro":["-march=pentiumpro"],
 "pentium2":["-march=pentium2"],
 "pentium3":["-march=pentium3"],
 "pentium4":["-march=pentium4"],
 "athlon":["-march=athlon"],
 "mcpu-i386":["-mcpu=i386"],
 "mcpu-i486":["-mcpu=i486"],
 "mcpu-i686":["-mcpu=i686"],
 "mcpu-pentiumpro":["-mcpu=pentiumpro"],
 "mcpu-pentium3":["-mcpu=pentium3"],
 "mcpu-pentium4":["-mcpu=pentium4"],
 "mcpu-athlon":["-mcpu=athlon"],
 "march-i386-mcpu-pro":["-march=i386","-mcpu=pentiumpro"],
 "march-i386-mcpu-p3":["-march=i386","-mcpu=pentium3"],
 "accumulate":["-maccumulate-outgoing-args"],
 "no-accumulate":["-mno-accumulate-outgoing-args"],
}
rows=[]
for name,(filename,flags,config,baseline) in targets.items():
  for snap in ("jan25","dec06"):
    for arch,opts in cpu.items():
      obj=O/(name+"-"+snap+"-"+arch+".o")
      p=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215",*flags,*opts,
                       "-c",str(V.parent/filename),"-o",str(obj)],capture_output=True,text=True)
      row={"case":name,"compiler":snap,"arch":arch}
      if p.returncode:row["compile_error"]=p.stderr[-250:]
      else:
        try:
          result=m.run(obj,V/"config"/config)
          row.update(length=result["length"],matched=result["exact_bytes"],
                     exact=result["exact"],relocs=result["relocations"],
                     literals=result["verified_literal_bytes"],
                     diffs=result["different_offsets"])
          if result["exact_bytes"]>baseline:row["new_bytes"]=result["exact_bytes"]-baseline
        except ValueError as e:row["unverified"]=str(e)
      rows.append(row);print(json.dumps(row,sort_keys=True),flush=True)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
