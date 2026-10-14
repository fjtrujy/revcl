#!/usr/bin/env python3
"""Compare historical GCC 3.3 snapshots for VUC's 120-byte STL scope copy.

This is an actual libstdc++ __uninitialized_copy_aux instantiation, not a
hand-written replacement. Existing Jan25 alignment sweeps found 115/127-byte
outputs; compare the Dec06 snapshot and scheduling/EH source context.
"""
from pathlib import Path
from itertools import product
import subprocess
import importlib.util
ROOT=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/scope-copy-alternate-oct14")
T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("matcher",ROOT/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
source=ROOT/"vuc/probes/scope_uninitialized_copy.cpp"
config=ROOT/"vuc/config/scope_uninitialized_copy_probe.json"
rows=[]
for compiler,rename,jump,loop,exception in product(
  ("dec06","jan25"),(True,False),(1,2,4,8),(1,4,16),(False,True)):
    flags=["-O2","-falign-jumps=%d"%jump,"-falign-loops=%d"%loop]
    if rename:flags+=["-frename-registers"]
    if exception:flags+=["-fuse-cxa-atexit"]
    key="%s-%s-j%d-l%d-%s"%(compiler,"r" if rename else "p",jump,loop,"cxa" if exception else "std")
    obj=T/(key+".o")
    cc=subprocess.run(["/tmp/vuc-binutils215/g++-"+compiler+"-as215",*flags,
                       "-c",str(source),"-o",str(obj)],text=True,capture_output=True)
    if cc.returncode:
        print("ERROR",key,cc.stderr[-100:],flush=True);continue
    symbol=next((ln for ln in subprocess.check_output(["nm","-S",str(obj)],text=True).splitlines()
        if ln.endswith("_ZSt24__uninitialized_copy_auxIP11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS1_ELi513EES7_ET0_T_S9_S8_12__false_type")),"")
    size=int(symbol.split()[1],16) if symbol else -1
    if size==120:
        try:
            result=m.run(obj,config)
            rows.append((result["exact_bytes"],size,key,result["different_offsets"][:15]))
            if result["exact"]:
                print("NEW_EXACT",key,obj,flush=True);break
        except ValueError as e:
            rows.append((0,size,key,str(e)[:100]))
    else:rows.append((0,size,key,"length"))
    if jump==8 and loop==16 and exception:
        print("CHECKED",compiler,"rename" if rename else "plain",flush=True)
for row in sorted(rows,reverse=True)[:26]:print("SCORE",row,flush=True)
