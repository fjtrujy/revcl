#!/usr/bin/env python3
"""Test Value::ToString within genuine arithmetic/comparison predecessor units.

Exact offsets for the original 64-byte initializer and three formats are
verified against the ZIP-backed ELF. Do not infer that original symbol order
alone proves a complete original translation unit.
"""
from pathlib import Path
import importlib.util,subprocess,struct,json
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/value-tostring-predecessors-oct13");T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("matcher",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
original=(R/"vuc/probes/value_to_string_oct13.cpp").read_text()
body=original[original.index("MLGameString Value::ToString() const"):]
cases={}
for key,file,member in [
 ("comparison-context","vuc/src/Value.cpp","    Value operator<=(const Value &other) const;"),
 ("arithmetic-context","vuc/src/ValueArithmeticUnit.cpp","    Value operator-(const Value &other) const;"),
]:
 unit=(R/file).read_text().replace(member,member+"\n    MLGameString ToString() const;")
 unit='#include <cstdio>\n#include "'+str(R/"vuc/src/MLGameString.h")+'"\n'+unit
 cases[key]=unit+"\n"+body
# TypeConvert is a genuine following definition, from the accepted C++
# source; do not fake any predecessor or follow-up bytes.
type_def=(R/"vuc/src/ValueTypeConvert.cpp").read_text()
type_def=type_def[type_def.index("bool Value::TypeConvert("):]
cases["arithmetic-type-following"]=cases["arithmetic-context"]+"\n"+type_def
options={"j4":["-O2","-frename-registers","-falign-jumps=4"],"j1":["-O2","-frename-registers","-falign-jumps=1"],"ren":["-O2","-frename-registers"],"plain-j4":["-O2","-falign-jumps=4"]}
def section(obj,name):
 b=obj.read_bytes();off=struct.unpack_from('<I',b,0x20)[0];size,count,strndx=struct.unpack_from('<HHH',b,0x2e)
 sh=[struct.unpack_from('<IIIIIIIIII',b,off+i*size) for i in range(count)]
 stroff,strlen=sh[strndx][4:6];names=b[stroff:stroff+strlen]
 for s in sh:
  pos=s[0];nm=names[pos:names.find(b'\0',pos)]
  if nm==name:
   st,length=s[4:6];return b[st:st+length]
 raise ValueError("missing " + str(name))
known={b'%d\0':"0x08072971",b'%f,%f,%f,%f\0':"0x08076690",b'%f\0':"0x08076699"}
base=json.loads((R/"vuc/config/value_to_string_oct13_probe.json").read_text())
for name,source in cases.items():
 src=T/(name+".cpp");src.write_text(source)
 for opt,flags in options.items():
  obj=T/(name+"-"+opt+".o")
  cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flags,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
  if cp.returncode:
   print(name,opt,"COMPILE",cp.stderr[-150:],flush=True);continue
  data=section(obj,b".rodata")
  if not data.startswith(bytes(64)):
   print(name,opt,"UNKNOWN_INITIALIZER",data[:12].hex(),flush=True);continue
  ranges=[{"offset":0,"size":64,"address":"0x0807669c"}]
  cursor=64
  try:
   while cursor<len(data):
    nul=data.find(b'\0',cursor)
    key=data[cursor:nul+1]
    if nul<0 or key not in known:raise ValueError("unexpected original-literal data at "+str(cursor)+": "+repr(key))
    ranges.append({"offset":cursor,"size":len(key),"address":known[key]})
    cursor=nul+1
   cfg=dict(base);cfg["relocation_literals"]={".rodata":ranges};cfg["source"]=str(src)
   path=T/(name+"-"+opt+".json");path.write_text(json.dumps(cfg,indent=2)+"\n")
   result=m.run(obj,path)
   print(name,opt,result["exact_bytes"],"/",result["length"],"relocs",result["relocations"],"literals",result["verified_literal_bytes"],flush=True)
   if result["exact"]:print("NEW_EXACT",src,flags,flush=True)
  except ValueError as e:print(name,opt,"REJECT",str(e)[:140],flush=True)
