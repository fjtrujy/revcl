#!/usr/bin/env python3
"""Compare original ELF3 VUC i386 codegen under real GCC 3.3 CPU targets.

This is a historic compiler provenance discriminator, not speculative source
byte padding. Check original-length bytes, relocations, literals for three
near-partial/unresolved functions with the same machine code requirements.
A changed or wrong-length result is never considered a valid improvement.
"""
from pathlib import Path
import json,importlib.util,subprocess
V=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-arch-provenance-20261123");O.mkdir(exist_ok=True)
unit=(V/"src/SymbolTableUnit.cpp")
try_file=V/"probes/TryOpenFileSuccessFirstOct15t.cpp"
value_file=V/"probes/ValueAdditionLaneOrderOct15.cpp"
spec=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
targets={
 "try-open":(try_file,
     ["-O2","-frename-registers","-falign-jumps=8"],
     "try_open_file_partial_oct15t.json",650),
 "value-plus":(value_file,
     ["-O2","-frename-registers","-falign-jumps=1"],
     "value_addition_partial_oct14_probe.json",203),
 "symbol-enter":(unit,
     ["-O2","-frename-registers","-falign-jumps=8","-falign-labels=1","-falign-loops=1"],
     "symboltable_enter_oct13_probe.json",0),
}
cpu={
 "default":[],
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
 "mcpu-pro":["-mcpu=pentiumpro"],
 "mcpu-p3":["-mcpu=pentium3"],
 "mcpu-p4":["-mcpu=pentium4"],
 "mcpu-athlon":["-mcpu=athlon"],
 "mcpu-pro-march386":["-mcpu=pentiumpro","-march=i386"],
 "mcpu-p3-march386":["-mcpu=pentium3","-march=i386"],
 "cpu-base-maccumulate":["-maccumulate-outgoing-args"],
 "cpu-base-noaccumulate":["-mno-accumulate-outgoing-args"],
}
rows=[]
for code,(source,flags,config,baseline) in targets.items():
  for snap in ("jan25","dec06"):
    for tag,arch in cpu.items():
      obj=O/(code+"-"+snap+"-"+tag+".o")
      cp=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215",*flags,*arch,
                         "-c",str(source),"-o",str(obj)],capture_output=True,text=True)
      row={"target":code,"cpu":tag,"snapshot":snap}
      if cp.returncode:row["compiler_error"]=cp.stderr[-250:]
      else:
        try:
          s=m.run(obj,V/"config"/config)
          row.update(matched=s["exact_bytes"],length=s["length"],
                     exact=s["exact"],relocs=s["relocations"],
                     diffs=s["different_offsets"][:28])
          if s["exact_bytes"]>baseline:row["improved"]=s["exact_bytes"]-baseline
        except ValueError as ex:row["unverified"]=str(ex)
      rows.append(row);print(json.dumps(row,sort_keys=True),flush=True)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
