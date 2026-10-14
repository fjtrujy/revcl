#!/usr/bin/env python3
"""Source-controlled structural hypotheses for VUC Canonize_Reduce(Fragment*).
Check authentic calls and relocations; no patched bytes or artificial padding.
"""
from pathlib import Path
import subprocess
import importlib.util
import itertools

R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/canonize-fragment-oct14")
T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/"vuc/probes/canonize_fragment_oct14.cpp").read_text()
base=base.replace('#include "../src/MLGameString.h"','#include "'+str(R/"vuc/src/MLGameString.h")+'"')
base=base.replace('#include "../src/MLHashFunGameString.h"','#include "'+str(R/"vuc/src/MLHashFunGameString.h")+'"')
base=base.replace('#include "../src/MLHashTable.h"','#include "'+str(R/"vuc/src/MLHashTable.h")+'"')
start=base.index("void Canonize_Reduce(IR::Fragment_Data *fragment)\n{")
pre=base[:start]
body=base[start:]
kind1=body[body.index("    if (fragment->kind == 1) {")+len("    if (fragment->kind == 1) {"):
           body.index("    } else if (fragment->kind == 2 && fragment->stmt) {")]
kind2=body[body.index("        globalEnv.clear();"):body.index("    }\n}",body.index("        globalEnv.clear();"))]
common="void Canonize_Reduce(IR::Fragment_Data *fragment)\n{\n"
body_forms={
  "kind1-first":body,
  "kind2-first":common+
      "    if (fragment->kind == 2) {\n"
      "        if (fragment->stmt) {\n"+kind2+"        }\n"
      "    } else if (fragment->kind == 1) {"+kind1+"    }\n}\n",
  "kind2-first-return":common+
      "    if (fragment->kind == 2) {\n"
      "        if (fragment->stmt) {\n"+kind2+"        }\n"
      "        return;\n"
      "    }\n"
      "    if (fragment->kind == 1) {"+kind1+"    }\n}\n",
  "switch":common+
      "    switch (fragment->kind) {\n"
      "    case 2: if (fragment->stmt) {\n"+kind2+"       }\n       break;\n"
      "    case 1: {"+kind1+"       break;\n     }\n"
      "    }\n}\n",
  "early-default":common+
      "    int tag = fragment->kind;\n"
      "    if (tag != 1 && tag != 2) return;\n"
      "    if (tag == 2) { if (fragment->stmt) {\n"+kind2+"      }\n"
      "    } else {"+kind1+"    }\n}\n",
}
def new_first(s):
    s=s.replace(
       "        unsigned hash = MLHashFun<MLGameString>().Hash(fragment->name);\n"
       "        Node *node = static_cast<Node *>(::operator new(sizeof(Node)));",
       "        Node *node = static_cast<Node *>(::operator new(sizeof(Node)));\n"
       "        unsigned hash = MLHashFun<MLGameString>().Hash(fragment->name);")
    return s
def index_before(s):
    return s.replace("        unsigned bucket = hash % 513;\n", "        unsigned bucket = hash % 513;\n")
forms={}
for n,s in body_forms.items():
    forms[n]=s
    forms[n+"-new-first"]=new_first(s)
    forms[n+"-new-first-inline"]=new_first(s).replace(
      "        unsigned bucket = hash % 513;","        unsigned bucket = node->hash % 513;")
    forms[n+"-hashlocal"]=s.replace(
      "        unsigned hash = MLHashFun<MLGameString>().Hash(fragment->name);",
      "        MLHashFun<MLGameString> hasher;\n"
      "        unsigned hash = hasher.Hash(fragment->name);")
flags={
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j4":["-O2","-frename-registers","-falign-jumps=4"],
 "j8":["-O2","-frename-registers","-falign-jumps=8"],
 "plain":["-O2","-falign-jumps=1"],
 "no-reorder":["-O2","-frename-registers","-fno-reorder-blocks","-falign-jumps=1"],
 "no-reorder-j4":["-O2","-frename-registers","-fno-reorder-blocks","-falign-jumps=4"],
}
rows=[]
for name,body in forms.items():
    source=T/(name+".cpp");source.write_text(pre+body)
    for fl,opts in flags.items():
        obj=T/(name+"-"+fl+".o")
        cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,
            "-c",str(source),"-o",str(obj)],capture_output=True,text=True)
        if cp.returncode:
            print("ERROR",name,fl,cp.stderr[-125:],flush=True);continue
        nm=subprocess.check_output(["nm","-S",str(obj)],text=True)
        line=next((x for x in nm.splitlines()
                 if x.endswith("_Z15Canonize_ReducePN2IR13Fragment_DataE")),"")
        size=int(line.split()[1],16) if line else -1
        if size==224:
            try:
                result=m.run(obj,R/"vuc/config/canonize_fragment_oct14_probe.json")
                rows.append((result["exact_bytes"],size,name,fl,result["different_offsets"][:12]))
                if result["exact"]:
                    print("NEW_EXACT",name,fl,source,flush=True);raise SystemExit(0)
            except ValueError as e:rows.append((0,size,name,fl,"reject "+str(e)[:125]))
        else:rows.append((0,size,name,fl,"size"))
    print("DONE",name,flush=True)
print("TOP")
for row in sorted(rows,reverse=True)[:35]:print(row,flush=True)
