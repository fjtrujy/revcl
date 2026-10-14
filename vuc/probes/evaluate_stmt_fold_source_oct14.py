#!/usr/bin/env python3
"""Source-structural hypotheses for the 119-byte ASTStmt list fold.

Only genuine ASTNodeList loop, original IR::Seq_Data constructor and
historical compiler. Check original-length objects against relocations.
"""
from pathlib import Path
import importlib.util
import subprocess
from itertools import product

R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/stmt-fold-rebase-oct14')
T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('matcher',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
original=(R/'vuc/probes/translate_stmt_list.cpp').read_text()
original=original.replace('#include "../src/ASTNodeList.h"','#include "'+str(R/'vuc/src/ASTNodeList.h')+'"')
start=original.index('IR::Stmt_Data *Translate(ASTNodeList<ASTStmt *> &statements)\n{')
prefix=original[:start]
funcs={}
bodies={
"baseline":"""    IR::Stmt_Data *result = 0;
    for (int i=0; i<statements.length; ++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (item) {
            if (result)
                result=(IR::Stmt_Data *)new IR::Seq_Data(result, item);
            else
                result=item;
        }
    }
    return result;""",
"early-continue":"""    IR::Stmt_Data *result=0;
    for(int i=0;i<statements.length;++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (!item) continue;
        if (!result) { result=item; continue; }
        result=(IR::Stmt_Data*)new IR::Seq_Data(result,item);
    }
    return result;""",
"goto-late":"""    IR::Stmt_Data *result=0;
    int i=0;
    while(i<statements.length) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (!item) goto next_item;
        if (!result) {result=item;goto next_item;}
        result=(IR::Stmt_Data*)new IR::Seq_Data(result,item);
next_item:
        ++i;
    }
    return result;""",
"result-reference":"""    IR::Stmt_Data *result=0;
    IR::Stmt_Data *&accumulator=result;
    for(int i=0;i<statements.length;++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (!item) continue;
        if (accumulator) accumulator=(IR::Stmt_Data*)new IR::Seq_Data(accumulator,item);
        else accumulator=item;
    }
    return result;""",
"result-address":"""    IR::Stmt_Data *result=0;
    IR::Stmt_Data **slot=&result;
    for(int i=0;i<statements.length;++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (!item) continue;
        if (*slot) *slot=(IR::Stmt_Data*)new IR::Seq_Data(*slot,item);
        else *slot=item;
    }
    return result;""",
"result-volatile":"""    IR::Stmt_Data *volatile result=0;
    for(int i=0;i<statements.length;++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (!item) continue;
        if (result) result=(IR::Stmt_Data*)new IR::Seq_Data(result,item);
        else result=item;
    }
    return result;""",
"local-old":"""    IR::Stmt_Data *result=0;
    for(int i=0;i<statements.length;++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (!item) continue;
        IR::Stmt_Data *old=result;
        if (old) result=(IR::Stmt_Data*)new IR::Seq_Data(old,item);
        else result=item;
    }
    return result;""",
"if-no-result":"""    IR::Stmt_Data *result=0;
    for(int i=0;i<statements.length;++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if(item!=0 && result==0) result=item;
        else if(item!=0 && result!=0) result=(IR::Stmt_Data*)new IR::Seq_Data(result,item);
    }
    return result;""",
"list-pointer":"""    IR::Stmt_Data *result=0;
    ASTNodeList<ASTStmt*> *list=&statements;
    for(int i=0;i<list->length;++i) {
        IR::Stmt_Data *item=Translate(list->entries[i]);
        if (!item) continue;
        if(result) result=(IR::Stmt_Data*)new IR::Seq_Data(result,item);
        else result=item;
    }
    return result;""",
"node-pointer":"""    IR::Stmt_Data *result=0;
    for(int i=0;i<statements.length;++i) {
        ASTStmt **cursor=&statements.entries[i];
        IR::Stmt_Data *item=Translate(*cursor);
        if (!item) continue;
        if(result) result=(IR::Stmt_Data*)new IR::Seq_Data(result,item);
        else result=item;
    }
    return result;"""
}
variants={}
for name,body in bodies.items():
 variants[name]=body
 variants[name+"-signed-index"]=body.replace("int i=0;","unsigned i=0;").replace("int i = 0;","unsigned i = 0;").replace("int i=0;","unsigned i=0;")
variants["baseline-index-post"]=bodies["baseline"].replace("++i","i++")
variants["baseline-result-before-item"]=bodies["baseline"].replace("IR::Stmt_Data *item=Translate(statements.entries[i]);","IR::Stmt_Data *old=result;\n        IR::Stmt_Data *item=Translate(statements.entries[i]);").replace("if (result)\n", "if (old)\n").replace("new IR::Seq_Data(result, item)","new IR::Seq_Data(old, item)")
flags={
 "standard":["-O2","-frename-registers"],
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j4":["-O2","-frename-registers","-falign-jumps=4"],
 "noreorder":["-O2","-frename-registers","-fno-reorder-blocks"],
 "noreorder-j1":["-O2","-frename-registers","-fno-reorder-blocks","-falign-jumps=1"],
 "noreorder-j4":["-O2","-frename-registers","-fno-reorder-blocks","-falign-jumps=4"],
}
rows=[]
for name,body in variants.items():
 src=T/(name+".cpp");src.write_text(prefix+"IR::Stmt_Data *Translate(ASTNodeList<ASTStmt *> &statements)\n{\n"+body+"\n}\n")
 for label,options in flags.items():
  obj=T/(name+"-"+label+".o")
  compiled=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*options,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
  if compiled.returncode:
   print("FAIL",name,label,compiled.stderr[-80:],flush=True);continue
  nm=subprocess.check_output(["nm","-S",str(obj)],text=True)
  symbol=next((x for x in nm.splitlines() if x.split()[-1:]==["_Z9TranslateR11ASTNodeListIP7ASTStmtE"]),"")
  size=int(symbol.split()[1],16) if symbol else -1
  if size!=119:rows.append((0,size,name,label,"size"));continue
  try:
   score=m.run(obj,R/"vuc/config/translate_stmt_list_probe.json")
   rows.append((score["exact_bytes"],size,name,label,score["different_offsets"][:10]))
   if score["exact"]:
    print("FULL_EXACT",name,label,src,flush=True);raise SystemExit(0)
  except ValueError as e:rows.append((0,size,name,label,str(e)[:90]))
 print("DONE",name,flush=True)
print("BEST")
for row in sorted(rows,reverse=True)[:30]:print(row,flush=True)
