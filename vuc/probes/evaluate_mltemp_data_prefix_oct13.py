#!/usr/bin/env python3
"""Test MLTempStringData's genuine format methods before variadic Set.

VUC ELF places Get, FormatString, VFormatString, formatted/default
constructors, Set in one contiguous run. Restore real recovered methods
rather than compiler-only dummy predecessors; check authentic Set
relocations and both constructor aliases.
"""
from pathlib import Path
import importlib.util,subprocess,sys
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/mltemp-full-neighbors-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("matcher",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
unit=R/"vuc/probes/ml_temp_vformat.cpp"
vformat=unit.read_text()
vformat=vformat[vformat.index('char *MLTempStringData::VFormatString('):]
part=(R/"vuc/src/MLGameString.cpp").read_text()
format_method=part[part.index('char *MLTempStringData::FormatString('):part.index('MLTempString &MLTempString::operator+=')]
flags={
 "plain":["-O2"],"rename":["-O2","-frename-registers"],
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "sections":["-O2","-frename-registers","-ffunction-sections"],
}
variants=[
 "selfref-standard-formatted-default-set",
 "selfptr-standard-formatted-default-set",
 "void-standard-formatted-default-set",
 "selfref-standard-default-formatted-set",
 "selfptr-standard-default-formatted-set",
]
generated_root=Path("/tmp/vuc-binutils215/mltemp-consecutive-oct13")
if any(not (generated_root/(name+".cpp")).is_file() for name in variants):
    subprocess.run(
        [sys.executable,str(R/"vuc/probes/evaluate_mltemp_consecutive_unit_oct13.py")],
        check=True, stdout=subprocess.DEVNULL)
for name in variants:
 # Preserve the proven earlier compiler source for this concrete shape,
 # without regenerating a different ABI declaration.
 generated=(generated_root/(name+".cpp")).read_text()
 marker=generated.index('MLTempString::MLTempString(const char *format, ...)')
 head=generated[:marker]
 head=head.replace('#include <cstdarg>', '''#include <cstdarg>
#include <cstdio>
#include <cstring>
extern "C" void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__((noreturn));''')
 tail=generated[marker:]
 for mode in ("format","vformat","both"):
  middle=(format_method if mode in ("format","both") else "")+"\n"+(vformat if mode in ("vformat","both") else "")+"\n"
  p=T/(name+"-"+mode+".cpp");p.write_text(head+middle+tail)
  for fl,opts in flags.items():
   o=T/(p.stem+"-"+fl+".o")
   cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(p),"-o",str(o)],capture_output=True,text=True)
   if cp.returncode:
    print("COMPILER_FAIL",p.stem,fl,cp.stderr[-130:],flush=True);continue
   scores={}
   for target,conf in {
      "set":"ml_temp_set_probe.json",
      "defaultc2":"ml_temp_default_c2_probe.json",
      "defaultc1":"ml_temp_default_C1_probe.json",
      "formatc1":"ml_temp_format_c1_probe.json"
    }.items():
    try:
     r=m.run(o,R/"vuc/config"/conf)
     scores[target]=r["exact_bytes"]
     if r["exact"] and target=="set":print("NEW_EXACT",p,fl,scores,flush=True)
    except (OSError,ValueError) as e:scores[target]=str(e)[:58]
   print("CHECK",p.stem,fl,scores,flush=True)
