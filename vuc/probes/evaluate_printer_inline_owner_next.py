#!/usr/bin/env python3
"""Original Flex-era C++03 inline AST function printer ownership controls."""
from pathlib import Path
import subprocess, importlib.util
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/printer-inline-owner-authentic'); T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
s=(R/'vuc/probes/print_ast_fun_call.cpp').read_text().replace(
    '"../src/ASTNodeList.h"','"'+str(R/'vuc/src/ASTNodeList.h')+'"')
s=s.replace('void Print(ASTFunCall *call,','inline void Print(ASTFunCall *call,')
for deferred in (False,True):
 for label,flags in {
   'plain':['-O2','-fkeep-inline-functions'],
   'ren':['-O2','-frename-registers','-fkeep-inline-functions'],
   'j1':['-O2','-fkeep-inline-functions','-falign-jumps=1']
 }.items():
  name=('deferred' if deferred else 'eager')+'-'+label
  header=T/(name+'.h')
  header.write_text(('#pragma interface\n' if deferred else '')+s)
  cpp=T/(name+'.cpp')
  cpp.write_text(('#pragma implementation "'+header.name+'"\n' if deferred else '')+
                 '#include "'+header.name+'"\n')
  out=T/(name+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(cpp),'-o',str(out)],capture_output=True,text=True)
  if p.returncode:print('COMPILE',name,p.stderr[-120:],flush=True);continue
  row=next((x for x in subprocess.check_output(['nm','-S',str(out)],text=True).splitlines()
            if x.split()[-1:]==['_Z5PrintP10ASTFunCallRSo']),'')
  size=int(row.split()[1],16) if row else -1
  if size!=98:print('SIZE',name,size,flush=True);continue
  try:
   r=m.run(out,R/'vuc/config/print_ast_fun_call_probe.json')
   print('MATCH',name,r['exact_bytes'],r['different_offsets'],flush=True)
   if r['exact']:print('NEW_EXACT',name,flush=True);raise SystemExit(0)
  except ValueError as e:print('REJECT',name,str(e)[:135],flush=True)
