#!/usr/bin/env python3
"""Test authentic constructor argument lifetimes and gcc 3.3 layout for Canonize.
Target 224 bytes, compare original relocations using match_probe; no forced NOP.
"""
from pathlib import Path
from itertools import product
import importlib.util,subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/canonize-ctor-args-oct14");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/"vuc/probes/canonize_fragment_ctor_args_oct14.cpp").read_text()
prefix=base[:base.index("void Canonize_Reduce(IR::Fragment_Data *fragment)\n{")]
for header in ("MLGameString.h", "MLHashFunGameString.h", "MLHashTable.h"):
 prefix=prefix.replace('#include "../src/'+header+'"', '#include "'+str(R/'vuc/src'/header)+'"')
original=base[base.index("void Canonize_Reduce(IR::Fragment_Data *fragment)\n{"):]
k2="""    if (fragment->kind == 2) {
        if (fragment->stmt) {
            globalEnv.clear();
            Canonize_Reduce(fragment->stmt);
        }
        return;
    }
"""
k1="""    if (fragment->kind != 1) return;

    FragmentHashNode *node =
        new FragmentHashNode(fragment->name, fragment->output,
                             MLHashFun<MLGameString>().Hash(fragment->name));
    unsigned slot = node->hash % 513;
    node->next = (FragmentHashNode *)globalEnv.heads[slot];
    globalEnv.heads[slot] = (Env::Node *)node;
"""
assert k2 in original and k1 in original
forms={
"baseline":original,
"first-1":original.replace(k2+"\n"+k1,
       "    if (fragment->kind == 1) {\n"+k1.replace("    if (fragment->kind != 1) return;","")
        + "    } else"+k2.replace("    if (fragment->kind == 2)", " if (fragment->kind == 2)")),
"two-cases":original.replace(k2+"\n"+k1,
    "    switch(fragment->kind) {\n    case 1: {\n"+k1.replace("    if (fragment->kind != 1) return;","")
    +"      break;\n    }\n    case 2: {\n"
    +k2.replace("    if (fragment->kind == 2) {","").replace("        return;\n    }","")
    +"      break;\n    }\n    }\n"),
"early-test":original.replace("    if (fragment->kind == 2) {",
   "    int kind=fragment->kind;\n    if (kind == 2) {").replace(
   "    if (fragment->kind != 1) return;","    if (kind != 1) return;"),
"swap-next":original.replace(
  "    node->next = (FragmentHashNode *)globalEnv.heads[slot];\n"
  "    globalEnv.heads[slot] = (Env::Node *)node;",
  "    Env::Node *old = globalEnv.heads[slot];\n"
  "    node->next = (FragmentHashNode *)old;\n"
  "    globalEnv.heads[slot] = (Env::Node *)node;"),
"index-local":original.replace(
 "    unsigned slot = node->hash % 513;",
 "    const unsigned hash = node->hash;\n    unsigned slot = hash % 513;"),
"inline-next":original.replace(
  "    unsigned slot = node->hash % 513;\n"
  "    node->next = (FragmentHashNode *)globalEnv.heads[slot];\n"
  "    globalEnv.heads[slot] = (Env::Node *)node;",
  "    Env::Node **bucket = globalEnv.heads + (node->hash % 513);\n"
  "    node->next = (FragmentHashNode *)*bucket;\n"
  "    *bucket = (Env::Node *)node;"),
}
variants={}
for name,source in forms.items():
 variants[name]=source
 variants[name+"-unlikely-kind2"]=source.replace(
   "fragment->kind == 2", "__builtin_expect(fragment->kind == 2, 0)")
flags=[]
for j,l,loop in product((1,2,3,4,8),(1,2,4,8),(1,4)):
 flags.append((f"j{j}-label{l}-loop{loop}",[
     "-O2","-frename-registers",f"-falign-jumps={j}",
     f"-falign-labels={l}",f"-falign-loops={loop}"
 ]))
rows=[]
for name,code in variants.items():
 cpp=T/(name+".cpp");cpp.write_text(prefix+code)
 for label,opts in flags:
  obj=T/(name+"-"+label+".o")
  cc=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,
     "-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  if cc.returncode:
   print("COMPILE_ERROR",name,label,cc.stderr[-150:],flush=True);continue
  symbol=next((x for x in subprocess.check_output(["nm","-S",str(obj)],text=True).splitlines()
     if x.endswith("_Z15Canonize_ReducePN2IR13Fragment_DataE")),"")
  size=int(symbol.split()[1],16) if symbol else -1
  if size==224:
   try:
    score=m.run(obj,R/"vuc/config/canonize_fragment_oct14_probe.json")
    rows.append((score["exact_bytes"],size,name,label,score["different_offsets"][:10]))
    if score["exact"]:
     print("NEW_EXACT",name,label,cpp,flush=True);raise SystemExit(0)
   except ValueError as e:rows.append((0,size,name,label,str(e)[:120]))
  else:rows.append((0,size,name,label,"length"))
 print("DONE",name,flush=True)
print("TOP")
for row in sorted(rows,reverse=True)[:35]:print(row,flush=True)
