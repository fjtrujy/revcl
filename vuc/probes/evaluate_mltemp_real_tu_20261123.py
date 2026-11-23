#!/usr/bin/env python3
"""VCL genuine-TU ownership tests for MLTempString constructor / Set.

Original C2 default constructor 95/101; Set 111/117. All share inline
lazy singleton MLTempStringData::Get and variadic formatting operations.
Check real 2004 C++03 source methods combined/split in plausible source
order and return contracts. Never modify accepted source; verify 102/102
C1 and 110-byte formatted C2 when candidates become eligible.
"""
from pathlib import Path
import itertools, importlib.util, json, subprocess
V=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-mltemp-tus-20261123");O.mkdir(exist_ok=True)
source=(V/"probes/ml_temp_constructor_unit_oct12.cpp").read_text()
source=source.replace('#include "../src/MLTempStringData.h"',
                      '#include "'+str(V/"src/MLTempStringData.h")+'"')
prefix=source[:source.index("MLTempString::MLTempString(const char *format")]
formatted=source[source.index("MLTempString::MLTempString(const char *format"):source.index("MLTempString::MLTempString()")]
default=source[source.index("MLTempString::MLTempString()"):]
setter=(V/"probes/ml_temp_set_return.cpp").read_text()
setter=setter[setter.index("MLTempString *MLTempString::Set("):]
cfg={
 "default-c2":V/"config/ml_temp_default_C2_probe.json",
 "set":V/"config/ml_temp_set_probe.json",
 "formatted-c2":V/"config/ml_temp_format_c2_probe.json",
 "default-c1":V/"config/ml_temp_default_C1_probe.json",
}
assert prefix.count("    MLTempString();")==1
assert prefix.count("    MLTempString(const char *format, ...);")==1
prefix=prefix.replace("    MLTempString();", "    MLTempString();\n    MLTempString *Set(const char *format, ...);",1)
setters={
 "returns-this":setter,
 "returns-void":setter.replace("MLTempString *MLTempString::Set", "void MLTempString::Set").replace("    return this;",""),
 "returns-reference":setter.replace("MLTempString *MLTempString::Set", "MLTempString &MLTempString::Set").replace("    return this;", "    return *this;"),
}
maps={"default":default, "formatted":formatted}
sp=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
rows=[]
for ret,method in setters.items():
 for order in itertools.permutations(("default","formatted","set")):
  for flag,flags in {
      "plain":["-O2"],"rename":["-O2","-frename-registers"],
      "rename-j1":["-O2","-frename-registers","-falign-jumps=1"],
      "plain-j1":["-O2","-falign-jumps=1"],
   }.items():
   for snap in ("jan25","dec06"):
    code=prefix.replace("    MLTempString *Set(",{
        "returns-this":"    MLTempString *Set(",
        "returns-void":"    void Set(",
        "returns-reference":"    MLTempString &Set(",
    }[ret],1)
    # Each reconstructed body is a distinct out-of-class C++ member definition.
    code += "\n\n".join(method if name=="set" else maps[name] for name in order)
    code += "\n"
    tag=ret+"-"+ "-".join(order)+"-"+flag+"-"+snap
    obj=O/(tag+".o")
    path=O/(tag+".cpp");path.write_text(code)
    cmd=["/tmp/vuc-binutils215/g++-"+snap+"-as215",
         *flags,"-c",str(path),"-o",str(obj)]
    cp=subprocess.run(cmd,capture_output=True,text=True)
    row={"return_type":ret,"order":order,"flag":flag,"compiler":snap}
    if cp.returncode: row["compile_error"]=cp.stderr[-360:]
    else:
     for name,config in cfg.items():
      try:
        s=m.run(obj,config)
        row[name]=[s["exact_bytes"],s["length"],s["relocations"]]
        if s["exact"]:row["new_exact"]=name
      except ValueError as e: row[name]=str(e)[:110]
    rows.append(row);print(json.dumps(row,sort_keys=True),flush=True)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
