#!/usr/bin/env python3
"""Genuine C++03 IR::Seq_Data construction-layout variants for VUC.

The binary's Seq_Data is 16 bytes with a vptr, tag, and two child pointers.
Test natural field init/body, base class and real list-iteration forms;
require 119-byte length and the original three relocations for scoring.
"""
from pathlib import Path
from itertools import product
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/translate-seq-layout-next");T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
base=(R/"vuc/probes/translate_stmt_list.cpp").read_text().replace(
 '"../src/ASTNodeList.h"','"'+str(R/"vuc/src/ASTNodeList.h")+'"')
piece=base[base.index("namespace IR {"):base.index("IR::Stmt_Data *Translate(")]
begin=piece.index("class Seq_Data")
end=piece.index("\n};",begin)+3
variants={
 "original":piece[begin:end],
 "ctor-body":"""class Seq_Data : public Data {
public:
    virtual ~Seq_Data();
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) : Data() {
        first = a; second = b;
    }
};""",
 "ctor-body-reverse":"""class Seq_Data : public Data {
public:
    virtual ~Seq_Data();
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) : Data() {
        second = b; first = a;
    }
};""",
 "ctor-inverted":"""class Seq_Data : public Data {
public:
    virtual ~Seq_Data();
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) : Data(), second(b), first(a) {}
};""",
 "ctor-no-base":"""class Seq_Data : public Data {
public:
    virtual ~Seq_Data();
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) : first(a), second(b) {}
};""",
 "ctor-second-first":"""class Seq_Data : public Data {
public:
    virtual ~Seq_Data();
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) : Data(), first(a) {
        second = b;
    }
};""",
 "ctor-first-second":"""class Seq_Data : public Data {
public:
    virtual ~Seq_Data();
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) : Data(), second(b) {
        first = a;
    }
};""",
 "seq-inherit-stmt":"""class Seq_Data : public Stmt_Data {
public:
    virtual ~Seq_Data();
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) : Stmt_Data(), first(a), second(b) {}
};""",
 "ctor-stmt-body":"""class Seq_Data : public Stmt_Data {
public:
    virtual ~Seq_Data();
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) : Stmt_Data() {
       first = a; second = b;
    }
};""",
}
body=base[base.index("IR::Stmt_Data *Translate(ASTStmt *);"):]
start=body.index("    IR::Stmt_Data *result = 0;")
stop=body.index("    return result;",start)+len("    return result;")
loops={
 "base":body[start:stop],
 "local-index":body[start:stop].replace("for (int i=0; i<statements.length; ++i)", "int i = 0;\n    for (; i<statements.length; ++i)"),
 "reverse-branch":body[start:stop].replace("if (result)\n                result=(IR::Stmt_Data *)new IR::Seq_Data(result, item);\n            else\n                result=item;", "if (!result) result=item;\n            else result=(IR::Stmt_Data *)new IR::Seq_Data(result, item);"),
 "continue":"""    IR::Stmt_Data *result = 0;
    for (int i=0; i<statements.length; ++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (!item) continue;
        if (!result) result = item;
        else result=(IR::Stmt_Data *)new IR::Seq_Data(result,item);
    }
    return result;""",
 "while":"""    IR::Stmt_Data *result = 0;
    int i = 0;
    while (i < statements.length) {
        IR::Stmt_Data *item=Translate(statements.entries[i++]);
        if (item) {
            if (result) result=(IR::Stmt_Data *)new IR::Seq_Data(result,item);
            else result=item;
        }
    }
    return result;""",
}
settings={
 "ren":["-O2","-frename-registers"],
 "plain":["-O2"],
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j4":["-O2","-frename-registers","-falign-jumps=4"]
}
scores=[]
for n,constructor in variants.items():
 for label,loop in loops.items():
  key=n+"-"+label
  src=T/(key+".cpp")
  src.write_text(base[:base.index("namespace IR {")]+piece[:begin]+constructor+piece[end:]+body[:start]+loop+body[stop:])
  for f,opt in settings.items():
   o=T/(key+"-"+f+".o")
   cc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opt,"-c",str(src),"-o",str(o)],capture_output=True,text=True)
   if cc.returncode:print("COMPILE_FAIL",key,f,cc.stderr[-95:],flush=True);continue
   row=next((r for r in subprocess.check_output(["nm","-S",str(o)],text=True).splitlines() if r.split()[-1:]==["_Z9TranslateR11ASTNodeListIP7ASTStmtE"]),"")
   sz=int(row.split()[1],16) if row else -1
   if sz!=119:
    scores.append((0,sz,key,f));continue
   try:
    x=m.run(o,R/"vuc/config/translate_stmt_list_probe.json")
    scores.append((x["exact_bytes"],sz,key,f,x["different_offsets"][:10]))
    print("ORIGINAL_SIZE",key,f,x["exact_bytes"],x["different_offsets"][:12],flush=True)
    if x["exact"]:
     print("NEW_EXACT",src,f,flush=True);raise SystemExit(0)
   except ValueError as e:print("REJECT",key,f,str(e)[:110],flush=True)
 print("DONE",n,flush=True)
print("TOP")
for s in sorted(scores,reverse=True)[:28]:print(s)
