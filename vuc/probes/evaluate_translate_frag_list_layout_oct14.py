#!/usr/bin/env python3
"""Follow up the genuine 241-byte VUC fragment-list template skeleton.

The original needs 240 bytes, with a forward exit and body starting at +0x40.
Try historical label/loop/jump controls only on the natural source graph,
never emit padding or manually fixed branch offsets.
"""
from itertools import product
from pathlib import Path
import importlib.util
import subprocess

R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/fraglist-layout-oct14");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/"vuc/probes/translate_frag_list_oct14.cpp").read_text()
first=base[:base.index("\ntemplate ASTNodeList<IR::Fragment_Data *> TranslateFragList<ASTDecl *>")]
variants={"baseline":first}
variants["local-fragment"]=first.replace(
    "        result.Add(TranslateFrag(input.entries[i]));",
    "    { IR::Fragment_Data *fragment = TranslateFrag(input.entries[i]);\n"
    "      result.Add(fragment); }")
variants["separate-index"]=first.replace(
    "    for (int i = 0; i < input.length; ++i)",
    "    int i = 0;\n    for (; i < input.length; ++i)")
scores=[]
for name,src in variants.items():
    cpp=T/(name+".cpp");cpp.write_text(src)
    for jump,loop,label in product((1,2,4,8),(1,4,8,16),(1,2,4,8)):
        key=f"{name}-j{jump}-loop{loop}-label{label}"
        obj=T/(key+".o")
        flags=["-O2","-frename-registers","-fno-reorder-blocks",
               f"-falign-jumps={jump}",f"-falign-loops={loop}",f"-falign-labels={label}"]
        cc=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flags,
                           "-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
        if cc.returncode:
            print("ERROR",key,cc.stderr[-90:],flush=True);continue
        sym=next((x for x in subprocess.check_output(["nm","-S",str(obj)],text=True).splitlines()
          if x.endswith("_Z17TranslateFragListIP10ASTVarDeclE11ASTNodeListIPN2IR13Fragment_DataEERS2_IT_E")),"")
        size=int(sym.split()[1],16) if sym else -1
        if size==240:
            try:
                score=m.run(obj,R/"vuc/config/translate_frag_list_var_oct14_probe.json")
                scores.append((score["exact_bytes"],size,key,score["different_offsets"][:12]))
                if score["exact"]:
                    print("NEW_EXACT",key,flush=True)
                    raise SystemExit(0)
            except ValueError as e:
                scores.append((0,size,key,"reject "+str(e)[:100]))
        else:scores.append((0,size,key,"length"))
    print("DONE",name,flush=True)
print("TOP")
for row in sorted(scores,reverse=True)[:26]:print(row,flush=True)
