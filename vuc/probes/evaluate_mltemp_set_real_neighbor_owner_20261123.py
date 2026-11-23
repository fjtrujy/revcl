#!/usr/bin/env python3
"""Try original MLTempString.cpp neighbor/owner contexts to recover Set.

The 117-byte original Set retains 6 mismatches (three scratch register
allocations). The two C2 constructors became exact when genuine method
definition order changed. This test brings their real original ELF neighbors:
Get, FormatString, operator+= and operator+ into one compilation unit.

No dummy methods, assembler, synthetic relocation, source padding, or accepted
production-owner edits. A strict original-length/relocation check is mandatory.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess
V=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-mltemp-neighbors-20261123");O.mkdir(exist_ok=True)
h=V/"src/MLTempStringData.h"
preamble=f"""#include "{h}"
#include <cstdarg>
#include <cstring>
struct MLTempString {{
  const char *text;
  MLTempString();
  MLTempString(const char *format, ...);
  MLTempString *Set(const char *format, ...);
  MLTempString &operator+=(const MLTempString &other);
  MLTempString operator+(const MLTempString &other);
}};
"""
getter="""inline MLTempStringData *MLTempStringData::Get()
{
    static MLTempStringData tsd;
    return &tsd;
}
"""
fbody="""
MLTempString::MLTempString(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
}
"""
dbody="""
MLTempString::MLTempString()
{
    text = MLTempStringData::Get()->FormatString("");
}
"""
sbody="""
MLTempString *MLTempString::Set(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    text = MLTempStringData::Get()->VFormatString(format, (char *)arguments);
    va_end(arguments);
    return this;
}
"""
plus_assign="""
MLTempString &MLTempString::operator+=(const MLTempString &other)
{
    const char *second = other.text;
    const char *first = text;
    Set("%s%s", first, second);
    return *this;
}
"""
plus="""
MLTempString MLTempString::operator+(const MLTempString &other)
{
    const char *second = other.text;
    const char *first = text;
    return MLTempString("%s%s", first, second);
}
"""
format_string="""
char *MLTempStringData::FormatString(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    char *result = VFormatString(format, (char *)arguments);
    va_end(arguments);
    return result;
}
"""
get_definition="""
MLTempStringData *MLTempStringData::Get()
{
    static MLTempStringData tsd;
    return &tsd;
}
"""
cfg=V/"config/ml_temp_set_probe.json"
sp=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
# Purely legitimate source order alternatives, progressively incorporating
# real historically neighboring operators and string-format owner.
recipes={}
methods={"F":fbody,"D":dbody,"S":sbody,"A":plus_assign,"P":plus}
for order in itertools.permutations("FDS"):
    for prefix in ("","A","P","AP"):
        for suffix in ("","A","P","AP"):
            if set(prefix)&set(suffix):continue
            tag="-".join(order)+"-pre"+(prefix or "0")+"-post"+(suffix or "0")
            seq=prefix+"".join(order)+suffix
            recipes[tag]="\n".join(methods[k] for k in seq)
for order in ("FDS","DFS","FSD","SFD","SDF","DSF"):
    for pos in ("before","after"):
        for extra in ("fmt","get","fmt-and-ops"):
            base="\n".join(methods[k] for k in order)
            definition=(format_string if extra in ("fmt","fmt-and-ops") else get_definition)
            if extra=="fmt-and-ops":definition+="\n"+plus_assign+"\n"+plus
            recipes[order+"-"+extra+"-"+pos]=(
                definition+"\n"+base if pos=="before" else base+"\n"+definition
            )
opts={
 "o2":["-O2"], "rename":["-O2","-frename-registers"],
 "j1":["-O2","-falign-jumps=1"],
 "rename-j1":["-O2","-frename-registers","-falign-jumps=1"],
 "rename-j8":["-O2","-frename-registers","-falign-jumps=8"],
}
rows=[]
for tag,code in recipes.items():
    for name,flags in opts.items():
        cpp=O/(tag+"-"+name+".cpp")
        body=preamble+(get_definition if "get-" in tag else getter)+code
        # A real out-of-line Get definition cannot coexist with the inline
        # definition. Replace the header if the source body owns Get.
        if "-get-" in tag:
            body=preamble+code
        cpp.write_text(body)
        for snap in ("jan25","dec06"):
            obj=O/(tag+"-"+name+"-"+snap+".o")
            cp=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215",*flags,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
            row={"tag":tag,"flags":name,"snap":snap}
            if cp.returncode:row["error"]=cp.stderr[-240:]
            else:
                try:
                    s=m.run(obj,cfg)
                    row.update(size=s["length"],score=s["exact_bytes"],relocs=s["relocations"],different=s["different_offsets"])
                except ValueError as exc:row["unverified"]=str(exc)
            print(json.dumps(row,sort_keys=True),flush=True);rows.append(row)
            if row.get("score",0)>111:
                (O/"promising.json").write_text(json.dumps(row,indent=2)+"\n")
                print("NEW_GAIN",row,flush=True)
                if row["score"]==117:
                    (O/"results.json").write_text(json.dumps(rows,indent=2)+"\n");raise SystemExit(0)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
