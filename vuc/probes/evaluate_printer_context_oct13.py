#!/usr/bin/env python3
"""Test original Printer.cpp method and instantiated-list ordering with GCC 3.3."""
import importlib.util
from pathlib import Path
import subprocess
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/printer-context-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
base=(R/'vuc/src/Printer.cpp').read_text().replace('struct ASTExpr {};','struct ASTExpr;')
base=base.replace('#include "../src/ASTNodeList.h"', '#include "'+str(R/'vuc/src/ASTNodeList.h')+'"')
list_path=R/'vuc/src/PrintASTLists.cpp'
variants={
 'module':base,
 'lists-after':base+'\n#include "'+str(list_path)+'"\n',
 'lists-before':base.replace('void Print(ASTFunCall *call,', '#include "'+str(list_path)+'"\nvoid Print(ASTFunCall *call,'),
}
# The accepted two functions share one real source unit; all probes verify
# original relocated symbols, original literals, and actual instruction bytes.
flags={
 'plain':['-O2'],
 'rename':['-O2','-frename-registers'],
 'full':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections'],
 'full-j1':['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections','-falign-jumps=1'],
 'plain-full':['-O2','-fuse-cxa-atexit','-ffunction-sections'],
 'no-sched':['-O2','-frename-registers','-fno-schedule-insns2','-fuse-cxa-atexit','-ffunction-sections'],
}
probes={'call':'print_ast_fun_context_oct14_probe.json','ugly':'ugly_printer_unit_oct13_probe.json','global':'iostream_global_printer_oct13_probe.json'}
for label,code in variants.items():
 src=T/(label+'.cpp');src.write_text(code)
 for opts,flag in flags.items():
  obj=T/(label+'-'+opts+'.o')
  c=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flag,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
  if c.returncode:
   print(label,opts,'COMPILE_ERROR',c.stderr[-180:],flush=True);continue
  results={}
  for p,cfg in probes.items():
   try:
    res=m.run(obj,R/'vuc/config'/cfg)
    results[p]=f"{res['exact_bytes']}/{res['length']}"
    if p=='call' and res['exact']:
     print('FULL CALL MATCH',label,opts,'other',results,flush=True)
   except ValueError as e:
    results[p]=str(e)[:90]
  print(label,opts,results,flush=True)
