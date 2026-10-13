#!/usr/bin/env python3
"""Real AST-list printer source-order probe for VUC 96/98 Print(ASTFunCall).

Reposition unchanged genuine AST list template bodies/instantiations,
AST function call printer and actual Printer_UglyPrint wrapper. No
fabricated helpers or manual register constraints.
"""
import subprocess,importlib.util
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/printer-real-order-next');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
unit=(R/'vuc/src/Printer.cpp').read_text()
a=unit.index('void Print(ASTFunCall *')
b=unit.index('void Printer_UglyPrint(')
fun=unit[a:b];ugly=unit[b:]
lst=(R/'vuc/src/PrintASTLists.cpp').read_text()
template=lst[lst.index('template <class T>\nvoid Print('):]
header=unit[:a].replace('"../src/ASTNodeList.h"','"'+str(R/'vuc/src/ASTNodeList.h')+'"')
header+='void Print(ASTExpr *, std::ostream &);\nvoid Print(ASTStmt *, std::ostream &);\nvoid Print(ASTDecl *, std::ostream &);\n'
prefixes={
 'fun-alone':fun,
 'fun-ugly':fun+ugly,
 'ugly-fun':ugly+fun,
 'list-fun':template+fun,
 'fun-list':fun+template,
 'list-fun-ugly':template+fun+ugly,
 'list-ugly-fun':template+ugly+fun,
 'ugly-list-fun':ugly+template+fun,
 'fun-ugly-list':fun+ugly+template,
 'fun-list-ugly':fun+template+ugly,
 'ugly-fun-list':ugly+fun+template,
}
cfgA=R/'vuc/config/print_ast_fun_call_probe.json'
cfgB=R/'vuc/config/print_ast_fun_context_oct14_probe.json'
rank=[]
for name,code in prefixes.items():
 p=T/(name+'.cpp');p.write_text(header+code)
 for label,flags in {
  'plain':['-O2'],'ren':['-O2','-frename-registers'],
  'plain-j1':['-O2','-falign-jumps=1'],
  'ren-j1':['-O2','-frename-registers','-falign-jumps=1']
 }.items():
  obj=T/(name+'-'+label+'.o')
  cc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(p),'-o',str(obj)],capture_output=True,text=True)
  if cc.returncode:print('COMPILE',name,label,cc.stderr[-115:],flush=True);continue
  row=next((line for line in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines()
            if line.split()[-1:]==['_Z5PrintP10ASTFunCallRSo']),'')
  size=int(row.split()[1],16) if row else -1
  if size!=98:rank.append((0,size,name,label));continue
  for config in (cfgA,cfgB):
   try:
    out=m.run(obj,config)
    rank.append((out['exact_bytes'],size,name,label,config.name))
    if out['exact']:
     print('NEW_EXACT',str(p),label,config,flush=True);raise SystemExit(0)
    break
   except ValueError as e:pass
 print('DONE',name,flush=True)
print('TOP')
for row in sorted(rank,reverse=True)[:24]:print(row)
