#!/usr/bin/env python3
"""Natural AsmOutput definition-order controls for near-exact AlignString.

GCC 3.3 peephole scratch state may depend on genuine method emission order.
Every tested body is unchanged, with true AsmOutput sources and static local
BSS. Compare the original 145-byte function with all its text/BSS relocations;
related source-unit literals are not attributed to AlignString.
"""
from pathlib import Path
import itertools,importlib.util,subprocess,json
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/align-source-order-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
s=(R/"vuc/src/AsmOutputUnit.cpp").read_text()
def between(a,b):return s[s.index(a):s.index(b)]
pieces={
 "pro":between("void AsmOutput_WritePrologue", "void AsmOutput_WriteEpilogue"),
 "epi":between("void AsmOutput_WriteEpilogue", "#include <cstring>"),
 "align":between("char *AlignString(", "extern int constantsTabStop;"),
 "constant":between("char *AlignConstant(", "#include <cstring>\n#include <vector>"),
 "tab":s[s.index("void AsmOutput_TabConstants("):],
}
header='''#include <fstream>
#include <cstring>
#include <vector>
class Instruction {public:const char *ToString();};
class InstructionList {public:std::vector<Instruction *> instructions_;};
extern int constantsTabStop;
char *AlignString(const char *, int);
'''
cfg=json.loads((R/"vuc/config/align_string_probe.json").read_text())
cfg.pop("relocation_literals",None)  # This function references no literal data.
cfg_path=T/"align-own-text-bss.json";cfg_path.write_text(json.dumps(cfg,indent=2))
orders=[
("pro","epi","align","constant","tab"),
("align","pro","epi","constant","tab"),
("pro","align","epi","constant","tab"),
("epi","align","pro","constant","tab"),
("pro","epi","constant","align","tab"),
("pro","epi","tab","align","constant"),
("tab","pro","epi","align","constant"),
("pro","tab","epi","align","constant"),
("tab","align","pro","epi","constant"),
("align","constant","pro","epi","tab"),
("pro","constant","epi","align","tab"),
("epi","pro","align","constant","tab"),
]
flags={
"rename-j4":["-O2","-frename-registers","-falign-jumps=4"],
"rename-j1":["-O2","-frename-registers","-falign-jumps=1"],
"normal-j4":["-O2","-falign-jumps=4"],
"no-weak":["-O2","-frename-registers","-falign-jumps=4","-fno-weak"],
}
scores=[]
for order in orders:
 key="-".join(order)
 source=T/(key+".cpp");source.write_text(header+"\n".join(pieces[p] for p in order))
 for label,options in flags.items():
  obj=T/(key+"-"+label+".o")
  cc=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*options,"-c",str(source),"-o",str(obj)],capture_output=True,text=True)
  if cc.returncode:print("COMPILE",key,label,cc.stderr[-115:],flush=True);continue
  nm=subprocess.check_output(["nm","-S",str(obj)],text=True)
  symbol=next((line for line in nm.splitlines() if line.endswith("_Z11AlignStringPKci")),"")
  size=int(symbol.split()[1],16) if symbol else -1
  if size!=145:scores.append((0,size,key,label));continue
  try:
   score=m.run(obj,cfg_path)
   scores.append((score["exact_bytes"],size,key,label,score["different_offsets"][:7]))
   if score["exact"]:
    print("NEW_EXACT",source,label,flush=True)
    raise SystemExit(0)
  except ValueError as e:print("REJECT",key,label,str(e)[:80],flush=True)
 print("DONE",key,flush=True)
print("BEST")
for row in sorted(scores,reverse=True)[:32]:print(row)
