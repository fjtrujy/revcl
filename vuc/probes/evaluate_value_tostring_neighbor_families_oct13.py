#!/usr/bin/env python3
"""Probe original Value definition-family neighbors before ToString.

Source definitions are taken from actual recovered C++ methods, in
original ELF address order. No dummy/forcing functions are injected.
The 82 literal bytes and eight call/address relocations remain strictly
authenticated against the real 2004 ELF.
"""
from pathlib import Path
import importlib.util,subprocess,struct,json
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/value-tostring-neighbor-families-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
raw=(R/"vuc/src/ValueArithmeticUnit.cpp").read_text().replace("    Value operator-(const Value &other) const;","    Value operator-(const Value &other) const;\n    Value operator/(const Value &other) const;\n    MLGameString ToString() const;")
decls={"compare":'''    Value operator==(const Value &) const;
    Value operator!=(const Value &) const;''',
 "relational":'''    Value operator>(const Value &) const;
    Value operator<(const Value &) const;
    Value operator>=(const Value &) const;
    Value operator<=(const Value &) const;''',
 "logical":'''    Value operator&&(const Value &) const;
    Value operator||(const Value &) const;''',
 "bitwise":'''    Value operator&(const Value &) const;
    Value operator|(const Value &) const;'''}
addition=raw[:raw.index("Value Value::operator+(")]
operators=raw[raw.index("Value Value::operator+("):]
prefix='#include <cstdio>\n#include "'+str(R/"vuc/src/MLGameString.h")+'"\n'
division=(R/"vuc/probes/value_division_reciprocal.cpp").read_text()
division=division[division.index("Value Value::operator/("):]
comps=(R/"vuc/probes/value_comparison.cpp").read_text()
comps=comps[comps.index("Value Value::operator==("):]
relations=(R/"vuc/probes/value_relational.cpp").read_text()
relations=relations[relations.index("Value Value::operator>("):]
logical=(R/"vuc/probes/value_logical.cpp").read_text()
logical=logical[logical.index("Value Value::operator&&("):]
bitwise=(R/"vuc/probes/value_bitwise.cpp").read_text()
bitwise=bitwise[bitwise.index("Value Value::operator&("):]
tostring=(R/"vuc/probes/value_to_string_oct13.cpp").read_text()
tostring=tostring[tostring.index("MLGameString Value::ToString() const"):]
components={"division":division,"compare":comps,"relational":relations,"logical":logical,"bitwise":bitwise}
choices=[
 (),("division",),("compare",),("division","compare"),
 ("division","compare","relational"),("division","compare","relational","logical"),
 ("division","compare","relational","logical","bitwise")
]
def rodata(obj):
 b=obj.read_bytes();shoff=struct.unpack_from('<I',b,0x20)[0];shsize,n,idx=struct.unpack_from('<HHH',b,0x2e)
 sh=[struct.unpack_from('<IIIIIIIIII',b,shoff+i*shsize) for i in range(n)]
 off,size=sh[idx][4:6];names=b[off:off+size]
 for s in sh:
  p=s[0];nm=names[p:names.find(b'\0',p)]
  if nm==b'.rodata':off,length=s[4:6];return b[off:off+length]
 raise ValueError("no rodata")
known={b'%d\0':"0x08072971",b'%f,%f,%f,%f\0':"0x08076690",b'%f\0':"0x08076699"}
base=json.loads((R/"vuc/config/value_to_string_oct13_probe.json").read_text())
flags={"j4":["-O2","-frename-registers","-falign-jumps=4"],
"j4-fast":["-O2","-frename-registers","-falign-jumps=4","-ffast-math"],
"j4-sections":["-O2","-frename-registers","-falign-jumps=4","-ffunction-sections"],
"j4-no-gcse":["-O2","-frename-registers","-falign-jumps=4","-fno-gcse"]}
for combo in choices:
 name="-".join(combo) if combo else "arithmetic"
 front=addition
 for family in combo:
  if family in decls:front=front.replace("    MLGameString ToString() const;",decls[family]+"\n    MLGameString ToString() const;")
 if "bitwise" in combo:
  front="enum TypeID { ValueInteger = 1 };\n"+front
  front=front.replace("    MLGameString ToString() const;","    static bool TypeConvertForce(TypeID, const Value &, const Value &, Value &, Value &);\n    MLGameString ToString() const;")
 unit=prefix+front+operators+"\n"+"\n".join(components[part] for part in combo)+"\n"+tostring
 src=T/(name+".cpp");src.write_text(unit)
 for label,options in flags.items():
  obj=T/(name+"-"+label+".o")
  comp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*options,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
  if comp.returncode:print("ERROR",name,label,comp.stderr[-140:],flush=True);continue
  content=rodata(obj)
  if content[:64]!=bytes(64):
   print("BAD_INITIALIZER",name,label,len(content),flush=True);continue
  cur=64;ranges=[{"offset":0,"size":64,"address":"0x0807669c"}]
  try:
   seen=set()
   while cur<len(content):
    end=content.find(b'\0',cur)
    if end<0:raise ValueError("unterminated literal")
    string=content[cur:end+1]
    if string not in known or string in seen:raise ValueError("unverified extra literal "+repr(string))
    ranges.append({"offset":cur,"size":len(string),"address":known[string]});cur=end+1;seen.add(string)
   if seen!=set(known):raise ValueError("missing authenticated literal")
   config=dict(base);config["relocation_literals"]={".rodata":ranges};config["source"]=str(src)
   path=T/(name+"-"+label+".json");path.write_text(json.dumps(config,indent=2)+"\n")
   score=m.run(obj,path)
   print("MATCH",name,label,score["exact_bytes"],"/",score["length"],"literals",score["verified_literal_bytes"],flush=True)
   if score["exact"]:print("NEW_EXACT",src,options,flush=True)
  except ValueError as e:print("REJECT",name,label,str(e)[:115],flush=True)
