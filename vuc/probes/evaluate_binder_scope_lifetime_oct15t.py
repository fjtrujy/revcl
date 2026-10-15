#!/usr/bin/env python3
"""VUC Binder_Bind original 552-byte C++03 Scope lifetime/context controls.

The real SymbolTable owns std::vector<Scope> of 0x804-byte buckets.
Compare genuine destructor/clear declarations and local source
lifetime variants. Never change the accepted declaration files.
Compile an isolated faithful header copy and function in the Lima guest.
"""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[1]
OUT=Path("/tmp/vuc-binutils215/binder-scope-lifetime-oct15t");OUT.mkdir(exist_ok=True)
root=R/"probes/BinderBindASTModuleFreshOct15s.cpp"
src=root.read_text()
src=src.replace('#include "../src/MLGameString.h"','#include "'+str(R/"src/MLGameString.h")+'"')
src=src.replace('#include "../src/MLHashFunGameString.h"','#include "'+str(R/"src/MLHashFunGameString.h")+'"')
src=src.replace('#include "../src/ASTNodeList.h"','#include "'+str(R/"src/ASTNodeList.h")+'"')
header=(R/"src/MLHashTable.h").read_text()
variants={
 "baseline":{},
 "dtor-throw":{"header":header.replace("~MLHashTable() { clear(); }","~MLHashTable() throw() { clear(); }")},
 "dtor-calls-this":{"header":header.replace("~MLHashTable() { clear(); }","~MLHashTable() { this->clear(); }")},
 "clear-throw":{"header":header.replace("void clear();","void clear() throw();")},
 "dtor-declared":{"header":header.replace("~MLHashTable() { clear(); }","~MLHashTable();")},
 "dtor-if-this":{"header":header.replace("~MLHashTable() { clear(); }","~MLHashTable() { if (this) clear(); }")},
 "dtor-helper":{"header":header.replace("~MLHashTable() { clear(); }","~MLHashTable() { this->clear(); return; }")},
 "local-reference":{"source":src.replace("SymbolTable table;","SymbolTable table; SymbolTable &local=table;").replace("table.BeginScope()","local.BeginScope()").replace("module->globals,table","module->globals,local").replace("module->variables,table","module->variables,local").replace("module->declarations,table","module->declarations,local").replace("table.Find(","local.Find(").replace("table.EndScope()","local.EndScope()")},
 "explicit-scope":{"source":src.replace("SymbolTable table;","SymbolTable table=SymbolTable();")},
 "main-name-const":{"source":src.replace('MLGameString mainName("main",false);','const MLGameString mainName("main",false);')},
 "main-name-bool":{"source":src.replace('if (!table.Find(mainName,false)) exportModule=true;','bool found = table.Find(mainName,false)!=0; if (!found) exportModule=true;')},
 "export-if-equal":{"source":src.replace('if (!table.Find(mainName,false)) exportModule=true;','if (table.Find(mainName,false)==0) exportModule=true;')},
 "end-scope-block":{"source":src.replace('    table.EndScope();\n    float4MemSymbols.EndScope();','    {table.EndScope();}\n    {float4MemSymbols.EndScope();}')},
}
rows=[]
for label,params in variants.items():
 h=OUT/(label+"-MLHashTable.h")
 h.write_text(params.get("header",header))
 # include relative to generated scratch for the entire header.
 cpp=OUT/(label+".cpp")
 source=params.get("source",src).replace('#include "../src/MLHashTable.h"','#include "'+str(h)+'"')
 cpp.write_text(source)
 for cc in ("jan25","dec06"):
  for alignment in (4,8):
   obj=OUT/(label+"-"+cc+"-j"+str(alignment)+".o")
   cmd=["/tmp/vuc-binutils215/g++-"+cc+"-as215","-O2","-frename-registers","-falign-jumps="+str(alignment),"-c",str(cpp),"-o",str(obj)]
   p=subprocess.run(cmd,capture_output=True,text=True)
   row={"variant":label,"compiler":cc,"jumps":alignment}
   if p.returncode:row["error"]=p.stderr[-170:]
   else:
    n=subprocess.run(["nm","-S","--defined-only",str(obj)],capture_output=True,text=True)
    symbols=[x.split() for x in n.stdout.splitlines() if x.endswith("_Z11Binder_BindP9ASTModule") and len(x.split())>=4]
    row["length"]=int(symbols[0][1],16) if symbols else None
   rows.append(row);print(row,flush=True)
(OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
