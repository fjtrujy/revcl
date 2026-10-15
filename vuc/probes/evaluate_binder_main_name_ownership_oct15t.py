#!/usr/bin/env python3
"""Binder_Bind: old C++03 mainName lifetime and real scope ownership.

Original 552-byte binder checks a locally constructed MLGameString("main")
after Bind of three ASTNodeList members. Nearby authentic SymbolTable
methods and exception region constrain what can be changed without
inventing compiler padding or branch artifacts.
"""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[1];O=Path('/tmp/vuc-binutils215/binder-real-source-lifetime-oct15t');O.mkdir(exist_ok=True)
original=(R/'probes/BinderBindASTModuleFreshOct15s.cpp').read_text()
for header in ('MLGameString.h','MLHashFunGameString.h','MLHashTable.h','ASTNodeList.h'):
 original=original.replace('#include "../src/'+header+'"','#include "'+str(R/'src'/header)+'"')
anchor='    MLGameString mainName("main",false);\n    if (!table.Find(mainName,false)) exportModule=true;'
assert anchor in original
forms={
 'baseline':anchor,
 'eq-null':'    MLGameString mainName("main",false);\n    if (table.Find(mainName,false)==0) exportModule=true;',
 'local-entry':'    MLGameString mainName("main",false);\n    SymbolTableEntry *entry=table.Find(mainName,false);\n    if (!entry) exportModule=true;',
 'local-missing':'    MLGameString mainName("main",false);\n    bool missing = table.Find(mainName,false)==0;\n    if(missing) exportModule=true;',
 'local-found':'    MLGameString mainName("main",false);\n    bool found = table.Find(mainName,false)!=0;\n    if(!found) exportModule=true;',
 'inverted-if':'    MLGameString mainName("main",false);\n    if (table.Find(mainName,false)) {} else exportModule=true;',
 'if-block':'    MLGameString mainName("main",false);\n    if (!table.Find(mainName,false)) { exportModule=true; }',
 'assign-true':'    MLGameString mainName("main",false);\n    if (!table.Find(mainName,false)) exportModule = 1;',
 'boolean-flag':'    MLGameString mainName("main",false);\n    if (!table.Find(mainName,false)) exportModule = (bool)1;',
 'main-ref':'    const MLGameString mainName("main",false);\n    if (!table.Find(mainName,false)) exportModule=true;',
 'name-inner-scope':'    { MLGameString mainName("main",false);\n      if (!table.Find(mainName,false)) exportModule=true; }',
 'name-compare-to-int':'    MLGameString mainName("main",false);\n    if (table.Find(mainName,0)==(SymbolTableEntry*)0) exportModule=true;',
 'name-local-args':'    MLGameString mainName("main",false);\n    SymbolTable &owner=table;\n    if (!owner.Find(mainName,false)) exportModule=true;',
 'name-extra-local':'    MLGameString mainName("main",false);\n    bool withinCurrentScope=false;\n    if (!table.Find(mainName,withinCurrentScope)) exportModule=true;',
}
rows=[]
for label,replace in forms.items():
 cpp=O/(label+'.cpp');cpp.write_text(original.replace(anchor,replace))
 for snap in ('jan25','dec06'):
  obj=O/(label+'-'+snap+'.o')
  flags=['-O2','-frename-registers','-falign-jumps=8']
  p=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215',*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  row={'case':label,'compiler':snap}
  if p.returncode:row['error']=p.stderr[-190:]
  else:
   nm=subprocess.run(['nm','-S','--defined-only',str(obj)],capture_output=True,text=True)
   sy=[x.split() for x in nm.stdout.splitlines() if x.endswith('_Z11Binder_BindP9ASTModule') and len(x.split())>=4]
   row['length']=int(sy[0][1],16) if sy else None
  rows.append(row);print(row,flush=True)
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
