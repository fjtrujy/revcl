#!/usr/bin/env python3
"""Test authentic consecutive MLTempString constructor and Set source unit.

Original symbols from 0x08071b40 to 0x08071d00 are the formatted C2/C1,
default C2/C1, then variadic Set. Check genuine predecessor coexistence
and polymorphic return signatures; preserve singleton Get and data source.
"""
from pathlib import Path
import itertools, importlib.util, subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/mltemp-consecutive-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("matcher",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/"vuc/src/MLTempStringFormatted.cpp").read_text()
base=base.replace('#include "MLGameString.h"', '')
base=base.replace('#include "MLTempStringData.h"', '#include "'+str(R/"vuc/src/MLTempStringData.h")+'"')
def_start=base.index("MLTempString::MLTempString(")
header=base[:def_start]
formatted=base[def_start:]
default=(R/"vuc/src/MLTempStringDefault.cpp").read_text()
default=default[default.index("MLTempString::MLTempString()"):]
forms={
 "void":("void",""),
 "selfptr":("MLTempString *","return this;"),
 "selfref":("MLTempString &","return *this;"),
 "text":("const char *","return text;")
}
set_bodies={
 "standard":"""va_list arguments; va_start(arguments, format);
 text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
 va_end(arguments);""",
 "resultlocal":"""va_list arguments; va_start(arguments, format);
 char *result = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
 text = result; va_end(arguments);""",
 "selflocal":"""va_list arguments; va_start(arguments, format);
 MLTempString *self=this;
 self->text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
 va_end(arguments);""",
}
flags={
 "r":["-O2","-frename-registers"],
 "plain":["-O2"],
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "sections":["-O2","-frename-registers","-ffunction-sections"],
 "cxa":["-O2","-frename-registers","-fuse-cxa-atexit"],
}
results=[]
for kind, (ret, retbody) in forms.items():
 for form, body in set_bodies.items():
  for arrangement in ("formatted-default-set","default-formatted-set","formatted-set-default"):
   declaration=header+f'\nstruct MLTempString {{const char *text; MLTempString(); MLTempString(const char *, ...); {ret} Set(const char *, ...);}};\n'
   definitions={
    "formatted":formatted,
    "default":default,
    "set":f"{ret} MLTempString::Set(const char *format, ...) {{\n {body}\n {retbody}\n}}\n"
   }
   code=declaration+"\n".join(definitions[x] for x in arrangement.split("-"))
   p=T/(kind+"-"+form+"-"+arrangement+".cpp");p.write_text(code)
   for opts,fl in flags.items():
    obj=T/(p.stem+"-"+opts+".o")
    comp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*fl,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
    if comp.returncode:
      print("COMPILE_FAIL",p.stem,opts,comp.stderr[-120:],flush=True);continue
    row={}
    for name,cfg in {
     "formatc1":"ml_temp_format_c1_probe.json",
     "defaultc1":"ml_temp_default_C1_probe.json",
     "defaultc2":"ml_temp_default_c2_probe.json",
     "set":"ml_temp_set_probe.json",
    }.items():
     try:
      res=m.run(obj,R/"vuc/config"/cfg)
      row[name]=res["exact_bytes"]
      if name=="set" and res["exact"]:
       print("SET_EXACT",p,opts,row,flush=True)
     except (ValueError,OSError) as error: row[name]=str(error)[:40]
    results.append((row,p.stem,opts))
   print("done",p.stem,flush=True)
print("BEST")
for row,source,flag in sorted(results,key=lambda r:r[0].get("set",0) if isinstance(r[0].get("set"),int) else 0,reverse=True)[:25]:
 print(source,flag,row)
