#!/usr/bin/env python3
"""Explore real ASTNodeList Add() liveness for unresolved VUC fragments.

Original template stores old list length in a 32-bit stack home across
allocation; compare source-level count/owner lifetimes before changing
the scanner or accepted code. Generated variants remain in /tmp only.
"""
from pathlib import Path
from itertools import product
import importlib.util, subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/fraglist-lifetimes-oct14");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/"vuc/probes/translate_frag_list_oct14.cpp").read_text()
start=base.index("    void Add(T element) {")
end=base.index("\n};",start)
prefix=base[:start]; suffix=base[end:base.index("\ntemplate ASTNodeList<IR::Fragment_Data *> TranslateFragList<ASTVarDecl *>")]
forms={
"local-count":"""    void Add(T element) {
        int count = length;
        if (count >= capacity) {
            int new_capacity = capacity ? 2 * capacity : 16;
            T *old_entries = entries;
            entries = new T[new_capacity];
            if (old_entries)
                __builtin_memcpy(entries, old_entries, count * sizeof(T));
            capacity = new_capacity;
        }
        entries[count] = element;
        length = count + 1;
    }""",
"local-count-early-cap":"""    void Add(T element) {
        int count = length;
        if (count >= capacity) {
            int new_capacity = capacity ? 2 * capacity : 16;
            T *old_entries = entries;
            capacity = new_capacity;
            entries = new T[new_capacity];
            if (old_entries)
                __builtin_memcpy(entries, old_entries, count * sizeof(T));
        }
        entries[count] = element;
        length = count + 1;
    }""",
"local-count-late-assign":"""    void Add(T element) {
        int count = length;
        if (count >= capacity) {
            int new_capacity = capacity ? 2 * capacity : 16;
            T *old_entries = entries;
            T *new_entries = new T[new_capacity];
            if (old_entries)
                __builtin_memcpy(new_entries, old_entries, count * sizeof(T));
            capacity = new_capacity;
            entries = new_entries;
        }
        entries[count] = element;
        length = count + 1;
    }""",
"local-count-after":"""    void Add(T element) {
        if (length >= capacity) {
            int count = length;
            int new_capacity = capacity ? 2 * capacity : 16;
            T *old_entries = entries;
            entries = new T[new_capacity];
            if (old_entries)
                __builtin_memcpy(entries, old_entries, count * sizeof(T));
            capacity = new_capacity;
        }
        int count = length;
        entries[count] = element;
        length = count + 1;
    }""",
"local-count-saved-new":"""    void Add(T element) {
        int count = length;
        if (count >= capacity) {
            int new_capacity = capacity ? 2 * capacity : 16;
            T *old_entries = entries;
            T *new_entries = new T[new_capacity];
            entries = new_entries;
            if (old_entries)
                __builtin_memcpy(new_entries, old_entries, count * sizeof(T));
            capacity = new_capacity;
        }
        entries[count] = element;
        length = count + 1;
    }""",
"count-after-pointer":"""    void Add(T element) {
        int old_size = length;
        if (old_size >= capacity) {
            int new_capacity = capacity ? 2 * capacity : 16;
            T *old_entries = entries;
            T *new_entries = new T[new_capacity];
            entries = new_entries;
            if (old_entries)
                __builtin_memcpy(new_entries, old_entries, old_size * sizeof(T));
            capacity = new_capacity;
        }
        ++length;
        entries[old_size] = element;
    }""",
}
variants={"var":("ASTVarDecl","vuc/config/translate_frag_list_var_oct14_probe.json"),
          "decl":("ASTDecl","vuc/config/translate_frag_list_decl_oct14_probe.json")}
flags={
"noreorder":["-O2","-frename-registers","-fno-reorder-blocks","-falign-loops=16","-falign-jumps=1"],
"j1":["-O2","-frename-registers","-falign-loops=16","-falign-jumps=1"],
"noreorder-j4":["-O2","-frename-registers","-fno-reorder-blocks","-falign-loops=16","-falign-jumps=4"],
"noreorder-label2":["-O2","-frename-registers","-fno-reorder-blocks","-falign-loops=16","-falign-jumps=1","-falign-labels=2"]
}
rows=[]
for (kind,(type_name, config)),(name,add), (label,opts) in product(variants.items(),forms.items(),flags.items()):
    cpp=T/(kind+"-"+name+".cpp")
    cpp.write_text(prefix+add+suffix+f"\ntemplate ASTNodeList<IR::Fragment_Data *> TranslateFragList<{type_name} *>(ASTNodeList<{type_name} *> &);\n")
    obj=T/(kind+"-"+name+"-"+label+".o")
    cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
    if cp.returncode:
        print("ERROR",kind,name,label,cp.stderr[-115:],flush=True);continue
    sym=next((x for x in subprocess.check_output(["nm","-S",str(obj)],text=True).splitlines() if "TranslateFragList" in x),"")
    size=int(sym.split()[1],16) if sym else -1
    if size==240:
        try:
            score=m.run(obj,R/config)
            rows.append((score["exact_bytes"],size,kind,name,label,score["different_offsets"][:10]))
            if score["exact"]:
                print("NEW_EXACT",kind,name,label,cpp,flush=True);raise SystemExit(0)
        except ValueError as e:rows.append((0,size,kind,name,label,str(e)[:110]))
    else:rows.append((0,size,kind,name,label,"length"))
    if label=="noreorder-label2":print("CHECKED",kind,name,flush=True)
for kind in variants:
    print("TOP",kind,flush=True)
    for x in sorted((x for x in rows if x[2]==kind),reverse=True)[:20]:print(x,flush=True)
